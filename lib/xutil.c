#include "../xenobox.h"

unsigned long long int read64(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8)|(x[2]<<16)|(x[3]<<24)|( (unsigned long long int)(x[4]|(x[5]<<8)|(x[6]<<16)|(x[7]<<24)) <<32);
}

unsigned int read32(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8)|(x[2]<<16)|(x[3]<<24);
}

unsigned int read24(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8)|(x[2]<<16);
}

unsigned short read16(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8);
}

void write64(void *p, const unsigned long long int n){
	unsigned char *x=(unsigned char*)p;
	x[0]=n&0xff,x[1]=(n>>8)&0xff,x[2]=(n>>16)&0xff,x[3]=(n>>24)&0xff,
	x[4]=(n>>32)&0xff,x[5]=(n>>40)&0xff,x[6]=(n>>48)&0xff,x[7]=(n>>56)&0xff;
}

void write32(void *p, const unsigned int n){
	unsigned char *x=(unsigned char*)p;
	x[0]=n&0xff,x[1]=(n>>8)&0xff,x[2]=(n>>16)&0xff,x[3]=(n>>24)&0xff;
}

void write24(void *p, const unsigned int n){
	unsigned char *x=(unsigned char*)p;
	x[0]=n&0xff,x[1]=(n>>8)&0xff,x[2]=(n>>16)&0xff;
}

void write16(void *p, const unsigned short n){
	unsigned char *x=(unsigned char*)p;
	x[0]=n&0xff,x[1]=(n>>8)&0xff;
}

unsigned int read32be(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[3]|(x[2]<<8)|(x[1]<<16)|(x[0]<<24);
}

unsigned int read24be(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[2]|(x[1]<<8)|(x[0]<<16);
}

unsigned short read16be(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[1]|(x[0]<<8);
}

void write32be(void *p, const unsigned int n){
	unsigned char *x=(unsigned char*)p;
	x[3]=n&0xff,x[2]=(n>>8)&0xff,x[1]=(n>>16)&0xff,x[0]=(n>>24)&0xff;
}

void write24be(void *p, const unsigned int n){
	unsigned char *x=(unsigned char*)p;
	x[2]=n&0xff,x[1]=(n>>8)&0xff,x[0]=(n>>16)&0xff;
}

void write16be(void *p, const unsigned short n){
	unsigned char *x=(unsigned char*)p;
	x[1]=n&0xff,x[0]=(n>>8)&0xff;
}

char* myfgets(char *buf,int n,FILE *fp){ //accepts LF/CRLF
	char *ret=fgets(buf,n,fp);
	if(!ret)return NULL;
	if(strlen(buf)&&buf[strlen(buf)-1]=='\n')buf[strlen(buf)-1]=0;
	if(strlen(buf)&&buf[strlen(buf)-1]=='\r')buf[strlen(buf)-1]=0;
	return ret;
}

void msleep(int msec){
#if defined(WIN32)
	Sleep(msec);
#elif !defined(FEOS)
	if(msec>999)sleep(msec/1000);
	if(msec%1000)usleep(msec%1000*1000);
#endif
}

int strchrindex(const char *s, const int c, const int idx){
	const char *ret=strchr(s+idx,c);
	if(!ret)return -1;
	return ret-s;
}

size_t _FAT_directory_mbstoucs2(unsigned short* dst, const unsigned char* src, size_t len){
	size_t i=0,j=0;
	for(;src[i];){
		if((src[i]&0x80) == 0x00){
			if(!dst)j++;else{
				if(len-j<2)break;
				dst[j++] = ((src[i  ] & 0x7f)     );
			}
			i++;
		}else if ((src[i] & 0xe0) == 0xc0 ){
			if(!dst)j++;else{
				if(len-j<2)break;
				dst[j++] = ((src[i  ] & 0x3f) << 6)
					  | ((src[i+1] & 0x3f)     );
			}
			i+=2;
		}else if ((src[i] & 0xf0) == 0xe0 ){
			if(!dst)j++;else{
				if(len-j<2)break;
				dst[j++] = ((src[i  ] & 0x0f) << 12)
					  | ((src[i+1] & 0x3f) <<  6)
					  | ((src[i+2] & 0x3f)      );
			}
			i+=3;
		}else if ((src[i] & 0xf8) == 0xf0 ){
			if(!dst)j+=2;else{
				unsigned short z = ((src[i  ] & (unsigned short)0x03) <<  8)  // 2
						   | ((src[i+1] & 0x3f) <<  2)  // 6
						   | ((src[i+2] & 0x30) >>  4); // 2
				if(len-j<2)break;
				dst[j++] = (z-0x40) | 0xd800;
				dst[j++] = ((src[i+2] & 0x0f) <<  6)           //4
					  | ((src[i+3] & 0x3f)      ) | 0xdc00; //6
			}
			i+=4;
		}else break; //cannot convert
	}
	if(dst)dst[j]=0;
	return j;
}

u32 mbstoucs2(unsigned short* dst, const unsigned char* src){
	return _FAT_directory_mbstoucs2(dst,src,256);
}

size_t _FAT_directory_ucs2tombs(unsigned char* dst, const unsigned short* src, size_t len){
	size_t i=0,j=0;
	for (;src[i];i++){
		if(src[i] <= 0x007f){
			if(!dst)j++;else{
				if(len-j<2)break;
				dst[j++] = ((src[i] & 0x007f)      );
			}
		}else if(src[i] <= 0x07ff){
			if(!dst)j+=2;else{
				if(len-j<3)break;
				dst[j++] = ((src[i] & 0x07c0) >>  6) | 0xc0;
				dst[j++] = ((src[i] & 0x003f)      ) | 0x80;
			}
		}else if((src[i] & 0xdc00) == 0xd800 && (src[i+1] & 0xdc00) == 0xdc00){
			if(!dst)j+=4;else{
				unsigned short z = (src[i]&0x3ff)+0x40;
				if(len-j<5)break;
				dst[j++] = ((z      & 0x0300) >>  8) | 0xf0;   //2
				dst[j++] = ((z      & 0x00fc) >>  2) | 0x80;   //6
				dst[j++] = ((z      & 0x0003) <<  4)           //2
					  | ((src[i+1] & 0x03c0) >>  6) | 0x80; //4
				dst[j++] = ((src[i+1] & 0x003f)      ) | 0x80; //6
			}i++;
		}else{
			if(!dst)j+=3;else{
				if(len-j<4)break;
				dst[j++] = ((src[i] & 0xf000) >> 12) | 0xe0;
				dst[j++] = ((src[i] & 0x0fc0) >>  6) | 0x80;
				dst[j++] = ((src[i] & 0x003f)      ) | 0x80;
			}
		}
	}
	if(dst)dst[j]=0;
	return j;
}

u32 ucs2tombs(unsigned char* dst, const unsigned short* src){
	return _FAT_directory_ucs2tombs(dst,src,768);
}

void NullMemory(void* buf, unsigned int n){
        while(n)((char*)buf)[--n]=0;
}

int memcmp_fast(const void *x,const void *y,unsigned int n){
	int v=0;
	unsigned char *X=(unsigned char*)x;
	unsigned char *Y=(unsigned char*)y;
	for(;n--&&!v;)v=*X++-*Y++;
	return v;
}
