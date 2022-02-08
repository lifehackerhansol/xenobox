#include "../../xenobox.h"

int compute_sysvsum(FILE *f, const char *name, const char *desired){
	u32 crc=0;
	int i;
	if(f){
		int readlen;
		for(;(readlen=fread(buf,1,BUFLEN,f))>0;){
			for(i=0;i<readlen;i++){
				crc+=buf[i];
			}
		}
	}else{
		for(i=0;i<strlen(name);i++){
			crc+=(unsigned char)name[i];
		}
	}
	u32 x = (crc&0xffff) + (crc>>16);
	u32 y = (x&0xffff) + (x>>16);

	if(desired)
		printf("%s: %s\n",name,strtoul(desired,NULL,16)==y?"OK":"NG");
	else
		printf("%04x  %s\n",y,name);
	return 0;
}
