#include "../xenobox.h"

static unsigned int search(unsigned char *mem, unsigned int size){
	if(size<100){fprintf(stderr,"too small\n");return 0;}
	unsigned int i=0,p=0,x=0,count=0;
	for(;i<size-100;i+=4){
		if(
			mem[i+ 3]==0xe5&&mem[i+ 2]==0x9f                                   && // ldr r*,=immediate
			mem[i+ 7]==0xe3&&mem[i+ 6]==0xa0&&mem[i+ 5]==0x10&&mem[i+ 4]==0x48 && // mov r1,#0x48
			mem[i+11]==0xe5&&(mem[i+10]&0xf0)==0x90                            && // ldr ...
			((
				mem[i+15]==0xe3&&mem[i+14]==0xa0                               && // mov r*...
				mem[i+19]==0xeb                                                   // bl
			)||(
				mem[i+15]==0xeb                                                   // bl
			))
		){
			x=((mem[i+1]&0x0f)<<8)|mem[i];
			p=i+x+8;
			//fprintf(stderr,"%08x %08x -> %08x\n",i,p,read32(mem+p));
			fprintf(stderr,"%08x\n",read32(mem+p));
			count++;
		}
	}
	if(count==0){
		fprintf(stderr,"no hits (search error)\n");return 0;
	}else if(count>1){
		fprintf(stderr,"multiple hits (search error)\n");return 0;
	}else if(!isatty(fileno(stdout))){
		printf("%08x",read32(mem+p));
	}
	return read32(mem+p);
}

int sprotectsearch(const int argc, const char **argv){
	unsigned char *mem;
	unsigned size,c=1;
	FILE *f;

	if(argc<2 && isatty(fileno(stdin))){fprintf(stderr,"s_protect_info searcher\nsprotectsearch kernel...\n");return 1;}

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

	for(;c<argc;c++){
		fprintf(stderr,"%s: ",argv[c]);
		f=fopen(argv[c],"rb");
		if(!memcmp(argv[c],"/dev/",5)){ //cannot fseek
			if(!f){fprintf(stderr,"cannot open kernel. non-root?\n");continue;}
			size=0x1000000; //16MB
			mem=(unsigned char*)malloc(size);
			if(!mem){fprintf(stderr,"cannot alloc memory\n");fclose(f);continue;}
			size=fread(mem,1,size,f);
			fprintf(stderr,"size=%u ",size);
		}else{
			if(!f){fprintf(stderr,"cannot open kernel\n");continue;}
			fseek(f,0,SEEK_END);
			size=ftell(f);
			fseek(f,0,SEEK_SET);
			if(size>0x2000000){fprintf(stderr,"too big\n");fclose(f);continue;} //32MB
			mem=(unsigned char*)malloc(size);
			if(!mem){fprintf(stderr,"cannot alloc memory\n");fclose(f);continue;}
			fread(mem,1,size,f);
		}
		fclose(f);

		search(mem,size);

		free(mem);
		//break;
	}
	return 0;
}
