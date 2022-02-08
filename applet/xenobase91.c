#include "../xenobox.h"

static const char *t="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&()*+,./:;<=>?@[]^_`{|}~\"";

static int base91_encode(){
	int i=0,j=0,b=0,c;
	u32 x=0;
	for(;~(c=fgetc(stdin));){
		x=c<<b|x;
		i++;
		b+=8;
		if(b>13){
			int shift=14;
			int out=x&0x1fff;
			if(out>=89)
				shift=13;
			else
				out=x&0x3fff;
			x>>=shift;
			b-=shift;
			fputc(t[(out%91)],stdout),j++;
			fputc(t[(out/91)],stdout),j++;
			if(j==78){fputc('\n',stdout);j=0;}
		}
	}
	if(b)fputc(t[(x%91)],stdout),j++;
	if(b>7||x>=91)fputc(t[(x/91)],stdout),j++;
	if(j)fputc('\n',stdout);
	return 0;
}

static inline int base91dec_fgetc(int *n,FILE *f){
	int c;
	char *p;
	for(;~(c=fgetc(f));)
		if(p=strchr(t,c)){*n=p-t;break;}
	if(c==EOF)return 0;
	for(;~(c=fgetc(f));)
		if(p=strchr(t,c)){*n=*n+(p-t)*91;break;}
	if(c==EOF)return 1;
	return 2;
}

//should be faster than original.
static int base91_decode(){
	int b=0,c,len;
	u32 x=0;
	for(;(len=base91dec_fgetc(&c,stdin))==2;){
		x=c<<b|x;
		b+=(c&0x1fff)>=89?13:14;
		while(b>=8)b-=8,fputc(x&0xff,stdout),x>>=8;
	}
	if(len)fputc((c<<b|x)&0xff,stdout);
	return 0;
}

//note:
//base91 enc/dec requires 32bit CPU.
//base91's bit stream is opposite from base64/32/etc.
//base64: input to lower, output from higher
//base91: input to higher, output from lower
int xenobase91(const int argc, const char **argv){
	if(argc>1)return base91_decode();
	else return base91_encode();
	//return 0;
}
