#include "../xenobox.h"
#include "../lib/des.h"

static char *_key="790077003F002800"; //"3F0050003F003F00"; //v1

static int txt2bin(const char *src, u8 *dst){
	int i=0;
	for(;i<64;i++){
		unsigned char src0=src[2*i];
		if(!src0||src0==' '||src0=='\t'||src0=='\r'||src0=='\n'||src0=='#'||src0==';'||src0=='\''||src0=='"')break;
		if(0x60<src0&&src0<0x7b)src0-=0x20;
		//if(!( isdigit(src0)||(0x40<src0&&src0<0x47) )){fprintf(stderr,"Invalid character %c\n",src0);exit(-1);}
		src0=isdigit(src0)?(src0-'0'):(src0-55);

		unsigned char src1=src[2*i+1];
		if(0x60<src1&&src1<0x7b)src1-=0x20;
		//if(!( isdigit(src1)||(0x40<src1&&src1<0x47) )){fprintf(stderr,"Invalid character %c\n",src1);exit(-1);}
		src1=isdigit(src1)?(src1-'0'):(src1-55);
		dst[i]=(src0<<4)|src1;
		//fprintf(stderr,"%02X",dst[i]);
	}
	//if(i!=8){printf("key/iv length must be 8bytes in hex\n");exit(-1);}
	return i;
//puts("");
}

// .Net DESCryptoServiceProvider compatible routine
// http://packetstormsecurity.org/crypt/aes/des/
// IV support based on http://polarssl.org/trac/browser/trunk/library/des.c (not copied)

int dsapfilt(const int argc, const char **argv){
	if(isatty(fileno(stdin))||isatty(fileno(stdout))){fprintf(stderr,"dsapfilt [d] < in > out\n");return 1;} 
	u32 key[32];
	u8 iv[8],tmp[8];
	int i;

	txt2bin(_key,iv);
	des_ky(iv,key);
	txt2bin(_key,iv);

	if(argc>1){ //decrypt
		fread(buf,1,8,stdin);
		for(;;){
			int readlen=fread(buf+8,1,8,stdin);
			if(readlen<0)break;
			if(readlen==0){ //final block. process PKCS5 padding
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
