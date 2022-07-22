#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

#define I2C_PATH "/dev/2arduino_i2c"
#define GPIO_PATH "/dev/gpioled_Dev"
#define BUF_SIZE 16
#define USEC 1000000

int main(int argc, char* argv[]){
	int ret;
	int fd1, fd2;
	char buf[BUF_SIZE];
	fd1=open(I2C_PATH, O_RDWR);
	fd2=open(GPIO_PATH, O_RDWR);
	if(fd1 && fd2){
		printf("fd1=%d, fd2=%d\n", fd1, fd2);
	}else{
		printf("open() was failed, fd1=%d, fd2=%d\n", fd1, fd2);
	}
	while(1){
		memset((void*)buf, 0, BUF_SIZE);
		ret=read(fd1, buf, BUF_SIZE);
		printf("buf=%c, ret=%d\n", buf[0], ret);
		
		switch (buf[0]){
			case 'H':
				write(fd2, "1", 1);
				break;
			case 'L':
				write(fd2, "0", 1);
				break;
			default:
				printf("%s: read() was failed\n", __func__);
		}
		usleep(0.5*USEC);
	}
	close(fd1);
	close(fd2);
	return 0;
}
