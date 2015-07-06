#include "bizService.h"

static int bizServSock;	//业务Server Socket
static int bizClntSock;	//业务Client Socket

/*开启业务Server端口*/
void getBizServSock(short bizServPort)
{
	bizServSock = initServSock(bizServPort);
	if(bizServSock == -1)
	{
		perror("open bizServPort error:");
		exit(-1);
	}
}

/*接收业务网关数据线程执行例程*/
static void* recv_thread_routine(void *arg)
{
	char recvbuf[MAX_RECVBUFF] = {0};
	int recvlen;
	while( (recvlen = read(bizClntSock, recvbuf, MAX_RECVBUFF)) > 0){
		//TODO
		memset(recvbuf, 0, MAX_RECVBUFF);
	}
	return NULL;
}

/*发送业务网关数据线程执行例程*/
static void* send_thread_routine(void *arg)
{
	char str = *(char*)arg;
	printf("%c=====\n",str);
//	char a[1000] = {'a'};
//	int sendlen;
//	while( (sendlen = write(bizClntSock,a, sizeof(a))) > 0)
//		;
//	perror("send error:");
	return NULL;
}

/*业务线程程序,开启业务网关读、写线程*/
void* bizThreadRoutine(void* arg)
{
	while( (bizClntSock= accept(bizServSock, NULL, NULL)) == -1 )
		perror("accept bizClntSock error:");

	printf("biz clnt:%d\n",bizClntSock);
	pthread_t recv_pid, send_pid;
	int err;

	err = pthread_create(&recv_pid, NULL, recv_thread_routine, NULL);
	if(err){
		char* strErr = strerror(err);
		fprintf(stderr,"create biz recv_thread_routine error:%s\n",strErr);
		exit(-1);
	}

	err = pthread_create(&send_pid, NULL, send_thread_routine, NULL);
	if(err){
		char* strErr = strerror(err);
		fprintf(stderr,"create biz send_thread_routine error:%s\n",strErr);
		exit(-1);
	}

	char a = 'a';
	err = pthread_create(&send_pid, NULL, send_thread_routine, &a);
		if(err){
			char* strErr = strerror(err);
			fprintf(stderr,"create biz send_thread_routine error:%s\n",strErr);
			exit(-1);
		}
	pthread_exit(NULL);
}
