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
#include "bizService.h"

#define RCEV_BUF_SIZE 4096
#define SEND_BUF_SIZE 4096
#define EPOLL_SIZE 11000
#define MAX_PTHREAD_NUM 20		//线程池中线程数量
#define MAX_TIMEOUT 60	//最大超时时间, 5分钟

static unsigned int thread_param[MAX_PTHREAD_NUM][3];//线程属性,0：标志线程是否空闲，1：标志线程序号，2：标志线程要处理的socket句柄
static pthread_t tid[MAX_PTHREAD_NUM];//线程ID
pthread_mutex_t thread_mutex[MAX_PTHREAD_NUM];//线程池互斥锁
int concentratorsSize = EPOLL_SIZE*2;
unsigned long concentrators[EPOLL_SIZE*2];	//存放集中器地址,sockfd作为下标
concentrator_struct concentrator_ary[EPOLL_SIZE];
int concentratorNum = 0;	//已加入的集控器数
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;	//控制二分表的读写

int clientNum = 0;

extern int bizClntSock;

/*初始化线程池和线程池互斥锁*/
static void init_pthread_pool();
/*线程池处理函数
 * @thread_para[] 线程信息
 * */
static void* pool_thread_handle(void *thread_para);//线程池处理函数
/*二分法查找插入*/
static int create_concentrator_table(concentrator_struct tempConcentrator);
/*定时扫描二叉表，找出超过超时时间未发心跳的集控器*/
void* timer_routine(void *arg);


void epollService(short termServPort)
{
	init_pthread_pool();//初始化线程池
	//启动定时器
	pthread_t timer;
	int err = pthread_create(&timer, NULL, timer_routine, NULL);
	if(err){
		fprintf(stderr,"create timer_routine error:%s\n",strerror(err));
		exit(-1);
	}

	int termServSock = initServSock(termServPort);
	if(termServSock == -1)
	{
		perror("open termServPort error:");
		exit(-1);
	}
	set_fl(termServSock, O_NONBLOCK); //设置成非阻塞

	int epfd = epoll_create1(0);
	if(epfd == -1){
		perror("create epoll error:");
		exit(-1);
	}

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = termServSock;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, termServSock, &event) == -1){
		perror("add termServSock to epoll error:");
		exit(-1);
	}
	printf("termServ sockfd:%d, port:%d is waiting for connect...\n",termServSock, termServPort);

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
				printf("accepted clientNum:%d\n",++clientNum);
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
					fprintf(stderr, "epoll busy,close clnt_fd:%d, clientNum:%d\n", ep_events[i].data.fd,--clientNum);
					shutdown(ep_events[i].data.fd, SHUT_RDWR);
					close(ep_events[i].data.fd);
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
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	for(i = 0; i < MAX_PTHREAD_NUM; i++) {
		res = pthread_create(tid+i, NULL, pool_thread_handle, thread_param[i]);
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
			if(recvlen == -1 && errno == EAGAIN){
				i_thread_param[0] = 0;//线程空闲
				continue;
			}else{
				fprintf(stderr, "recv term sockfd:%d error %d:%s, close, clientNum:%d\n",clnt_sock,errno,strerror(errno),--clientNum);
				close(clnt_sock);
				i_thread_param[0] = 0;//线程空闲
				continue;
			}
		}

		//设置上海协议的命令码
		unsigned short shCmd = 0;
		char isIncluHeart = 0;	//协议中是否包含心跳包
		switch(recvlen){
		case 5:	//recvlen==5时表示注册包或心跳包，注册包和心跳包都为集中器地址
			shCmd = 0xFF11;
			break;
		case 29:	//终端(灯控器)发送数据格式
			shCmd = 0xFF21;
			break;
		case 34:	//29表示注册包或心跳包与该帧一并发送过来
			shCmd = 0xFF21;
			isIncluHeart = 1;
			break;
		case 10:	//回路查询响应帧 和 控制指令响应帧
			shCmd = 0xFF31;
			break;
		case 15:
			shCmd = 0xFF31;
			isIncluHeart = 1;
			break;
		case 53:	//3相电查询响应帧
			shCmd = 0xFF33;
			break;
		case 58:
			shCmd = 0xFF33;
			isIncluHeart = 1;
			break;
		case 20:	//读取网关本地时间响应帧
			shCmd = 0xFF34;
			break;
		case 25:
			shCmd = 0xFF34;
			isIncluHeart = 1;
			break;
		case 30:	//读取策略时间响应帧
			shCmd = 0xFF35;
			break;
		case 35:
			shCmd = 0xFF35;
			isIncluHeart = 1;
			break;
		default:	//无匹配的帧直接返回
			break;
		}

		//更新二叉树
		if(shCmd == 0xFF11 || isIncluHeart==1)
		{
			concentrators[clnt_sock]=bigEndian2long(recvBuf,5);
			concentrator_struct tempConcentrator = {concentrators[clnt_sock], clnt_sock, 0};
			int res;
			if( (res=pthread_rwlock_wrlock(&rwlock)) == 0 ){
				create_concentrator_table(tempConcentrator);
				pthread_rwlock_unlock(&rwlock);
			}else{
				fprintf(stderr, "clnt_sock:%d, rdlock error:%d, %s\n", clnt_sock, res, strerror(res));
			}
//			printf("scoket:%d, concentrator:%ld login\n", clnt_sock, concentrators[clnt_sock]);
		}

		if(shCmd)
		{
			if(isIncluHeart == 0){	//不包含心跳包的数据包
				memset(sendBuf, 0, SEND_BUF_SIZE);
				Sz2Sh(concentrators[clnt_sock], recvBuf, recvlen, shCmd, sendBuf, &shSize);
				if(send(clnt_sock, sendBuf, shSize, MSG_NOSIGNAL) == -1)
					fprintf(stderr, "echo clnt error %d:%s\n",errno, strerror(errno));
//				if(send(bizClntSock, sendBuf, shSize, MSG_NOSIGNAL) == -1)
//					fprintf(stderr, "send to biz:%d error %d:%s\n",bizClntSock, errno, strerror(errno));
				sendMsg(sendBuf, shSize);
			}else{
				memset(sendBuf, 0, SEND_BUF_SIZE);
				Sz2Sh(concentrators[clnt_sock], recvBuf, 5, shCmd, sendBuf, &shSize);	//截取心跳
				if(send(clnt_sock, sendBuf, shSize, MSG_NOSIGNAL) == -1)
					fprintf(stderr, "echo clnt error %d:%s\n",errno, strerror(errno));
				if(send(bizClntSock, sendBuf, shSize, MSG_DONTWAIT) == -1)
					fprintf(stderr, "send to biz error %d:%s\n",errno, strerror(errno));

				memset(sendBuf, 0, SEND_BUF_SIZE);
				Sz2Sh(concentrators[clnt_sock], recvBuf+5, recvlen-5, shCmd, sendBuf, &shSize);	//截取响应数据
				if(send(clnt_sock, sendBuf, shSize, MSG_NOSIGNAL) == -1)
					fprintf(stderr, "echo clnt error %d:%s\n",errno, strerror(errno));
				if(send(bizClntSock, sendBuf, shSize, MSG_DONTWAIT) == -1)
					fprintf(stderr, "send to biz error %d:%s\n",errno, strerror(errno));
			}
		}else{
			//无法匹配收到的数据包
			fprintf(stderr, "recv term:%d length error:%d\n", clnt_sock, recvlen);
//			printHexBytes("recv:",recvBuf,recvlen);
		}
		i_thread_param[0] = 0;//线程空闲

		struct timeval t_end;
		gettimeofday(&t_end,NULL);
		long interval = t_end.tv_usec - t_start.tv_usec;
		if(interval > 1000 || interval < 0)
			printf("end-start:%ld\n", t_end.tv_usec - t_start.tv_usec);
	}
   pthread_exit(NULL);
}

