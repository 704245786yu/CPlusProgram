#ifndef CODETRANSFORM_H_
#define CODETRANSFORM_H_

/*大字端转long*/
long bigEndian2long(const unsigned char buf[],int len);
/*long转大字端*/
void long2bigEndian(unsigned char *const destBuf, unsigned long val);
/*大字端转int*/
int bigEndian2int(const unsigned char buf[],int len);
/*按16进制打印字节数组*/
void printHexBytes(char prompt[], unsigned char bytes[], int len);
#endif /* CODETRANSFORM_H_ */
