#include "bizService.h"

static int bizServSock;	//业务Server Socket
int bizClntSock = -1;	//业务Client Socket，若为-1表示bizClntSock未准备好，需要等待业务网关连接

/*业务线程程序,等待业务网关连接，获得业务端口后*/
void* bizThreadRoutine(void* arg)
{
	bizServSock = *(int*)arg;
	while( (bizClntSock= accept(bizServSock, NULL, NULL)) == -1 )
		perror("accept bizClntSock error:");
	printf("biz client socket:%d connected\n",bizClntSock);

	char recvbuf[MAX_RECVBUFF] = {0};
	int recvlen;
	while(1)
	{
		memset(recvbuf, 0, MAX_RECVBUFF);
		recvlen = read(bizClntSock, recvbuf, MAX_RECVBUFF);
		if(recvlen <= 0)
		{
			//关闭旧biz client socket，并重新连接
			perror("read biz client socket error:");
			close(bizClntSock);
			bizClntSock = -1;	//表示bizClntSock不可用
			printf("close biz client socket, accept a new client socket\n");
			while( (bizClntSock= accept(bizServSock, NULL, NULL)) == -1 )
				perror("accept bizClntSock error:");
			printf("biz client socket:%d connected\n",bizClntSock);
		}else{
			//解析读取到的数据

		}
	}
	pthread_exit(NULL);
}

/*发送业务网关数据线程执行例程
static void* send_thread_routine(void *arg)
{
	char str = *(char*)arg;
	char a[10000];
	int i=0;
	for(i=0; i<10000; i++)
		a[i] = str;
	int sendlen;
	i = 0;
//	sendlen = write(bizClntSock,a, sizeof(a));
	while( (sendlen = write(bizClntSock,a, sizeof(a))) > 0 && i<10){
		printf("send %s: %d\n", a, sendlen);
		i++;
	}
	perror("send error:");
	return NULL;
}*/
