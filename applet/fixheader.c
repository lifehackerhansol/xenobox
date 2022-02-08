#include "../xenobox.h"

int fixheader(const int argc, const char **argv){
	unsigned char head[0x160];
	FILE *f;
	int i=1;

	if(argc<2){fprintf(stderr,"fixheader nds...\n");return 1;}
	for(;i<argc;i++){
		fprintf(stderr,"%s... ",argv[i]);
		f=fopen(argv[i],"r+b");
		if(!f){fprintf(stderr,"cannot open\n");continue;}
		fread(head,1,0x15c,f);
		write16(head+0x15c,crc16(0xffff,head+0xc0,0x9c));
		write16(head+0x15e,crc16(0xffff,head,0x15e));
		fseek(f,0,SEEK_CUR); //This is garbage, but Windows Version requires this... I don't know why.
		fwrite(head+0x15c,1,4,f);
		fclose(f);
		fprintf(stderr,"done\n");
	}
	return 0;
}
