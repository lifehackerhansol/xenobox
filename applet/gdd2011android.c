#include "../xenobox.h"
#include "../lib/sha1.h"

static char *T="243570BED1C698FA";

int gdd2011android(const int argc, const char **argv){
	if(argc<2){fprintf(stderr,"gdd2011android email passphrase\n");return 1;}

	struct sha1_ctxt ctxt;
	unsigned char sha1[20];
	char pass[30];
	int i=0,j=0;

	sha1_init(&ctxt);
	sha1_loop(&ctxt,(u8*)argv[1],strlen(argv[1]));
	if(argc>2)sha1_loop(&ctxt,(u8*)argv[2],strlen(argv[2])); //aww weak. no separater?
	sha1_result(&ctxt,sha1); //now sha1 has the value (in binary)
	for(;i<10;i++){
		unsigned char seed=sha1[i]^sha1[i+10];
		pass[j++]=T[seed>>4],pass[j++]=T[seed&0xf];
		if(i&1)pass[j++]=' ';
	}
	pass[j-1]=0; //kill trailing space
	puts(pass);
	return 0;
}