/*定时扫描二叉表，移除超过超时时间未发心跳的集控器*/
void* timer_routine(void *arg)
{
	int i;
	while(1){
		sleep(10);
		for(i=0; i<concentratorNum; i++)
		{
			pthread_rwlock_wrlock(&rwlock);
			if(concentrator_ary[i].timeout > MAX_TIMEOUT){	//超过超时时间移除该终端
				memmove(concentrator_ary+i, concentrator_ary+i+1, concentratorNum-i);
				concentratorNum--;
				printf("concentratorNum--");
			}else
				concentrator_ary[i].timeout++;
			pthread_rwlock_unlock(&rwlock);
		}
		printf("concentratorNum:%d, last+1 concentrator sock:%d\n", concentratorNum, concentrator_ary[concentratorNum].sock_fd);
	}
	return NULL;
}

 /*二分法查找*/
int find_concentrator(unsigned long concentrator){
	int low = 0;
	int high = concentratorNum;
	int mid = 0;

	while(low <= high){
		mid = (low + high)/2;
		if(concentrator_ary[mid].concentratro == concentrator)
			return mid;
		if(concentrator_ary[mid].concentratro < concentrator)
			low = mid + 1;
		else
			high = mid - 1;
	}
	return -1;
}

/*二分法插入，并进行数据的更新*/
int times = 0;
static int create_concentrator_table(concentrator_struct tempConcentrator)
{
	if(concentratorNum >= EPOLL_SIZE)
		return -1;

	int low = 0;
	int high = concentratorNum - 1;
	int mid = 0;

	while(low <= high){
		mid = (high + low)/2;
		if(concentrator_ary[mid].concentratro == tempConcentrator.concentratro){
			//集控器已存在更新数据
			concentrator_ary[mid] = tempConcentrator;
			return 0;
		}
		if(concentrator_ary[mid].concentratro < tempConcentrator.concentratro)
			low = mid + 1;
		else
			high = mid - 1;
	}
	//不存在插入
	int i;
	for(i = concentratorNum - 1; i >= low; i--){
		concentrator_ary[i+1] = concentrator_ary[i];
	}
	concentrator_ary[low] = tempConcentrator;
	concentratorNum++;
//	printf("times:%d\n", ++times);
	return 0;
}
