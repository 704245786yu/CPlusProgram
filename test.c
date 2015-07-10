#include <stdio.h>
#include "CodeTransform.h"

int main1(void)
{
	unsigned char a[]={0xDC, 0x41, 0x22, 0x34, 0xCE};
	printf("%ld\n",bigEndian2long(a,5));
	return 0;
}
