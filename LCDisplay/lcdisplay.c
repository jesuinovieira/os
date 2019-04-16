/*
 * lcdisplay.c -- char module.
 *
 *  Copyright (C) 2018 Jesuino Vieira Filho
 *  Copyright (C) 2018 Lucas de Camargo Souza
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files. No warranty 
 * is attached; we cannot take responsibility for errors or 
 * fitness for use.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/uaccess.h>	/* copy_*_user */
#include <linux/i2c.h>
#include "lcdisplay.h" 		/* local definitions */

// ========================================================

static struct i2c_client *client;
static struct i2c_adapter *adapter;
static lcd_t *lcd; /* Allocated in lcdisplay_init */

int lcdisplay_major = 0; /* Dynamic major by default */
int lcdisplay_minor = 0;

static void lcdsend(lcd_t *, int, int);

// ========================================================
// I2C structs
// ========================================================

static int lcdisplay_probe(struct i2c_client *, const struct i2c_device_id *);
static int lcdisplay_remove(struct i2c_client *);

/*
 * O kernel obtém informações sobre o endereço do dispositivo, o número 
 * do barramento e o nome do dispositivo que está sendo registrado através
 * dessa estrutura
 */
struct i2c_board_info lcdriver_board_info __initdata = {
	I2C_BOARD_INFO(LCD_NAME, LCD_ADDRESS)
};

/*
 * The I2C framework uses it to find the driver that is to be attached to 
 * a specific I2C device, represented by two members: name and driver_data.
 */
static const struct i2c_device_id lcdisplay_id[] = {
	{LCD_NAME, 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, lcdisplay_id);

/*
 * For each device on the system, there should be a driver that controls it. For 
 * the I2C device, the corresponding driver is represented by struct i2c_driver.
 */
static struct i2c_driver lcdriver = {
	.driver = {
        .owner  = THIS_MODULE,
		.name	= LCD_NAME,
	},
    .probe  	= lcdisplay_probe,
    .remove 	= lcdisplay_remove,
	.id_table	= lcdisplay_id,
};

// ========================================================
// Device struct
// ========================================================

static int 	   lcdisplay_open	(struct inode *, struct file *);
static int 	   lcdisplay_release(struct inode *, struct file *);
static ssize_t lcdisplay_write	(struct file *, const char __user *, size_t, loff_t *);
static ssize_t lcdisplay_read	(struct file *, char __user *, size_t, loff_t *);
loff_t 		   lcdisplay_llseek	(struct file *, loff_t, int);
static long    lcdisplay_ioctl	(struct file *, unsigned int, unsigned long);

/*
 * Essa estrutura contem os ponteiros para funções definidas 
 * pelo driver que executam varias operações no dispositivo.
 */
static struct file_operations lcdisplay_fops = {
	.owner 			= THIS_MODULE,
	.open 			= lcdisplay_open,
	.release 		= lcdisplay_release,
	.write 			= lcdisplay_write,
	.read  			= lcdisplay_read,
	.unlocked_ioctl = lcdisplay_ioctl,
};

// ========================================================
// I2C
// ========================================================

/*
 * This function is responsible for performing the first communication 
 * with the display. If all is correct, initialize the structure used 
 * to represent it and call the display initialization method.
 *
 * @param: i2c_client *client representa o dispositivo
 * @param: i2c_device_id *id contains the name and driver_data.
 * @return: 0 if all happens right, otherwise a error code will be returned
 */
static int lcdisplay_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	
	printk(KERN_DEBUG "LCDisplay: lcdisplay_probe() is called\n");

	lcd = (lcd_t *)devm_kzalloc(&client->dev, sizeof(lcd_t), GFP_KERNEL);
	if(!lcd)
		return -ENOMEM;

	cdev_init(&lcd->cdev, &lcdisplay_fops);
	lcd->cdev.owner = THIS_MODULE;
	lcd->cdev.ops = &lcdisplay_fops;
	err = cdev_add(&lcd->cdev, MKDEV(lcdisplay_major, lcdisplay_minor), 1);
	
	if(err)
	{
		printk(KERN_NOTICE "Error %d adding cdev", err);
		return -ENOMEM;
	}

	lcd->row 	= 0;
	lcd->column = 0;
	lcd->major  = lcdisplay_major;
	lcd->handle = client;

	i2c_set_clientdata(client, lcd);
	mutex_init(&lcd->mtx);

    lcdinit(lcd);

	return 0;
}

