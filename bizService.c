#include <errno.h>
#include "bizService.h"
#include "CodeTransform.h"
#include "epollService.h"

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
			perror("read biz client socket error");
			close(bizClntSock);
			bizClntSock = -1;	//表示bizClntSock不可用
			printf("close biz client socket, accept a new client socket\n");
			while( (bizClntSock= accept(bizServSock, NULL, NULL)) == -1 )
				perror("accept bizClntSock error");
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
extern int concentratorNum;
extern concentrator_struct concentrator_ary[];
extern pthread_rwlock_t rwlock;

static int unpack(unsigned char recvbuf[], int buflen)
{
	int i;
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
				if(recvbuf[i+25]==0xFF && recvbuf[i+26]==0xFF){
					//命令码为0xFFFF表示广播
					int j;
					for(j=0; j<concentratorNum; j++){
						if(send(concentrator_ary[j].sock_fd, recvbuf+i+34, datalen, MSG_NOSIGNAL) <= 0)	//发送数据到终端
							fprintf(stderr, "send to term sockfd:%d failed!Errno:%d, %s\n", concentrator_ary[j].sock_fd, errno, strerror(errno));
					}
				}else{
					unsigned long concentrator = bigEndian2long(recvbuf+i+1,8);
					//寻找concentrator对应的socket
					int res;
					if( (res=pthread_rwlock_rdlock(&rwlock)) == 0 ){
						int index = find_concentrator(concentrator);
						pthread_rwlock_unlock(&rwlock);
						if(index != -1)
							if(send(concentrator_ary[index].sock_fd, recvbuf+i+34, datalen, MSG_NOSIGNAL) == -1)	//发送数据到终端
								fprintf(stderr, "send to epoll client socket %d failed!\n", concentrator_ary[index].sock_fd);
					}else
						fprintf(stderr, "get rdlock error:%d, %s\n", res, strerror(res));
				}
				i = tail;
				lastIndex = tail+1;
			}
		}
	}
	return lastIndex;
}
