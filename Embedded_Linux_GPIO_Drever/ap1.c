#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>

#define FILE_PATH "/dev/gpioled_Dev"
#define BUF_SIZE 64
#define USEC 1000000

int write2dev(int fd, const char* _char){
	int ret, len;
	char *buf;
	char *bufadr;
	
	len=strlen(_char);
	buf=malloc(len*sizeof(char));
	memset((void* )buf, '0', len);
	
	strcpy(buf, _char);
	
	bufadr=buf;
	while(len>0){
		ret=write(fd, (void *)bufadr, len);
		if(ret<0){
			printf("write() was failed\n");
			close(fd);
			return -1;
		}
	len=len-ret;
	bufadr=bufadr+ret;
	}

	free(buf);

	return len;
}

int main(int argc, char* argv[]){
	int fd, ret;

	fd=open(FILE_PATH, O_RDWR);
	
	while(1){
		/* LEN on */
		ret=write2dev(fd, "1");
		usleep(0.5*USEC);
		/* LEN off */
		ret=write2dev(fd, "0");
		usleep(0.5*USEC);
	}

	close(fd);

	return 0;
}

/** 
	int open (const char *filename, int flags[, mode_t mode]) 
	int close (int filedes) 
	ssize_t read (int filedes, void *buffer, size_t size)  
	ssize_t write (int filedes, const void *buffer, size_t size) 

*/
