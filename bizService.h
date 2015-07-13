#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Socket.h"

#ifndef BIZSERVICE_H_
#define BIZSERVICE_H_

#define MAX_RECVBUFF 4096

/*业务线程程序,等待业务网关连接，获得业务端口后，开启业务网关读、写线程*/
void* bizThreadRoutine(void* arg);

#endif /* BIZSERVICE_H_ */