/*
 * Invokes the display finishing method
 * 
 * @param: i2c_client *client representa o dispositivo conectado ao barramento i2c
 * @return: return 0 if all happens right, otherwise -1 will be returned 
 */
static int lcdisplay_remove(struct i2c_client *client)
{
	lcd_t *l;

	printk(KERN_DEBUG "LCDisplay: lcdisplay_remove() is called\n");

	l = i2c_get_clientdata(client);

    if(l)
		lcdfinalize(l);
	else
		return -1;

	return 0;
}

// ========================================================
// Device
// ========================================================

/*
 * Routine call when the open system call is performed at the user level
 *
 * @param: struct inode *inode
 * @param: struct file *file
 * @return: 0 if all happens right, otherwise a error code will be returned
 */
static int lcdisplay_open(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "LCDisplay: lcdisplay_open() is called\n");

	if(mutex_lock_interruptible(&lcd->mtx))
		return -ERESTARTSYS;
		
    try_module_get(THIS_MODULE);

    lcdrestart(lcd);
    lcdsetbacklight(lcd, 1);

	mutex_unlock(&lcd->mtx);
	
    return 0;
}

/*
 * Routine call when the close system call is performed at the user level
 *
 * @param: struct inode *inode
 * @param: struct file *file
 * @return: 0 if all happens right, otherwise a error code will be returned
 */
static int lcdisplay_release(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "LCDisplay: lcdisplay_release() is called\n");

	if(mutex_lock_interruptible(&lcd->mtx))
		return -ERESTARTSYS;
	
	lcdrestart(lcd);
	lcdsetbacklight(lcd, 0);

    module_put(THIS_MODULE);
	mutex_unlock(&lcd->mtx);
    
    return 0;
}

/*
 * Routine call when the write system call is performed at the user level
 *
 * @param: struct file *file
 * @param: const char __user *buffer which is the string to be written
 * @param: size_t length of the string
 * @param: loff_t *offset unused
 * @return: number of bytes written to the screen, otherwise a error code will be returned
 */
static ssize_t lcdisplay_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
	printk(KERN_DEBUG "LCDisplay: lcdisplay_write() is called\n");

	if(!buffer)
		return 0;

	if(mutex_lock_interruptible(&lcd->mtx))
		return -ERESTARTSYS;
	
	lcdwrite(lcd, buffer);

	mutex_unlock(&lcd->mtx);

	return sizeof(buffer);
}

/*
 * TODO: requires the implementation of a buffer in the structure representing the device
 *
 * Routine call when the read system call is performed at the user level
 *
 * @param: struct file *file
 * @param: char __user *buffer 
 * @param: size_t length
 * @param: loff_t *offset
 * @return:
 */
static ssize_t lcdisplay_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
	// TODO
	printk(KERN_DEBUG "LCDisplay: lcdisplay_read() is called\n");

	return 0;
}

/*
 * Routine call when the ioctl system call is performed at the user level
 *
 * @param: struct file *file
 * @param: unsigned int cmd which is the command to execute
 * @param: unsigned long arg which is the parameter, if necessary.
 *
 * Currently, the following commands are possible:
 *
 * LCD_CLEAR: 		0x01
 * LCD_HOME:        0x02
 * LCD_BACKLIGHT:	0x08
 *
 * @return: number of bytes written to the screen, otherwise a error code will be returned
 */
