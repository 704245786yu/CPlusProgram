/*24.3 两层指针做传出参数
 * redirect_ptr.c
 * */
#include"redirect_ptr.h"
static const char *msg[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

void get_a_day(const char **pp)
{
	static int i=0;
	*pp = msg[i%7];
	i++;
}
