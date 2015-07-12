#ifndef PROTOCALANALYSIS_H_
#define PROTOCALANALYSIS_H_

/*上海协议数据帧*/
struct shProtocal{
	unsigned char head;	//帧起始符，固定值为0xAA
	unsigned char concentrator[8];	//集中控制器地址，实际存放注册包
	unsigned char cmd[2];	//命令码
	unsigned char answerCode;	//响应码，默认为0
	union{
		int len;
		unsigned char lenBytes[4];
	} length_u;	//有效数据的长度
	unsigned char *pData;	//数据
	union{
		unsigned short checkCode;
		unsigned char checkCodeBytes[2];
	} checkCode_u;	//校验码，用美标CRC16校验，从帧起始符到数据域内的所有数据的校验
	unsigned char tail;	//帧结束符，固定值位0x55
};

/*顺舟转上海协议
 * @buf[] 顺舟协议
 * @len 协议长度
 * @shCmd 上海协议命令码
 * */
void Sz2Sh(unsigned char source[], int sourceLen, unsigned short shCmd,
		unsigned char target[], int *size, int maxLen);

#endif /* PROTOCALANALYSIS_H_ */
