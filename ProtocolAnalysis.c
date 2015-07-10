#include <string.h>
#include "ProtocolAnalysis.h"

/*顺舟转上海协议
 * @buf[] 顺舟协议
 * @len 协议长度
 * @shCmd 上海协议命令码
 * */
void Sz2Sh(unsigned char buf[], int len, unsigned short shCmd)
{
	struct shProtocal frame;	//上海协议数据帧
	frame.head = 0xAA;

	switch (shCmd) {
		case 0xFF11:	//注册包
			memcpy(frame.concentrator+3, buf,len);
			frame.cmd[0]=0xFF;
			frame.cmd[1]=0x11;
			frame.answerCode = 0;
			break;
		default:
			break;
	}
}
