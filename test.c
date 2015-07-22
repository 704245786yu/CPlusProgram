#include <stdio.h>
#include "CodeTransform.h"
#include "ProtocolAnalysis.h"
#include <netinet/in.h>

void testBigEndian2long();
void testLong2bigEndian();
void testSz2Sh();
void testEndian();

int main1(void)
{
//	testSz2Sh();
//	testEndian();
	testLong2bigEndian();
	return 0;
}

void testBigEndian2long()
{
	unsigned char a[]={0xDC, 0x41, 0x22, 0x34, 0xCE};
	printf("%ld\n",bigEndian2long(a,5));
}

void testLong2bigEndian()
{
	unsigned long val = 945985565902;
	unsigned char buf[8]={0};
	long2bigEndian(buf,val);
	printHexBytes("buf:",buf, 8);
}

void testSz2Sh()
{
//	unsigned char a[]={0xDC, 0x41, 0x22, 0x34, 0xCE};
//	Sz2Sh(a, 5, 0xFF11);
}

/*判断主机字节序*/
void testEndian()
{
	union{
		short s;
		char c[sizeof(short)];
	} un;
	un.s = 0x0102;
	if(sizeof(short)==2){
		if(un.c[0] == 1 && un.c[1] == 2)
			printf("big-endian\n");
		else if(un.c[0] == 2 && un.c[1] == 1)
			printf("little-endian\n");
		else
			printf("unknow\n");
	}else{
		printf("sizeof(short) = %ld\n", sizeof(short));
	}
	un.s = htons(un.s);
	printf("0x%x\n",un.s);
}
