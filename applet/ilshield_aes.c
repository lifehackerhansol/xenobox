#include "../xenobox.h"
#include "../lib/rijndael.h"
#include "../lib/sha256.h"

//I'm not sure if I should use 128/192/256...
#define KEYBITS 256

// .Net RijndaelManaged/AesCryptoServiceProvider compatible routine
// http://www.efgh.com/software/rijndael.htm
// IV support based on http://polarssl.org/trac/browser/trunk/library/des.c (not copied)

int ilshield_aes(const int argc, const char **argv){
	if(argc<2||isatty(fileno(stdin))||isatty(fileno(stdout))){fprintf(stderr,"ilshield_aes keyfile/:password [d] < in > out\n");return 1;} 
	unsigned int rk[RKLENGTH(KEYBITS)];
	//unsigned char key[KEYLENGTH(KEYBITS)];
	u8 iv[32],tmp[16];
	int i;
	int nrounds;

	lzma_check_state sha256ctx;
	lzma_sha256_init(&sha256ctx);
	if(argv[1][0]==':'){
		lzma_sha256_update((unsigned char*)argv[1]+1,strlen(argv[1]+1),&sha256ctx);
	}else{
		FILE *f=fopen(argv[1],"rb");
		if(!f){fprintf(stderr,"fopen %s failed\n",argv[1]);return 2;}
		int size=filelength(fileno(f));
		for(;size;size-=min(size,BUFLEN)){
			int readlen=fread(buf,1,min(size,BUFLEN),f);
			lzma_sha256_update(buf,readlen,&sha256ctx);
		}
		fclose(f);
	}
	lzma_sha256_finish(&sha256ctx);

	memset(iv,0,sizeof(iv));
	for(i=0;i<32;i++){
		iv[i%32] = (byte)(iv[i%32]^sha256ctx.buffer.u8[i]);
	}
	if(argc>2){ //decrypt
		nrounds = rijndaelSetupDecrypt(rk, iv, KEYBITS);
	}else{
		nrounds = rijndaelSetupEncrypt(rk, iv, KEYBITS);
	}

	memset(iv,0,sizeof(iv));
	for(i=0;i<32;i++){
		iv[i%16] = (byte)(iv[i%16]+sha256ctx.buffer.u8[i]);
	}

	if(argc>2){ //decrypt
		fread(buf,1,16,stdin);
		for(;;){
			int readlen=fread(buf+16,1,16,stdin);
			if(readlen<0)break;
			if(readlen==0){ //final block. process PKCS7 padding
				//memcpy(tmp,buf+16,16);
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
	return 0;
}
