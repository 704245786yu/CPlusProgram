#ifndef EPOLLSERVICE_H_
#define EPOLLSERVICE_H_

void epollService(short termServPort);
int find_concentrator(unsigned long concentrator);

typedef struct{
	unsigned long concentratro;
	int sock_fd;
	char timeout;	//超时记录，超时15分钟算断线
} concentrator_struct;

#endif /* EPOLLSERVICE_H_ */
