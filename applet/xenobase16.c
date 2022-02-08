#include "../xenobox.h"

static const char *t="0123456789abcdef";

static int base16_encode(){
	int i=0,j=0,b=0,c;
	u32 x=0;
	for(;~(c=fgetc(stdin));){
		x=(x<<8)+c;
		i++;
		b+=8;
		//while(b>=4)
		b-=4,fputc(t[(x>>b)&0xf],stdout),j++;
		b-=4,fputc(t[(x>>b)&0xf],stdout),j++;
		if(j==76){fputc('\n',stdout);j=0;}
	}
	//if(b)fputc(t[(x<<(4-b))&0xf],stdout),j++; //shouldn't happen :p
	if(j)fputc('\n',stdout);
	return 0;
}

static int base16_decode(){
	int b=0,c;
	u32 x=0;
	char *p;
	for(;~(c=fgetc(stdin));){
		//if(c=='='){
		//	break;
		//}
		if(between('A',c,'F'))c+=0x20;
		if(p=strchr(t,c)){
			x=(x<<4)+(p-t);
			b+=4;
			if(b>=8)b-=8,fputc((x>>b)&0xff,stdout);
		}
	}
	while(b>=8)b-=8,fputc((x>>b)&0xff,stdout);
	return 0;
}

int xenobase16(const int argc, const char **argv){
	if(argc>1)return base16_decode();
	else return base16_encode();
	//return 0;
}

#if 0
static int binary_encode(){
	int c;
	for(;~(c=fgetc(stdin));){
		unsigned char d=(unsigned char)c;
		int x=d>>4,y=d&0xf;
		fputc(x>9?(x-10+'a'):(x+'0'),stdout);
		fputc(y>9?(y-10+'a'):(y+'0'),stdout);
	}
	fputc('\n',stdout);
	return 0;
}

static int binary_decode(){
	int i=0,src0=0,src1=0,c;
	for(;~(c=fgetc(stdin));){
		if(!i){
			src0=c;
			if(src0==EOF)break;
			//if(!src0||src0==' '||src0=='\t'||src0=='\r'||src0=='\n'||src0=='#'||src0==';'||src0=='\''||src0=='"')break;
			if(0x60<src0&&src0<0x7b)src0-=0x20;
			if(!( isdigit(src0)||(0x40<src0&&src0<0x47) ))continue;//{fprintf(stderr,"Invalid character %c\n",src0);exit(-1);}
			src0=isdigit(src0)?(src0-'0'):(src0-55);
		}else{
			src1=c;
			if(src1==EOF){fprintf(stderr,"File truncated\n");return 1;}
			if(0x60<src1&&src1<0x7b)src1-=0x20;
			if(!( isdigit(src1)||(0x40<src1&&src1<0x47) ))continue;//{fprintf(stderr,"Invalid character %c\n",src1);exit(-1);}
			src1=isdigit(src1)?(src1-'0'):(src1-55);
			fputc((src0<<4)|src1,stdout);
			//fprintf(stderr,"%02X",dst[i]);
		}
		i^=1;
	}
	return 0;
}

int tobinary(const int argc, const char **argv){
	if(argc>1)return binary_decode();
	else return binary_encode();
	//return 0;
}
#endif
