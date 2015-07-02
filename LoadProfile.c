#include "LoadProfile.h"

/* @paramName[] 要获取的配置属性名
 * @paramValue[] 存放获取的配置属性值，作为输出参数
 * @paramNum 配置属性的个数
 * */
void getConfigVal(const char * const paramName[], char *paramValue[], const int paramNum)
{
	FILE* fp = fopen(PROFILE_NAME, "r");
	if(fp == NULL){
		perror("open profile failure:");
		exit(-1);
	}

	char tempName[PARAM_STR_LENGTH] = {0};
	char tempValue[PARAM_STR_LENGTH] = {0};
	int count=0;	//用于存放已获取的属性值个数
	int j=0;
	int res = 0;
	while( (res = fscanf(fp,"%s %s", tempName, tempValue)) == 2){
		for(j=0; j<paramNum; j++){
			if(strcmp(tempName, paramName[j])==0){
				strcpy(paramValue[j],tempValue);
				count++;
				break;
			}
		}
		memset(tempName,0,sizeof(tempName));
	}
	if(paramNum != count){
		printf("profile content may be error\n");
		exit(-1);
	}
	fclose(fp);
}
