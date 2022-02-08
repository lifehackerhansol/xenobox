/*
	kaura lng disassembler

	OKMFS KAURA1.0.0(16bytes)
	data offset(4)
	data length(4)
	number of header(2)
	charcode id(2)
	Unknown FFFFFFFF(4)
	header(4*"number of header")
	data(Separated with 0000)
*/

#include "../xenobox.h"

int kauralngdis(const int argc,const char **argv){
	if(argc<2){printf("Kaura lng disassembler: kauralngdis in.lng > out.txt\n");return 1;}
	FILE *f=fopen(argv[1],"rb");
	if(!f){printf("Cannot open file\n");return 1;}
	fread(buf,1,BUFLEN,f);
	fclose(f);

	if(memcmp(buf,"OKMFS KAURA1.0.0",16)){fclose(f);printf("Not kaura lng\n");return 1;}
	unsigned int offset=read32(buf+0x10);
	//unsigned int final=read32(buf+0x14)+offset;
	unsigned short nStrings=read16(buf+0x18);
	unsigned short CharsetID=read16(buf+0x1a);

	printf("%u\n",CharsetID);
	unsigned int i=0;
	for(;i<nStrings;i++){
		puts((char*)buf+offset+read32(buf+0x20+4*i));
	}
	return 0;
}
