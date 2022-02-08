#include "../xenobox.h"

static const char *t1="!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstu";
static const char *t2="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";
static int C;
static void PUT(int c){fputc(c,stdout);C++;if(C==79)fputc('\n',stdout),C=0;}
static inline int MOD(unsigned int x,int p){int i=0;for(;i<p;i++)x/=85;return x%85;}

static inline int base85enc_fgetc(unsigned int *n,FILE *f,const char *t){
	*n=0;
	int c=0,i;
	for(i=0;i<4;i++){
		if((c=fgetc(f))==EOF)break;
		*n=((*n)<<8)+c;
	}
	{int j=i;for(;j<4;j++)*n<<=8;}
	return i;
}

static int base85_encode(const char *t){
	C=0;
	unsigned int c;
	int len,i;
	for(;len=base85enc_fgetc(&c,stdin,t);){
		if(t==t1){ //compression
			if(c==0x20202020){PUT('y');continue;}
			if(c==0x00000000){PUT('z');continue;}
		}
		for(i=0;i<len+1;i++){
			PUT(t[MOD(c,/*len+1*/4-i)]);
		}
	}
	return 0;
}

static inline int base85dec_fgetc(unsigned int *n,FILE *f,const char *t){
	int c,i;
	char *p;
	for(;~(c=fgetc(f));){
		if(p=strchr(t,c)){*n=p-t;break;}
		else if(c=='y'){*n=0x20202020;return 5;} //will be true only if t==t1
		else if(c=='z'){*n=0x00000000;return 5;}
	}
	if(c==EOF)return 0;
	for(i=1;~(c=fgetc(f));){
		if(p=strchr(t,c)){*n=*n*85+(p-t);i++;if(i==5)break;}
		else if(c=='y'||c=='z'){fprintf(stderr,"data corrupted\n");exit(1);} //will be true only if t==t1
	}
	//if(i==1){fprintf(stderr,"data truncated\n");exit(1);}
	{int j=i;for(;j<5;j++)*n=*n*85+84;} //padded with 'u'
	if(c==EOF)return i;
	return 5;
}

//should be faster than original.
static int base85_decode(const char *t){
	//C=0;
	unsigned int c;
	int i,len;
	for(;len=base85dec_fgetc(&c,stdin,t);){
		for(i=0;i<len-1;i++)fputc((c>>(8*(/*len-1*/4-i-1)))&0xff,stdout);
	}
	return 0;
}

//note:
//base85 enc/dec requires 32bit CPU.
int xenobase85(const int argc, const char **argv){
	if(argc>1)return base85_decode(t1);
	else return base85_encode(t1);
	//return 0;
}
int xenobase85rfc(const int argc, const char **argv){
	if(argc>1)return base85_decode(t2);
	else return base85_encode(t2);
	//return 0;
}
