#include "Socket.h"

/*出错返回-1，成功返回套接子文件描述符listenfd*/
int initServSock(short servPort)
{
	struct sockaddr_in servaddr;

	int listenfd;
	if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket() error:");
		return -1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(servPort);

	/*下面两句作用是防止服务端断开后,无法立刻使用端口,从而出现TIME_WAIT*/
	int opt =1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
		perror("bind() error:");
		return -1;
	}

	if(listen(listenfd, 20) == -1){
		perror("listen() error:");
		return -1;
	}
	printf("Server start,wait for connect...\n");
	return listenfd;
}
