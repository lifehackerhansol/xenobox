#include "../xenobox.h"

static unsigned int search(unsigned char *mem, unsigned int size,unsigned int base){
	if(size<100){fprintf(stderr,"too small\n");return 0;}
	//unsigned char *check="\x00\x80\x00\xc0\x00\x80\x00\xc0\x00\x80\x00\xc0\x00\x80\x00\xc0";
	unsigned char check[16];
	unsigned int lcheck=16;

	unsigned int i=0,count=0,n=0;
	for(i=0;i<16;i+=4)write32(check+i,base);

	for(i=0;i<size-100;i+=4){
		if(!memcmp(mem+i,check,lcheck)){
			fprintf(stderr,"Offset=%08x Virtual=%08x\n",i,n=i+base);
			count++;
		}
	}

	if(count==0){
		fprintf(stderr,"no hits (search error)\n");return 1;
	}

	for(i=0;i<size-100;i+=4){
		if(read32(mem+i)==n){
			fprintf(stderr,"Data=%08x\n",i+base+4);
		}
	}
	return 0;
}

int kallsymslookupsearch(const int argc, const char **argv){
	unsigned char *mem;
	unsigned int size;
	FILE *f;

	if(argc<3){// && isatty(fileno(stdin))){
		fprintf(stderr,
			"kallsyms_lookup_name searcher (beta)\n"
			"kallsymslookupsearch kernel base_address\n"
			"base_address is usually CONFIG_PAGE_OFFSET+0x8000\n"
		);return 1;
	}

#if 0
	if(!isatty(fileno(stdin))){
		fprintf(stderr,"stdin: ");
		size=0x1000000; //16MB
		mem=(unsigned char*)malloc(size);
		if(!mem){fprintf(stderr,"cannot alloc memory\n");goto stdin_end;}
		size=fread(mem,1,size,stdin);
		fprintf(stderr,"size=%u ",size);
		search(mem,size);

		free(mem);
stdin_end:;
	}
#endif
	unsigned int base=strtoul(argv[2],NULL,0);

	//for(;c<argc;c++){
	//	fprintf(stderr,"%s: ",argv[c]);
		f=fopen(argv[1],"rb");
		if(!f){fprintf(stderr,"cannot open kernel\n");return 1;}//continue;}
		fseek(f,0,SEEK_END);
		size=ftell(f);
		fseek(f,0,SEEK_SET);
		if(size>0x2000000){fprintf(stderr,"too big\n");fclose(f);return 1;}//continue;} //32MB
		mem=(unsigned char*)malloc(size);
		if(!mem){fprintf(stderr,"cannot alloc memory\n");fclose(f);return 1;}//continue;}
		fread(mem,1,size,f);

		search(mem,size,base);

		free(mem);
		//break;
	//}
	return 0;
}
