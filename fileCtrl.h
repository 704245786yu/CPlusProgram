#include <fcntl.h>

#ifndef FILECTRL_H_
#define FILECTRL_H_

/*设置文件状态标志*/
void set_fl(int fd, int flags);
/*取消设置文件状态标志*/
void cancel_fl(int fd, int flags);

#endif /* FILECTRL_H_ */
