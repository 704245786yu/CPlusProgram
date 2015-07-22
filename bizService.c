#include "bizService.h"
#include "CodeTransform.h"

static int bizServSock;	//业务Server Socket
int bizClntSock = -1;	//业务Client Socket，若为-1表示bizClntSock未准备好，需要等待业务网关连接

/*解包
 * @return lastIndex 最后未成功解析的帧头位置
 * */
static int unpack(unsigned char recvbuf[], int buflen);

/*业务线程程序,等待业务网关连接，获得业务端口后*/
void* bizThreadRoutine(void* arg)
{
	bizServSock = *(int*)arg;
	while( (bizClntSock= accept(bizServSock, NULL, NULL)) == -1 )
		perror("accept bizClntSock error:");
	printf("biz client socket:%d connected\n",bizClntSock);

	unsigned char recvbuf[MAX_RECVBUFF] = {0};
	int recvlen;
	int offset = 0;	//redvbuf的偏移量，偏移量之前的内容为上次解析剩下的包
	while(1)
	{
		memset(recvbuf+offset, 0, MAX_RECVBUFF-offset);
		recvlen = read(bizClntSock, recvbuf+offset, MAX_RECVBUFF-offset);
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
			int buflen = recvlen + offset;
//			printHexBytes("recv:",recvbuf, buflen);
			int lastIndex = unpack(recvbuf, buflen);
			if(lastIndex == buflen){
				offset = 0;
			}else{
				offset = buflen-lastIndex;
				memmove(recvbuf, recvbuf+lastIndex, offset);
			}
		}
	}
	pthread_exit(NULL);
}

/*解包
 * @return lastIndex 最后未成功解析的帧头位置
 * */
extern int concentratorsSize;
extern unsigned long concentrators[];
static int unpack(unsigned char recvbuf[], int buflen)
{
	int i;
	int term_sockfd;	//concentrators的下标对应相应的终端socketfd
	int lastIndex = 0; //最后未成功解析的帧头位置
	for(i=0; i<buflen; i++)
	{
		if(recvbuf[i] == 0xAA)
		{
			//帧起始符为0xAA的同时，要保证帧结束符为0x55
			int datalen = bigEndian2int(recvbuf+i+30, 4);
			int tail = i + 34 + datalen + 2;	//帧结束符位置
			if(recvbuf[tail] == 0x55)
			{
				unsigned long concentrator = bigEndian2long(recvbuf+i+1,8);
				//寻找concentrator对应的socket
				for(term_sockfd=0; term_sockfd<concentratorsSize; term_sockfd++)
				{	//if里的第二个判断条件为广播
					if(concentrators[term_sockfd] == concentrator ||
							(recvbuf[i+25]==0xFF && recvbuf[i+26]==0xFF && concentrators[term_sockfd]!=0) ){
						if(send(term_sockfd, recvbuf+i+34, datalen, MSG_NOSIGNAL) <= 0)	//发送数据到终端
							fprintf(stderr, "send to epoll client socket %d failed!\n", term_sockfd);
						else
							printf("send to epoll client socket fd:%d success\n",term_sockfd);
						break;
					}
				}
				i = tail;
				lastIndex = tail+1;
			}
		}
	}
	return lastIndex;
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
