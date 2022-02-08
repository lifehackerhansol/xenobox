/*
	kaura lng assembler

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

int kauralngasm(const int argc,const char **argv){
	char array[512][512];
	memset(buf,0,sizeof(buf));

	if(argc<3){fprintf(stderr,"Kaura lng assembler: kauralngasm in.txt out.lng\n");return 1;}
	FILE *f=fopen(argv[1],"r");
	if(!f){fprintf(stderr,"Cannot open file\n");return 1;}
	unsigned short CharsetID=0;
	myfgets(array[0],512,f);
	CharsetID=atoi(array[0]);
	//myfgets(array[0],512,f);
	if(!CharsetID){fclose(f);fprintf(stderr,"Failed to load CharsetID\n");return 1;}

	unsigned short nStrings=0;
	while(myfgets(array[nStrings],512,f))nStrings++;
	fclose(f);
	unsigned short offset=0x20+4*nStrings;
	unsigned short next=offset;
	unsigned int i=0;
	for(;i<nStrings;i++){
		unsigned int len=strlen(array[i]);
		memcpy(buf+next,array[i],len);
		write32(buf+0x20+4*i,next-offset);
		next+=len+2;
	}

	memcpy(buf,"OKMFS KAURA1.0.0",16);
	write32(buf+0x10,offset);
	write32(buf+0x14,next-offset);
	write16(buf+0x18,nStrings);
	write16(buf+0x1a,CharsetID);
	buf[0x1c]=buf[0x1d]=buf[0x1e]=buf[0x1f]=0xff;

	f=fopen(argv[2],"wb");
	if(!f){fprintf(stderr,"Cannot open output\n");return 2;}
	fwrite(buf,1,next,f);
	fclose(f);
	return 0;
}
