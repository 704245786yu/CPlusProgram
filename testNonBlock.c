#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

char buf[500000];
void set_fl(int fd, int flags);
void clr_fl(int fd, int flags);

int main1(void)
{
	int ntowrite, nwrite;
	char *ptr;

	ntowrite = read(STDIN_FILENO, buf, sizeof(buf));
	fprintf(stderr, "read %d bytes\n", ntowrite);

	set_fl(STDOUT_FILENO, O_NONBLOCK); //set nonblocking

	ptr = buf;
	while (ntowrite > 0){
		errno = 0;
		nwrite = write(STDOUT_FILENO, ptr, ntowrite);
		fprintf(stderr, "nwrite = %d, errno = %d\n", nwrite, errno);

		if(nwrite > 0){
			ptr += nwrite;
			ntowrite -= nwrite;
		}
	}
	clr_fl(STDOUT_FILENO, O_NONBLOCK); //clear nonblocking
	exit(0);
}

/*@desc 对一个文件描述符打开一个或多个文件状态标志
 * flags are file status flags to turn on*/
void set_fl(int fd, int flags)
{
	int flVal;	//file status flags
	if( (flVal = fcntl(fd, F_GETFL, 0)) == -1 )
		perror("fcntl F_GETFL error");

	flVal |= flags; //turn on flags

	if( fcntl(fd, F_SETFL, flVal) == -1 )
		perror("fcntl F_SETFL error");
}

/*@desc 对一个文件描述符关闭一个或多个文件状态标志
 * flags are file status flags to turn off*/
void clr_fl(int fd, int flags)
{
	int flVal;
	if( (flVal = fcntl(fd, F_GETFL, 0)) == -1 )
		perror("fcntl F_GETFL error");

	flVal &= ~flags; //turn flags off

	if( fcntl(fd, F_SETFL, flVal) == -1 )
		perror("fcntl F_SETFL error");
}
