#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "Socket.h"
#include "LoadProfile.h"

#define BUF_SIZE 100
#define EPOLL_SIZE 50

static short termServPort;	//终端连结端口
static short buzServPort;	//业务连接端口

void getInitConf();	//根据配置文件设置termServPort buzServPort
void clr_fl(int fd, int flags);

int main(void)
{
	getInitConf();
	int serv_sock, clnt_sock;
	int str_len, i;
	char buf[BUF_SIZE];

	struct epoll_event *ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	if( (serv_sock = initServSock(termServPort)) == -1)
		return -1;

	epfd = epoll_create1(0);
	ep_events = (struct epoll_event*)malloc( sizeof(struct epoll_event) * EPOLL_SIZE );

	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

	while(1){
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if(event_cnt == -1){
			puts("epoll_wait() error");
			break;
		}

		for(i=0; i<event_cnt; i++){
			if(ep_events[i].data.fd == serv_sock){
				clnt_sock=accept(serv_sock, NULL, NULL);
				event.events = EPOLLIN;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client: %d \n",clnt_sock);
			}else{
				str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
				if(str_len == 0){	//close request
					epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
					close(ep_events[i].data.fd);
					printf("closed client: %d \n", ep_events[i].data.fd);
				}else{
					write(ep_events[i].data.fd, buf, str_len);//echo
				}
			}
		}
	}
	close(serv_sock);
	close(epfd);
	return 0;
}

void getInitConf(){
	const char * const param[] = {"termServPort","buzServPort"};
	int size = sizeof(param)/sizeof(char*);
	char termPortStr[10] = {0};
	char buzPortStr[10] = {0};
	char *value[2] = {termPortStr, buzPortStr};
	getConfigVal(param, value, size);
	termServPort = atoi(termPortStr);
	buzServPort = atoi(buzPortStr);
}

void clr_fl(int fd, int flags)
{
	int flVal;
	if( (flVal = fcntl(fd, F_GETFL, 0)) == -1 )
		perror("fcntl F_GETFL error");

	flVal &= ~flags; //turn flags off

	if( fcntl(fd, F_SETFL, flVal) == -1 )
		perror("fcntl F_SETFL error");
}
