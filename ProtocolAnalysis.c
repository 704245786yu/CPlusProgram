#include <string.h>
#include "ProtocolAnalysis.h"
#include <stdio.h>
#include <netinet/in.h>
#include "CodeTransform.h"

unsigned short crc_ta[16]={
		0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
		0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef
};

/*美标CRC16算法*/
static unsigned short crc16(unsigned char *ptr, unsigned char len);

//{0xDC, 0x41, 0x22, 0x34, 0xCE}
/*顺舟转上海协议
 * @buf[] 顺舟协议
 * @len 协议长度
 * @shCmd 上海协议命令码
 * */
void Sz2Sh(unsigned char source[], int sourceLen, unsigned short shCmd,
		unsigned char target[], int *size, int maxLen)
{
	struct shProtocal frame;	//上海协议数据帧
	memset(&frame, 0, sizeof(struct shProtocal));
	frame.head = 0xAA;

	switch (shCmd) {
		case 0xFF11:	//注册包
			memcpy(frame.concentrator+3, source,sourceLen);
			frame.cmd[0]=0xFF;
			frame.cmd[1]=0x11;
			frame.answerCode = 0;
			frame.length_u.len = htonl(sourceLen);
//			printf("s:%d e:%d\n", sourceLen, frame.length_u.len);
			frame.pData = source;
			break;
		default:
			break;
	}
	target[0] = frame.head;
	memcpy(target+1, frame.concentrator, 8);
	memcpy(target+9, frame.cmd, 2);
	target[11] = frame.answerCode;
	memcpy(target+12, frame.length_u.lenBytes, 4);
	memcpy(target+16, frame.pData, 5);
	//获取CRC16校验码
	frame.checkCode_u.checkCode = htons(crc16(target, 16+5));
	frame.tail = 0x55;
	memcpy(target+16+5, frame.checkCode_u.checkCodeBytes, 2);
	target[16+5+2] = frame.tail;
	*size = 16+5+2+1;
}

/*美标CRC16算法*/
static unsigned short crc16(unsigned char *ptr, unsigned char len)
{
	unsigned short crc;
	unsigned char da;
	crc=0;
	while(len--!=0)
	{
		da=crc>>12;
		crc<<=4;
		crc^=crc_ta[da^(*ptr/16)];
		da=crc>>12;
		crc<<=4;
		crc^=crc_ta[da^(*ptr&0x0f)];
		ptr++;
	}
	return crc;
}
