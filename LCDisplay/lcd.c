#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/ioctl.h> /* ioctl */

// Comandos para chamada de sistema ioctl
#define LCD_CLEAR           0x01
#define LCD_HOME            0x02
#define LCD_BACKLIGHT		0x08

char g_stop = 'J';

void* thread_function(void *arg)
{
	while(getchar() != 'q')
		printf("WARNING: press q + enter to stop\n");
	g_stop = 'q';
	
	printf("\nFinishing..\n");
	return NULL;
}

int main()
{
	printf("The application test is an loop. Press q + enter to stop\n");
	printf("----------------------------------------------------------\n\n");

	pthread_t t;

    pthread_create(&t, NULL, &thread_function, NULL); 

	int fd = open("/dev/lcdisplay", O_RDWR);

	if(fd < 0)
	{
		printf("WARNING: failed to open the device!\n");
		return -1;
	}

	printf("LCDisplay test session :)\n");
	
	write(fd, "Test session", 0);
	sleep(2);

	while(g_stop != 'q')
	{
		ioctl(fd, LCD_CLEAR, 0);
		
		write(fd, "Backlight off", 0);				sleep(3); if(g_stop == 'q') break;
		ioctl(fd, LCD_BACKLIGHT, 0);				sleep(3); if(g_stop == 'q') break;

		write(fd, "? Not anymore", 0);				sleep(3); if(g_stop == 'q') break;

		ioctl(fd, LCD_CLEAR, 0);
		write(fd, "Yes, I cleared the display", 0);	sleep(3); if(g_stop == 'q') break;
		
		ioctl(fd, LCD_CLEAR, 0);
		write(fd, "ABCDEFGHIJKLMNOPQRSTUVXZ", 0);	sleep(3); if(g_stop == 'q') break;
		write(fd, "0123456789", 0);					sleep(3); if(g_stop == 'q') break;
	}

	close(fd);
	pthread_join(t, NULL);
	
	return 0;
}
