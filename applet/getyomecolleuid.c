#include "../xenobox.h"
#include "../lib/rijndael.h"

#define KEYBITS 128

int getyomecolleuid(const int argc, const char **argv){
	if(isatty(fileno(stdin))&&argc<2){
		fprintf(stderr,
			"[IMEI->uid]\n"
			"getyomecolleuid [IMEI]\n"
			"[uid->IMEI]\n"
			"echo [uid]|xenobase16 -d|getyomecolleuid|fcutdown - 16\n"
		);return -1;
	}
	unsigned int rk[RKLENGTH(KEYBITS)];
	//unsigned char key[KEYLENGTH(KEYBITS)];
	u8 tmp[16];
	int i;
	int nrounds;

	int c=0;
	const char *arg=(const char*)argv[1];

	/// warning: these keys are concealed ///
	if(argc<2){ //decrypt
		nrounds = rijndaelSetupDecrypt(rk, (u8*)"neBIG08-08-21#AP", KEYBITS);
	}else{
		nrounds = rijndaelSetupEncrypt(rk, (u8*)"neBIG08-08-21#AP", KEYBITS);
	}
	u8 iv[16];
	memcpy(iv,(u8*)"NEBIGVoice08Zero",16);

	if(argc<2){ //decrypt
		fread(buf,1,16,stdin);
		for(;;){
			int readlen=fread(buf+16,1,16,stdin);
			if(readlen<0)break;
			if(readlen==0){ //final block. process padding
				memcpy(tmp,buf+16,16);
				rijndaelDecrypt(rk, nrounds, buf, buf);
				for(i=0;i<16;i++)buf[i]^=iv[i];
				//memcpy(iv,tmp,16);
				//fprintf(stderr,"%d padding %d\n",readlen,buf[15]);
				u8 c=buf[15];
				fwrite(buf,1,16-(c<17?c:0),stdout); //lol...
				break;
			}
			memcpy(tmp,buf,16);
			rijndaelDecrypt(rk, nrounds, buf, buf);
			for(i=0;i<16;i++)buf[i]^=iv[i];
			fwrite(buf,1,readlen,stdout);
			if(readlen<16)break; //error: truncated.
			memcpy(iv,tmp,16);
			memcpy(buf,buf+16,16);
		}
		if(isatty(fileno(stdout)))puts("");
	}else{
		memcpy(buf,"0000000000000000",16);
		for(i=0;i<16;i++)buf[i]^=iv[i];
		rijndaelEncrypt(rk, nrounds, buf, buf);
		for(i=0;i<16;i++){
			int x=buf[i]>>4,y=buf[i]&0xf;
			fputc(x<10?(x+'0'):(x-10+'a'),stdout);
			fputc(y<10?(y+'0'):(y-10+'a'),stdout);
		}
		memcpy(iv,buf,16);

		for(c=0;;c+=16){
			int readlen=strlen(arg+c);
			if(readlen>16)readlen=16;
			memcpy(buf,arg+c,readlen);
			//if(readlen<0)break;
			if(readlen<16){ //process PKCS7 padding
				if(readlen==0)readlen=16;
				memset(buf+(readlen==16?0:readlen),16-readlen,16-readlen);
				for(i=0;i<16;i++)buf[i]^=iv[i];
				rijndaelEncrypt(rk, nrounds, buf, buf);
				for(i=0;i<16;i++){
					int x=buf[i]>>4,y=buf[i]&0xf;
					fputc(x<10?(x+'0'):(x-10+'a'),stdout);
					fputc(y<10?(y+'0'):(y-10+'a'),stdout);
				}
				break;
			}
			for(i=0;i<16;i++)buf[i]^=iv[i];
			rijndaelEncrypt(rk, nrounds, buf, buf);
			for(i=0;i<16;i++){
				int x=buf[i]>>4,y=buf[i]&0xf;
				fputc(x<10?(x+'0'):(x-10+'a'),stdout);
				fputc(y<10?(y+'0'):(y-10+'a'),stdout);
			}
			memcpy(iv,buf,16);
		}
		if(isatty(fileno(stdout)))puts("");
	}
	return 0;
}
