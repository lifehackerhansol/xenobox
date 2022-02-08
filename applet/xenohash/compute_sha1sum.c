#include "../../xenobox.h"
#include "../../lib/sha1.h"

#define HASH_SIZE 20
static char hash[HASH_SIZE*2+1];
int compute_sha1sum(FILE *f, const char *name, const char *desired){
	struct sha1_ctxt sha1ctx;
	sha1_init(&sha1ctx);
	if(f){
		int readlen;
		for(;(readlen=fread(buf,1,BUFLEN,f))>0;){
			sha1_loop(&sha1ctx,buf,readlen);
		}
	}else{
		sha1_loop(&sha1ctx,(unsigned char*)name,strlen(name));
	}
	unsigned char digest[HASH_SIZE];
	sha1_result(&sha1ctx,digest);

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
