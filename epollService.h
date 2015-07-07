#include <sys/epoll.h>

#ifndef EPOLLSERVICE_H_
#define EPOLLSERVICE_H_

#define BUF_SIZE 100
#define EPOLL_SIZE 50

void epollService(short termServPort);

#endif /* EPOLLSERVICE_H_ */
