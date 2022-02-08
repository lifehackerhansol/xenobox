#include "../../xenobox.h"

int compute_moonguid(FILE *f, const char *name, const char *desired){
	u32 crc=0;
	int i;
	if(f){
		int readlen;
		for(;(readlen=fread(buf,1,BUFLEN,f))>0;){
			if(readlen<BUFLEN)memset(buf+readlen,0,BUFLEN-readlen);
			for(i=0;i<readlen;i+=4){
				crc=lrotl(crc,1)^read32(buf+i);
			}
			if(readlen<BUFLEN)break;
		}
	}else{
		for(i=0;i<strlen(name);i+=4){
			u32 x=0;
			if(strlen(name)-i>3)x=read32(name+i);
			else if(strlen(name)-i==3)x=read24(name+i);
			else if(strlen(name)-i==2)x=read16(name+i);
			else /*if(strlen(name)-i==1)*/x=((unsigned char*)name)[i];
			crc=lrotl(crc,1)^x;
		}
	}
	if(desired)
		printf("%s: %s\n",name,strtoul(desired,NULL,16)==crc?"OK":"NG");
	else
		printf("%08x  %s\n",crc,name);
	return 0;
}
