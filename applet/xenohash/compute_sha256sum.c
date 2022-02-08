#include "../../xenobox.h"
#include "../../lib/sha256.h"

#define HASH_SIZE 32
static char hash[HASH_SIZE*2+1];
int compute_sha256sum(FILE *f, const char *name, const char *desired){
	lzma_check_state sha256ctx;
	lzma_sha256_init(&sha256ctx);
	if(f){
		int readlen;
		for(;(readlen=fread(buf,1,BUFLEN,f))>0;){
			lzma_sha256_update(buf,readlen,&sha256ctx);
		}
	}else{
		lzma_sha256_update((unsigned char*)name,strlen(name),&sha256ctx);
	}
	lzma_sha256_finish(&sha256ctx);
	unsigned char *digest=sha256ctx.buffer.u8;

	hash[HASH_SIZE*2]=0;
	int i=0;
	for(;i<HASH_SIZE;i++){
		int x=digest[i]>>4,y=digest[i]&0xf;
		hash[2*i+0]=x>9?(x-10+'a'):(x+'0');
		hash[2*i+1]=y>9?(y-10+'a'):(y+'0');
	}
	if(desired)
		printf("%s: %s\n",name,!strcasecmp(hash,desired)?"OK":"NG");
	else
		printf("%s  %s\n",hash,name);
	return 0;
}
