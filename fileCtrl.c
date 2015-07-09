#include <stdio.h>
#include "fileCtrl.h"

/*设置文件状态标志*/
void set_fl(int fd, int flags)
{
	int flVal;
	if( (flVal = fcntl(fd, F_GETFL, 0)) == -1 )
		perror("fcntl F_GETFL error");

	flVal |= flags; //turn on flags

	if( fcntl(fd, F_SETFL, flVal) == -1 )
		perror("fcntl F_SETFL error");
}

/*取消设置文件状态标志*/
void cancel_fl(int fd, int flags)
{
	int flVal;
	if( (flVal = fcntl(fd, F_GETFL, 0)) == -1 )
		perror("fcntl F_GETFL error");

	flVal &= ~flags; //turn flags off

	if( fcntl(fd, F_SETFL, flVal) == -1 )
		perror("fcntl F_SETFL error");
}
