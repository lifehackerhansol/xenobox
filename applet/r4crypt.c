#include "../xenobox.h"

#define R4_KEY    0x484a
#define R4ILS_KEY 0x4002

static void r4_crypt(int k,int fcrypt,int n,unsigned char *p){
	unsigned char xor;
	unsigned short key=n^k;
	unsigned int i=0,j,x,y;
	for(;i<0x200;i++){
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

static void r4_process(int key,int fcrypt,FILE *in, FILE *out){
	unsigned char p[512];
	int r,n=0,s1=0,s2=filelength(fileno(in));
	for(;r=fread(p,1,512,in);fwrite(p,1,r,out)){
		r4_crypt(key,fcrypt,n++,p);
		s1+=r;
		fprintf(stderr,"%s %8d / %8d\r",fcrypt?"Encrypting":"Decrypting",s1,s2);
	}
	fprintf(stderr,"%s %8d / %8d Done.\n",fcrypt?"Encrypting":"Decrypting",s2,s2);
}

int r4crypt(const int argc, const char **argv){
	int key=R4_KEY;
	if(!strncasecmp("r4ils",argv[0],5))key=R4ILS_KEY;

	if(argc==4){
		char sw=argv[1][0];
		if(sw=='-')sw=argv[1][1];
		if(sw){
			FILE *in=fopen(argv[2],"rb"),*out;
			if(!in){fprintf(stderr,"cannot open %s\n",argv[2]);return 1;}
			out=fopen(argv[3],"wb");
			if(!out){fclose(in);fprintf(stderr,"cannot open %s\n",argv[3]);return 2;}
			r4_process(key,sw-'d',in,out);
			fclose(in);fclose(out);
			return 0;
		}
	}

if(!isatty(fileno(stdin))&&!isatty(fileno(stdout))){
	if(argc==2){
		char sw=argv[1][0];
		if(sw=='-')sw=argv[1][1];
		if(sw){r4_process(key,sw-'d',stdin,stdout);return 0;}
	}
	if(argc==1){
		int sw=0;
		if(!strcasecmp(argv[0],"r4dec"))sw='d';
		if(!strcasecmp(argv[0],"r4ilsdec"))sw='d';
		if(!strcasecmp(argv[0],"r4enc"))sw='e';
		if(!strcasecmp(argv[0],"r4ilsenc"))sw='e';
		if(sw){r4_process(key,sw-'d',stdin,stdout);return 0;}
	}
}

	fprintf(stderr,
		"r4[ils]crypt X\n"
		"r4[ils]dec < in > out\n"
		"r4[ils]enc < in > out\n"
		"r4[ils]crypt [e|d] < in > out\n"
		"r4[ils]crypt [e|d] in out\n"
		"* You can use [-e|-d] instead of [e|d] *\n"
		"\n"
		"Basically, if the first 4 bytes are:\n"
		"88A8177F: use r4crypt.\n"
		"AC07A2A2: use r4ilscrypt.\n"
	);
	return 10;
}
