#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include "Socket.h"
#include "LoadProfile.h"
#include "bizService.h"
#include "epollService.h"

static short bizServPort;	//业务连接端口
static short termServPort;	//终端连结端口

static void getInitConf();	//根据配置文件设置termServPort buzServPort
static void modifyRlimit(int resource, int rlim_cur, int rlim_max);	//修改进程的资源限制

int main(void)
{
	getInitConf();
	modifyRlimit(RLIMIT_NOFILE, 20000, 20000);	//设置进行可打开的最大文件句柄数

	//开启业务网关连接端口
	int bizServSock = initServSock(bizServPort);
	if(bizServSock == -1){
		fprintf(stderr,"open bizServSock failure\n");
		return -1;
	}
	printf("bizServSock:%d, bizServPort :%d is waiting for connect...\n",bizServSock, bizServPort);
	//创建业务网关处理线程
	pthread_t biz_pid;
	int err = pthread_create(&biz_pid, NULL, bizThreadRoutine, &bizServSock);
	if(err){
		fprintf(stderr,"create bizThreadRoutine error:%s\n",strerror(err));
		return -1;
	}

	//开启epoll，进行终端连接
	epollService(termServPort);
	return 0;
}

/*根据配置文件设置termServPort buzServPort*/
static void getInitConf(){
	const char * const param[] = {"termServPort","bizServPort"};
	int size = sizeof(param)/sizeof(char*);
	char termPortStr[10] = {0};
	char bizPortStr[10] = {0};
	char *value[2] = {termPortStr, bizPortStr};
	getConfigVal(param, value, size);
	termServPort = atoi(termPortStr);
	bizServPort = atoi(bizPortStr);
}
/*修改进程的资源限制，失败则程序退出*/
static void modifyRlimit(int resource, int rlim_cur, int rlim_max){
//	printf("max file:%d\n", sysconf(_SC_OPEN_MAX));
	struct rlimit rlim;
	rlim.rlim_cur = rlim_cur;
	rlim.rlim_max = rlim_max;
	if(setrlimit(resource, &rlim) == -1){
		perror("setrlimit error:");
		exit(-1);
	}
}
