/*
 * lcdisplay.h -- definitions for the char module
 *
 * Copyright (C) 2018 Jesuino Vieira Filho
 * Copyright (C) 2018 Lucas de Camargo Souza
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files. No warranty 
 * is attached; we cannot take responsibility for errors or 
 * fitness for use.
 */

#ifndef _LCDRIVER_H
#define _LCDRIVER_H

#include <linux/ioctl.h> // Needed for the _IOW etc stuff
#include <linux/mutex.h>
#include <linux/delay.h>

// ========================================================
// Useful defines

#define LCD_ADDRESS 		0x3F 		// Device address
#define LCD_NAME 			"lcdisplay" // Device name

#define LCD_CHR             0x01 		// Mode - Sending data
#define LCD_CMD             0x00 		// Mode - Sending command

#define LCD_LINE0           0x80 		// 1st line
#define LCD_LINE1           0xC0 		// 2nd line

#define LCD_ENABLE          0x04 		// Enable bit

// LCD Commands

#define LCD_CLEAR           0x01
#define LCD_HOME            0x02
#define LCD_BACKLIGHT       0x08

// ========================================================
// Low level functions

#define I2C_WRITE(client, data)         i2c_smbus_write_byte(client, data)
#define I2C_READ(client, data)          i2c_smbus_read_byte_data(client,data)

// ========================================================
// Delays

#define USLEEP(usecs)                   udelay(usecs)
#define MSLEEP(msecs)                   mdelay(msecs)

// ========================================================

/*
 * LCDisplay represents each device with a structure of type struct lcdriver_dev.
 */
typedef struct lcdisplay_dev
{
	int major;
	struct mutex mtx;
	struct cdev cdev;
	struct i2c_client *handle;

	u8 row;
	u8 column;
} lcd_t;

// ========================================================
// LCD Functions

void lcdinit(lcd_t *);
void lcdfinalize(lcd_t *);
void lcdclear(lcd_t *);
void lcdhome(lcd_t *);
void lcdrestart(lcd_t *);
void lcdwrite(lcd_t *, const char *);
void lcdsetbacklight(lcd_t *, u8);

#endif
