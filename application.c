#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <sys/ioctl.h>

#include <fcntl.h>



int freq;

int fd;



int main()

{

	fd = open("/dev/pwm_buzzer", O_WRONLY);

	if(fd == -1)

	{

		printf("Fail to open device lab4!\n");

		goto finish;	

	}

	while(1)

	{

		freq= scanf("%d", &freq);



		write(fd, &freq, 1);

		printf("Outout is %c \n", freq);

	}

	close(fd);



finish:	

	return 0;

}