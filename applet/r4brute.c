#include "../xenobox.h"

static void r4_crypt(int k,int fcrypt,int n,unsigned char *p){
	unsigned char xor;
	unsigned short key=n^k;
	unsigned int i=0,j,x,y;
	for(;i<0x10;i++){ //0x200
		xor=(key&3)|((key>>4)&0x0c)|((key>>5)&0x10)|((key>>6)&0x60)|((key>>7)&0x80);
		if(fcrypt)p[i]^=xor;
		x=y=((p[i]<<8)^key)<<16;
		if(!fcrypt)p[i]^=xor;
		for(j=1;j<32;j++)x^=y>>j;
		key =(x>>25)&0x0003;
		key^=(x>>22)&0x0300;
		key^=(x>> 8)&0x8000;
		key^=(y>>24)&0x0003;
		key^=(y>>23)&0x00fc;
		key^=(y>>22)&0x00fc;
		key^=(y>> 8)&0x7f00;
	}
}

static void r4_process(int fcrypt,FILE *in){
	unsigned char p[512],q[512];
	//int r,n=0;
	fread(p,1,512,in);
	int key=0;
	for(;key<0x10000;key++){
		memcpy(q,p,512);
		r4_crypt(key,fcrypt,0,q);
		if(!memcmp("\x2E\0\0\xEA\0\0\0\0\0\0\0\0####",q,16))printf("%04x\n",key);
	}
}

int r4brute(const int argc, const char **argv){
	if(argc==2){
		FILE *in=fopen(argv[1],"rb");
		if(!in){fprintf(stderr,"cannot open %s\n",argv[1]);return 1;}
		r4_process(0,in);
		fclose(in);
		return 0;
	}

	fprintf(stderr,"r4brute _DS_MENU.DAT\n");
	return 1;
}
