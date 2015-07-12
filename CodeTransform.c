/*编码转换*/
#include "CodeTransform.h"
#include <stdio.h>

/*大字端转long*/
long bigEndian2long(const unsigned char buf[],int len)
{
	long res = 0;
	int i;
	for(i=0;i<len;i++)
	{
		res = res<< 8 | buf[i];
	}
	return res;
}

void printHexBytes(char prompt[], unsigned char bytes[], int len)
{
	int i=0;
	printf("%s", prompt);
	for(; i<len; i++){
		printf("%x ", bytes[i]);
	}
	printf("\n");
}