static long lcdisplay_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_DEBUG "LCDisplay: lcdisplay_ioctl() is called\n");

	if(mutex_lock_interruptible(&lcd->mtx))
		return -ERESTARTSYS;

	switch(cmd)
	{
		case LCD_CLEAR:
			lcdclear(lcd);
			break;
		
		case LCD_HOME:
			lcdhome(lcd);
			break;

		case LCD_BACKLIGHT:
			lcdsetbacklight(lcd, arg);
			break;

		default:
			printk(KERN_DEBUG "LCDisplay: unknown IOCTL\n");
	      	break; 
	}

	mutex_unlock(&lcd->mtx);

	return 0;
}

/*
 * Invoked when the module is loaded into the kernel
 *
 * @param: none
 * @return: none
 */
static int __init lcdisplay_init(void)
{	
	int tmp;
    dev_t dev = 0;

	printk(KERN_DEBUG "LCDisplay: lcdisplay_init() is called\n");

	// Part 01 =======================================================
	// Registra um número para o dispositivos de caractere

	if(lcdisplay_major) // Major pre-definido
	{
		dev = MKDEV(lcdisplay_major, lcdisplay_minor);
		tmp = register_chrdev_region(dev, 1, LCD_NAME);
	}
	
	else // Alocacao dinamica de major
	{
		tmp = alloc_chrdev_region(&dev, lcdisplay_minor, 1, LCD_NAME);
		lcdisplay_major = MAJOR(dev);
	}
	
    if(tmp < 0)
    {
        printk(KERN_WARNING "LCDisplay: can't get major number %d\n", lcdisplay_major);
        return tmp;
    }
	
    // Part 02 =======================================================
    // Informa o kernel sobre o dispositivo presente no barramento I2C

	adapter = i2c_get_adapter(1); // Argument: bus number
    if(!adapter)
    {
    	printk(KERN_WARNING "LCDisplay: error getting i2c adapter\n");
    	goto fail;
    }

    client = i2c_new_device(adapter, &lcdriver_board_info);
    if(!client)
    {
    	printk(KERN_WARNING "LCDisplay: error registering i2c device\n");
    	goto fail;
    }

	tmp = i2c_add_driver(&lcdriver);
    if(tmp < 0)
	{
		printk(KERN_WARNING "LCDisplay: error adding i2c driver\n");
		goto fail;
	}

    i2c_put_adapter(adapter);

    return 0; /* Succeed */
	
	fail:
		unregister_chrdev_region(dev, 1);
		if(client)
			i2c_unregister_device(client);
    	return -EINVAL;
}

/*
 * Invoked when the module is removed from the kernel
 *
 * @param: none
 * @return: none
 */
static void __exit lcdisplay_exit(void)
{
	printk(KERN_DEBUG "LCDisplay: lcdisplay_exit() is called\n");
	
	unregister_chrdev_region(MKDEV(lcdisplay_major, lcdisplay_minor), 1);
    unregister_chrdev(lcdisplay_major, LCD_NAME);

    if(client)
		i2c_unregister_device(client);

    i2c_del_driver(&lcdriver);
}


// ========================================================
// Display
// ========================================================

/*
 * Toggle LCD_ENABLE pin on LCD display
 *
 * @param: lcd_t *l handler structure address
 * @param: int bits high or low
 * @return: none
 */
static void lcdtoggleenable(lcd_t *l, int bits)
{
	if(!l)
		return;

	USLEEP(250);
	I2C_WRITE(l->handle, (bits | LCD_ENABLE));
	USLEEP(250);
	I2C_WRITE(l->handle, (bits & ~LCD_ENABLE));
	USLEEP(250);
}

/*
 * Send byte to data pins
 *
 * @param: lcd_t *l handler structure address
 * @param: u8 byte to send to LCD
 * @param: u8 mode of communication (RS line of a LCD). 1 for data, 0 for command
 * @return: none
 */
static void lcdsend(lcd_t *l, int bits, int mode)
{
	u8 bits_high;
	u8 bits_low;
	
	if(!l)
		return;

	// Uses the two half byte writes to LCD
	bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
	bits_low  = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

	// High bits
	I2C_WRITE(l->handle, bits_high);
	lcdtoggleenable(l, bits_high);

	// Low bits
	I2C_WRITE(l->handle, bits_low);
	lcdtoggleenable(l, bits_low);
}

