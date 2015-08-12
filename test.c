#include <stdio.h>
#include "CodeTransform.h"
#include "ProtocolAnalysis.h"
#include <netinet/in.h>

void testBigEndian2long();
void testLong2bigEndian();
void testSz2Sh();
void testEndian();
void testBinarayInsert();

int main1(void)
{
//	testSz2Sh();
//	testEndian();
//	testLong2bigEndian();
	testBinarayInsert();
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

void binarayInsertSort(int a[])
{
	int low = 0;
	int high = 0;
	int mid = 0;
	int current = 0;
	int i;
	for (i = 1; i < 10; i++)
	{
		current = a[i];
		low = 0;
		high = i - 1;
		while (low <= high)
		{
			mid = (low + high) / 2;
			if (a[i] < a[mid])
			{
				high = mid - 1;
			}
			else
			{
				low = mid + 1;
			}
		}
		int j;
		for (j = i - 1; j >= low; j--)
		{
			a[j + 1] = a[j];
		}
		a[low] = current;
	}
}

void printArray(int array[])
{
	printf("ary:");
	int i;
	for (i = 0; i < 10; i++)
		printf("%d ",array[i]);
	printf("\n");
}

void testBinarayInsert()
{
	int a[] = { 7, 2, 1, 6, 13,21, 8, 4, 33, 26 };
	binarayInsertSort(a);
	printArray(a);
}
