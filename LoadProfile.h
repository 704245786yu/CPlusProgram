#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef LOADPROFILE_H_
#define LOADPROFILE_H_

#define PROFILE_NAME "profile.ini"
#define PARAM_STR_LENGTH 30 //文件中配置属性名、配置属性值的字符串长度

/* @paramName[] 要获取的配置属性名
 * @paramValue[] 存放获取的配置属性值，作为输出参数
 * @paramNum 配置属性的个数
 * */
void getConfigVal(const char * const paramName[], char *paramValue[], int paramNum);

#endif /* LOADPROFILE_H_ */
