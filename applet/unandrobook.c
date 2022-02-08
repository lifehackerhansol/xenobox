#include "../xenobox.h"
#include "../lib/rijndael.h"

#define KEYBITS 128

int unandrobook(const int argc, const char **argv){
	if(isatty(fileno(stdin))){
		fprintf(stderr,
			"unzip -p FILE.apk/jar assets/setting | unandrobook\n"
			"7z x -so FILE.apk/jar assets/setting 2>/dev/null | unandrobook\n"
			"Then extract corresponding zip data using displayed password.\n"
		);return -1;
	}
	unsigned int rk[RKLENGTH(KEYBITS)];
	//unsigned char key[KEYLENGTH(KEYBITS)];
	u8 tmp[16];
	//int i;
	int nrounds;

	/// warning: this key is confidential ///
	//if(argc>2){ //decrypt
		nrounds = rijndaelSetupDecrypt(rk, (u8*)"03nWNpmRjtmsXw95", KEYBITS);
	//}else{
	//	nrounds = rijndaelSetupEncrypt(rk, (u8*)"03nWNpmRjtmsXw95", KEYBITS);
	//}

	//if(argc>2){ //decrypt
		fread(buf,1,16,stdin);
		for(;;){
			int readlen=fread(buf+16,1,16,stdin);
			if(readlen<0)break;
			if(readlen==0){ //final block. process ISO10126 padding
				//memcpy(tmp,buf+16,16);
				rijndaelDecrypt(rk, nrounds, buf, buf);
				//for(i=0;i<16;i++)buf[i]^=iv[i];
				//memcpy(iv,tmp,16);
				//fprintf(stderr,"%d padding %d\n",readlen,buf[15]);
				u8 c=buf[15];
				fwrite(buf,1,16-(c<17?c:0),stdout); //lol...
				break;
			}
			memcpy(tmp,buf,16);
			rijndaelDecrypt(rk, nrounds, buf, buf);
			//for(i=0;i<16;i++)buf[i]^=iv[i];
			fwrite(buf,1,readlen,stdout);
			if(readlen<16)break; //error: truncated.
			//memcpy(iv,tmp,16);
			memcpy(buf,buf+16,16);
		}
		if(isatty(fileno(stdout)))puts("");
#if 0
	}else{
		for(;;){
			int readlen=fread(buf,1,16,stdin);
			if(readlen<0)break;
			if(readlen<16){ //process PKCS7 padding
				if(readlen==0)readlen=16;
				memset(buf+(readlen==16?0:readlen),16-readlen,16-readlen);
				for(i=0;i<16;i++)buf[i]^=iv[i];
				rijndaelEncrypt(rk, nrounds, buf, buf);
				fwrite(buf,1,16,stdout);
				break;
			}
			for(i=0;i<16;i++)buf[i]^=iv[i];
			rijndaelEncrypt(rk, nrounds, buf, buf);
			fwrite(buf,1,16,stdout);
			memcpy(iv,buf,16);
		}
	}
#endif
	return 0;
}
