#include "../xenobox.h"
#define HEADSIZE 256

static u8 key[8],head[HEADSIZE];
int m4c2m4a(const int argc, const char **argv){
	if(isatty(fileno(stdin))||isatty(fileno(stdout))){
		fprintf(stderr,"m4c2m4a <in.m4c >out.m4a\n");
		return 1;
	}

	int i,j,readlen;
	fread(head,1,HEADSIZE,stdin);

	//determine other than byte3
	memcpy(key,head,8);
	for(i=0;i<8;i++)key[i]^="\0\0\0?ftyp"[i];

	//determine byte3
	u8 *p=head;
	for(i=0;i<HEADSIZE/8;i++)
		for(j=i+1;j<HEADSIZE/8;j++)
			if(!memcmp(p+(i*8),p+(j*8),8))goto done;
	fprintf(stderr,"key find failed\n");
	return 2;

done:
	key[3]=p[j*8+3];
	fprintf(stderr,"key=");
	for(i=0;i<8;i++)fprintf(stderr,"%02x",key[i]);
	fprintf(stderr,"\n");

	//decrypt head
	for(i=0;i<HEADSIZE;i++)head[i]^=key[i%8];
	fwrite(head,1,HEADSIZE,stdout);

	//decrypt remaining data
	for(;readlen=fread(buf,1,BUFLEN,stdin);){
		for(i=0;i<readlen;i++)buf[i]^=key[i%8];
		fwrite(buf,1,readlen,stdout);
	}
	return 0;
}
