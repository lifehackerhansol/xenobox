#include "../xenobox.h"

int xenounubinize(const int argc, const char **argv){
	int readlen;
	if(isatty(fileno(stdin))||isatty(fileno(stdout))){fprintf(stderr,"xenounubinize <in >out\n");return 1;}
	//fseek(stdin,0x040000,SEEK_SET);
	fread(buf,1,0x10000,stdin);fread(buf,1,0x10000,stdin);fread(buf,1,0x10000,stdin);fread(buf,1,0x10000,stdin);
	for(;!feof(stdin);){
		readlen=fread(buf,1,0x800,stdin);
		if(readlen<0x800)break;
		readlen=fread(buf,1,0x1f800,stdin);
		fwrite(buf,1,readlen,stdout);
		if(readlen<0x1f800)break;
	}
	return 0;
}
