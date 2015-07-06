#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Socket.h"

#ifndef BIZSERVICE_H_
#define BIZSERVICE_H_

#define MAX_RECVBUFF 4096

/*开启业务Server端口*/
void getBizServSock(short bizServPort);
/*业务线程程序*/
void* bizThreadRoutine(void* arg);

#endif /* BIZSERVICE_H_ */
