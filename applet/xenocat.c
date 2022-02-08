#include "../xenobox.h"

static int perform(FILE *f, const char *name){
	int readlen;
	for(;(readlen=fread(buf,1,BUFLEN,f))>0;){
		fwrite(buf,1,readlen,stdout);
	}
	return 0;
}

int xenocat(const int argc, const char **argv){
	if(isatty(fileno(stdin))&&isatty(fileno(stdout))&&argc<2){
		fprintf(stderr,"xenocat [file...] [< file]\nIf you specify file, use \"-\" for stdin.\n");
		return 1;
	}
	if(argc==1)perform(stdin,"-");
	int i=1;
	for(;i<argc;i++){
		if(!strcmp(argv[i],"-")){perform(stdin,"-");continue;}
		FILE *f=fopen(argv[i],"rb");
		if(!f){fprintf(stderr,"%s: cannot open\n",argv[i]);continue;}
		perform(f,argv[i]);
		fclose(f);
	}
	return 0;
}