/*
 * Initialization procedure of LCD
 *
 * @param: lcd_t* l handler structure address
 * @return: none
 */
void lcdinit(lcd_t *l)
{
	printk(KERN_DEBUG "LCDisplay: lcdinit() is called\n");

	if(!l)
		return;

	lcdsend(l, 0x33, LCD_CMD); // Initialize
	lcdsend(l, 0x32, LCD_CMD); // Initialize
	lcdsend(l, 0x06, LCD_CMD); // Cursor move direction
	lcdsend(l, 0x0C, LCD_CMD); // 0x0F Blink On, 0x0C Blink Off
	lcdsend(l, 0x28, LCD_CMD); // Data length, number of lines, font size
	
	lcdclear(l); // Clear display
	lcdhome(l);  // Move curosr to home
	
	USLEEP(500);

	l->row = 0;
	l->column = 0;

	lcdsend(l, LCD_LINE0, LCD_CMD);
	lcdwrite(l, "Driver LCDisplay");
}

/*
 * LCD deinitialization procedure
 *
 * @param: lcd_t* l handler structure address
 * @return: none
 */
void lcdfinalize(lcd_t *l)
{
	printk(KERN_DEBUG "LCDisplay: lcdfinalize() is called\n");
	
	if(!l)
		return;
	
	lcdrestart(l);
	lcdsetbacklight(l, 0);
}

/*
 * Clear the display and return home
 *
 * @param: lcd_t* l handler structure address
 * @return: none
 */
void lcdclear(lcd_t *l)
{
	if(!l)
		return;

    lcd->column = 0;
    lcd->row = 0;

	lcdsend(l, LCD_CLEAR, LCD_CMD);
	MSLEEP(2);
}

/*
 * Set LCD back to home, position (0,0)
 *
 * @param lcd_t *l handler structure address
 * @return none
 */
void lcdhome(lcd_t *l)
{
	if(!l)
		return;

    lcd->column = 0;
    lcd->row = 0;

	lcdsend(l, LCD_HOME, LCD_CMD);
    MSLEEP(2);
}

/*
 * Restart display: lcdclear + lcdhome
 *
 * @param: lcd_t* l handler structure address
 * @return: none
 */
void lcdrestart(lcd_t *l)
{
	if(!l)
		return;

	lcdclear(l);
	lcdhome(l);
}

/*
 * This allows to print a string at current position
 *
 * @param: lcd_t* l handler structure address
 * @param: const char *s string to print
 * @return: none
 */
void lcdwrite(lcd_t *l, const char *s)
{
	if(!l || !s)
		return;

	while(*s)
    {
		lcdsend(l, *(s++), LCD_CHR);

        lcd->column++;
        
        if(lcd->column == 16) // Number of columns
        {
        	if(lcd->row == 0)
        	{
        		lcd->row = 1;
        		lcdsend(l, LCD_LINE1, LCD_CMD);
        	}

        	else
        	{
        		lcd->row = 0;
        		lcdsend(l, LCD_LINE0, LCD_CMD);
        	}

        	lcd->column = 0;
        }
    }
}

/*
 * Switches backlight on or off.
 *
 * @param lcd_t *l handler structure address
 * @param u8 0 value switchs backlight off, otherwise backlight will
 * be switched on
 * @return none
 */
void lcdsetbacklight(lcd_t *l, u8 bl)
{
	if(!l)
		return;

	I2C_WRITE(l->handle, bl ? LCD_BACKLIGHT : 0);
}

/*
 * As macros module_init e module_exit notificam o
 * kernel sobre o dispositivo carregado e descarregado.
 */
module_init(lcdisplay_init);
module_exit(lcdisplay_exit);

MODULE_DESCRIPTION("LCD Display I2C");
MODULE_AUTHOR("Jesuino Vieira Filho, Lucas de Camargo Souza");
MODULE_LICENSE("GPL");
