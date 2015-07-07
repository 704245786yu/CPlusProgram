#include <stdlib.h>
#include <unistd.h>
#include "epollService.h"
#include "Socket.h"

void epollService(short termServPort)
{
	int termServSock = initServSock(termServPort);
	if(termServSock == -1)
	{
		perror("open termServPort error:");
		exit(-1);
	}

	struct epoll_event event;
	struct epoll_event ep_events[EPOLL_SIZE];
	int epfd, event_cnt;

	epfd = epoll_create1(0);
	if(epfd == -1){
		perror("create epoll error:");
		exit(-1);
	}

	event.events = EPOLLIN;
	event.data.fd = termServSock;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, termServSock, &event) == -1){
		perror("add termServSock to epoll error:");
		exit(-1);
	}
	printf("termServSock is waiting for connect\n");

	int i, str_len;	//for循环用
	char buf[50];
	int clnt_sock;	//存放链接上来的客户端Socket句柄
	while(1){
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if(event_cnt == -1){
			puts("epoll_wait() error");
			continue;
		}

		for(i=0; i<event_cnt; i++){
			//服务端接收到新的连接
			if(ep_events[i].data.fd == termServSock){
				clnt_sock=accept(termServSock, NULL, NULL);
				if(clnt_sock == -1){
					perror("termServSock accpet error:");
					continue;
				}
				event.events = EPOLLIN;
				event.data.fd = clnt_sock;
				if(epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event)==-1){
					perror("add clnt_sock to epoll error:");
					exit(-1);
				}
				printf("connected client: %d \n",clnt_sock);
			}else{
				//clnt_sock接收到数据
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
	close(termServSock);
	close(epfd);
}
