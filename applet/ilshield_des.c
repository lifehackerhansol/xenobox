#include "../xenobox.h"
#include "../lib/des.h"
#include "../lib/sha256.h"

// .Net DESCryptoServiceProvider compatible routine
// http://packetstormsecurity.org/crypt/aes/des/
// IV support based on http://polarssl.org/trac/browser/trunk/library/des.c (not copied)

int ilshield_des(const int argc, const char **argv){
	if(argc<2||isatty(fileno(stdin))||isatty(fileno(stdout))){fprintf(stderr,"ilshield_des keyfile/:password [d] < in > out\n");return 1;} 
	u32 key[32];
	u8 iv[8],tmp[8];
	int i;

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
		iv[i%8] = (byte)(iv[i%8]^sha256ctx.buffer.u8[i]);
	}
	des_ky(iv,key);

	memset(iv,0,sizeof(iv));
	for(i=0;i<32;i++){
		iv[i%8] = (byte)(iv[i%8]+sha256ctx.buffer.u8[i]);
	}

	if(argc>2){ //decrypt
		fread(buf,1,8,stdin);
		for(;;){
			int readlen=fread(buf+8,1,8,stdin);
			if(readlen<0)break;
			if(readlen==0){ //final block. process PKCS5(7) padding
				//memcpy(tmp,buf+8,8);
				des_dc(buf,buf,key);
				for(i=0;i<8;i++)buf[i]^=iv[i];
				//memcpy(iv,tmp,8);
				//fprintf(stderr,"%d padding %d\n",readlen,buf[7]);
				u8 c=buf[7];
				fwrite(buf,1,8-(c<9?c:0),stdout); //lol...
				break;
			}
			memcpy(tmp,buf,8);
			des_dc(buf,buf,key);
			for(i=0;i<8;i++)buf[i]^=iv[i];
			fwrite(buf,1,readlen,stdout);
			if(readlen<8)break; //error: truncated.
			memcpy(iv,tmp,8);
			memcpy(buf,buf+8,8);
		}
	}else{
		for(;;){
			int readlen=fread(buf,1,8,stdin);
			if(readlen<0)break;
			if(readlen<8){ //process PKCS5 padding
				if(readlen==0)readlen=8;
				memset(buf+(readlen==8?0:readlen),8-readlen,8-readlen);
				for(i=0;i<8;i++)buf[i]^=iv[i];
				des_ec(buf,buf,key);
				fwrite(buf,1,8,stdout);
				break;
			}
			for(i=0;i<8;i++)buf[i]^=iv[i];
			des_ec(buf,buf,key);
			fwrite(buf,1,8,stdout);
			memcpy(iv,buf,8);
		}
	}
	return 0;
}
