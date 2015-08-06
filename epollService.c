#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/time.h>
#include "epollService.h"
#include "Socket.h"
#include "fileCtrl.h"
#include "CodeTransform.h"
#include "ProtocolAnalysis.h"

#define RCEV_BUF_SIZE 4096
#define SEND_BUF_SIZE 4096
#define EPOLL_SIZE 11000
#define MAX_PTHREAD_NUM 20		//线程池中线程数量

static unsigned int thread_param[MAX_PTHREAD_NUM][3];//线程属性,0：标志线程是否空闲，1：标志线程序号，2：标志线程要处理的socket句柄
static pthread_t tid[MAX_PTHREAD_NUM];//线程ID
pthread_mutex_t thread_mutex[MAX_PTHREAD_NUM];//线程池互斥锁
int concentratorsSize = EPOLL_SIZE*2;
unsigned long concentrators[EPOLL_SIZE*2];	//存放集中器地址,sockfd作为下标
extern int bizClntSock;

/*初始化线程池和线程池互斥锁*/
static void init_pthread_pool();
/*线程池处理函数
 * @thread_para[] 线程信息
 * */
static void* pool_thread_handle(void *thread_para);//线程池处理函数


void epollService(short termServPort)
{
	init_pthread_pool();//初始化线程池

	int termServSock = initServSock(termServPort);
	if(termServSock == -1)
	{
		perror("open termServPort error:");
		exit(-1);
	}

	int epfd = epoll_create1(0);
	if(epfd == -1){
		perror("create epoll error:");
		exit(-1);
	}
	printf("epoll fd:%d\n", epfd);

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = termServSock;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, termServSock, &event) == -1){
		perror("add termServSock to epoll error:");
		exit(-1);
	}
	printf("termServSock:%d, termServPort:%d is waiting for connect...\n",termServSock, termServPort);

	int i;	//for循环用
	int clnt_sock;	//存放链接上来的客户端Socket句柄
	struct epoll_event ep_events[EPOLL_SIZE];	//存放epoll有事件发生时产生的event
	int event_cnt;
	while(1)
	{
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if(event_cnt == -1)
		{
			perror("epoll_wait() error:");
			continue;
		}
		//循环处理发生输入事件的句柄
		for(i=0; i<event_cnt; i++)
		{
			//服务端接收到新的连接
			if(ep_events[i].data.fd == termServSock)
			{
				clnt_sock=accept(termServSock, NULL, NULL);
				if(clnt_sock == -1)
				{
					perror("termServSock accpet error:");
					continue;
				}
				set_fl(clnt_sock, O_NONBLOCK); //设置成非阻塞
				event.events = EPOLLIN | EPOLLET;
				event.data.fd = clnt_sock;
				if(epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event)==-1)
				{
					perror("add clnt_sock to epoll error:");
					exit(-1);
				}
				printf("a term clnt_sock:%d connected\n",clnt_sock);
			}else{
				//处理客户端接收数据事件
				int j;
				//查找空闲线程
				for(j = 0; j < MAX_PTHREAD_NUM; j++)
				{
				   if (thread_param[j][0] == 0)
						break;
				}
				//没有找到空闲线程处理，则关闭此次socekt链接
				if (j >= MAX_PTHREAD_NUM)
				{
//					fprintf(stderr, "pthread pool full\r\n");
//					shutdown(events[n].data.fd, SHUT_RDWR);
//					close(events[n].data.fd);

					//myself
					fprintf(stderr, "can't find free epoll thread\n");
					continue;
				}
				//找到空闲线程，解锁处理数据
			   thread_param[j][0] = 1;//线程忙
			   thread_param[j][2] = ep_events[i].data.fd;//socket句柄
			   pthread_mutex_unlock(thread_mutex + j);//解锁睡眠锁
			}
		}
	}
	close(termServSock);
	close(epfd);
}

/*初始化线程池和线程池互斥锁*/
static void init_pthread_pool(){
	int i;
	for(i = 0; i < MAX_PTHREAD_NUM; i++) {
		thread_param[i][0] = 0;//线程空闲
		thread_param[i][1] = i;//线程索引
		pthread_mutex_lock(thread_mutex + i);//对应线程锁
	}
	int res;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
//	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	for(i = 0; i < MAX_PTHREAD_NUM; i++) {
		res = pthread_create(tid+i, &attr, pool_thread_handle, thread_param[i]);
		if (0 != res){
			fprintf(stderr, "create pthread in thread pool error:%s\n",strerror(res));
			exit(-1);
		}
   }
}

/*线程池处理函数
 * @thread_para[] 线程信息
 * */
static void* pool_thread_handle(void* thread_param)
{
	int * i_thread_param = (int*)thread_param;
   int clnt_sock;	//临时socket句柄
   unsigned char recvBuf[RCEV_BUF_SIZE];	//接收缓存区
   int recvlen;	//接收数据长度
   unsigned char sendBuf[SEND_BUF_SIZE];
   int shSize;	//实际转换后的上海协议字节数

   //线程属性分离
//   pthread_detach(pthread_self());
   int thread_index = i_thread_param[1]; //获取线程索引，以找到对应的互斥锁

	while(1)
	{
		pthread_mutex_lock(thread_mutex + thread_index);//加锁，没有数据接收时睡眠线程

		struct timeval t_start;
		gettimeofday(&t_start,NULL);

		clnt_sock = i_thread_param[2];//socket 句柄
		memset(recvBuf,0,sizeof(recvBuf));
		//接收终端发送的数据
		recvlen = recv(clnt_sock, recvBuf, sizeof(recvBuf), MSG_NOSIGNAL);
		if(recvlen <= 0){
			if(recvlen == -1 && errno == EAGAIN)
				continue;
			else{
				fprintf(stderr, "recv epoll term fd:%d error %d:%s, close clnt_sock\n",clnt_sock,errno,strerror(errno));
				close(clnt_sock);
				i_thread_param[0] = 0;//线程空闲
				continue;
			}
		}
		if(recvlen==5){	//recvlen==5时表示注册包或心跳包，注册包和心跳包都为集中器地址
			concentrators[clnt_sock]=bigEndian2long(recvBuf,recvlen);
//			printf("scoket:%d, concentrator:%ld login\n", clnt_sock, concentrators[clnt_sock]);
		}
		memset(sendBuf, 0, SEND_BUF_SIZE);
		Sz2Sh(concentrators[clnt_sock], recvBuf, recvlen, sendBuf, &shSize);

		struct timeval t_mid;
		gettimeofday(&t_mid,NULL);
		printf("mid:%ld\n", t_mid.tv_usec - t_start.tv_usec);

		if(bizClntSock > 0 && shSize!=0){
//			if(send(clnt_sock, sendBuf, shSize, MSG_NOSIGNAL) == -1)
//				fprintf(stderr, "echo clnt error %d:%s\n,",errno, strerror(errno));
//			if(send(bizClntSock, sendBuf, shSize, MSG_DONTWAIT) == -1)
//				fprintf(stderr, "send to biz error %d:%s\n,",errno, strerror(errno));
		}else{
			fprintf(stderr, "didn't send to bizClntSock:%d, shSize:%d, recvlen:%d\n",bizClntSock,shSize,recvlen);
//			printHexBytes("recv:",recvBuf,recvlen);
		}
		i_thread_param[0] = 0;//线程空闲

		struct timeval t_end;
		gettimeofday(&t_end,NULL);

		printf("end-mid:%ld\n", t_end.tv_usec - t_mid.tv_usec);
	}
   pthread_exit(NULL);
}
