#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#ifndef SOCKET_H_
#define SOCKET_H_

/*出错返回-1，成功返回套接子文件描述符listenfd*/
int initServSock(short servPort);

#endif /* SOCKET_H_ */
