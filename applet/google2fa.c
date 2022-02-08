#include "../xenobox.h"
#include "../lib/libmshlsplash.h"
#include "../lib/sha1.h"

static const char *t="ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static int base32_decode(memstream *in,memstream *out){
	int b=0,c;
	u32 x=0;
	char *p;
	for(;~(c=mgetc(in));){
		if(c=='='){
			break;
		}
		if(between('a',c,'z'))c-=0x20;
		if(p=strchr(t,c)){
			x=(x<<5)+(p-t);
			b+=5;
			if(b>=8)b-=8,mputc((x>>b)&0xff,out);
		}
	}
	while(b>=8)b-=8,mputc((x>>b)&0xff,out);
	return 0;
}

#define HMAC_SHA1_DIGESTSIZE 20
#define HMAC_SHA1_BLOCKSIZE  64

//out must have HMAC_SHA1_DIGESTSIZE bytes.
static void hmac_sha1(const u8 *key, int lkey, const u8 *data, int ldata, u8 *out){
	u8 key2[HMAC_SHA1_DIGESTSIZE];
	u8 tmp_digest[HMAC_SHA1_DIGESTSIZE];
	u8 buf[HMAC_SHA1_BLOCKSIZE];
	int i;
	struct sha1_ctxt ctx;

	//truncate
	if(lkey>HMAC_SHA1_BLOCKSIZE){
		sha1_init(&ctx);
		sha1_loop(&ctx,key,lkey);
		sha1_result(&ctx,key2);
		key = key2;
		lkey = HMAC_SHA1_DIGESTSIZE;
	}

	//stage1
	for(i=0;i<lkey;i++)buf[i]=key[i]^0x36;
	for(;i<HMAC_SHA1_BLOCKSIZE;i++)buf[i]=0x36;
	sha1_init(&ctx);
	sha1_loop(&ctx,buf,HMAC_SHA1_BLOCKSIZE);
	sha1_loop(&ctx,data,ldata);
	sha1_result(&ctx,tmp_digest);

	//stage2
	for(i=0;i<lkey;i++)buf[i]=key[i]^0x5c;
	for(;i<HMAC_SHA1_BLOCKSIZE;i++)buf[i]=0x5c;
	sha1_init(&ctx);
	sha1_loop(&ctx,buf,HMAC_SHA1_BLOCKSIZE);
	sha1_loop(&ctx,tmp_digest,HMAC_SHA1_DIGESTSIZE);
	sha1_result(&ctx,out);
}

static char secret[1024];
int google2fa(const int argc, const char **argv){
	if(argc<2){
		fprintf(stderr,
			"google2fa - Google TwoFactor Authenticator\n"
			"Token will be updated every 30sec.\n"
			"PLEASE MAKE SURE YOUR CLOCK IS CORRECT.\n"
			"Usage:\n"
			"google2fa secret_key(spaces will be ignored)\n"
			"google2fa keys.txt index(beginning with 1)\n"
		);return 1;
	}
	if(argc>2){
		FILE *f=fopen(argv[1],"rb");
		if(!f){fprintf(stderr,"cannot open %s\n",argv[1]);return 1;}
		int i=1;for(;i<strtol(argv[2],NULL,0);i++)myfgets(secret,1024,f);
		if(!myfgets(secret,1024,f)){
			fclose(f);fprintf(stderr,"index too large\n");return 1;
		}
		fclose(f);
	}else{
		strncpy(secret,argv[1],1023);
	}
	if(strlen(secret)>128){fprintf(stderr,"secret_key max 128bytes (might be programmer's fault?\n");return 1;}
	u8 key[80];
	memstream in,out;
	mopen(secret,strlen(secret),&in),mopen(key,80,&out),
	base32_decode(&in,&out);

	time_t timer_old=0,timer;
	struct tm *pt;

	u8 T[8],hash[20];
	int otp=0;
	for(;;){ //This program is meant to be terminated with Ctrl+C.
		for(;;msleep(200)){
			time(&timer);
			pt=localtime(&timer);
			if((pt->tm_sec%30==0||timer_old==0)&&timer!=timer_old){timer_old=timer;break;}
			fprintf(stderr,"%02d:%02d:%02d\r",pt->tm_hour,pt->tm_min,pt->tm_sec);
		}
		u64 TIMER=(u64)timer/30;
		memcpy(T,&TIMER,8);
		{ //fix endian to little
			u8 z;
			z=T[0],T[0]=T[7],T[7]=z;
			z=T[1],T[1]=T[6],T[6]=z;
			z=T[2],T[2]=T[5],T[5]=z;
			z=T[3],T[3]=T[4],T[4]=z;
		}
		hmac_sha1(key,mtell(&out),T,8,hash);
		int offset=hash[19]&0xf;
		otp=( ((hash[offset]&0x7f)<<24) | (hash[offset+1]<<16) | (hash[offset+2]<<8) | hash[offset+3] )%1000000;
		fprintf(stderr,"%02d:%02d:%02d %06d\r",pt->tm_hour,pt->tm_min,pt->tm_sec,otp);
	}
exit(0);}
