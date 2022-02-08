#include "../xenobox.h"

//Todo: rewrite handler scheme to support base64 encoding...

#define F(name) int name(FILE *f, const char *fname, const char *desired);
F(compute_adler32)
F(compute_bsdsum)
F(compute_bz2crc32)
F(compute_cksum)
F(compute_crc16)
F(compute_crc32)
F(compute_elf32)
F(compute_moonguid)
F(compute_md5sum)
F(compute_sha1sum)
F(compute_sha256sum)
F(compute_size)
F(compute_sum32)
F(compute_sysvsum)
#undef F

typedef struct{
	char *name;
	int  (*compute)(FILE*, const char*, const char*);
}hash_applet;

static const hash_applet apps[]={
	{"xenoadler32",compute_adler32},
	{"xenobsdsum",compute_bsdsum},
	{"xenobz2crc32",compute_bz2crc32},
	{"xenocksum",compute_cksum},
	{"xenocrc16",compute_crc16},
	{"xenocrc32",compute_crc32},
	{"xenoelf32",compute_elf32},
	{"xenomd5sum",compute_md5sum},
	{"xenomoonguid",compute_moonguid},
	{"xenosha1sum",compute_sha1sum},
	{"xenosha256sum",compute_sha256sum},
	{"xenosize",compute_size},
	{"xenosum32",compute_sum32},
	{"xenosysvsum",compute_sysvsum},
};
static const int appsize=sizeof(apps)/sizeof(hash_applet);
static int compute_index;

static int check(const char *sum_name){
	char remote_sum[65]; //sha256sum(ascii) costs 64bytes
	char path_file[768];
	FILE *f,*g;

	if(!strcmp(sum_name,"")){
		f=stdin;
	}else{
		f=fopen(sum_name,"rb");
	}
	if(!f){fprintf(stderr,"cannot open %s\n",sum_name);return 1;}
	for(;myfgets(cbuf,1024,f);){
		*path_file=0;
		sscanf(cbuf,"%s%s",remote_sum,path_file);
		char *x=path_file;
		if(*x=='*')x++;
		if(!strcmp(x,"-")){apps[compute_index].compute(stdin,"-",remote_sum);continue;} //lol?
		g=fopen(x,"rb");
		if(!g){
			printf("%s: Not found\n",x);continue;
		}
		apps[compute_index].compute(g,x,remote_sum);
		fclose(g);
	}
	if(f!=stdin)fclose(f);
	return 0;
}

int xenohash(const int argc, const char **argv){
	for(compute_index=0;compute_index<appsize;compute_index++)if(!strcasecmp(argv[0],apps[compute_index].name))break;
	if(compute_index==appsize){fprintf(stderr,"xenohash: internal error.\n");return -1;}

	if(isatty(fileno(stdin))&&argc<2){
		fprintf(stderr,
			"%s [file...] [:string...] [< file] >hash.%s\n"
			"[Check] %s @[hash.%s]\n"
			"If you specify file, use \"-\" for stdin.\n",
			apps[compute_index].name,apps[compute_index].name+4,apps[compute_index].name,apps[compute_index].name+4
		);
		return 1;
	}
	if(argc==1)apps[compute_index].compute(stdin,"-",NULL);
	if(argc==2&&argv[1][0]=='@'){return check(argv[1]+1);}
	int i=1;
	for(;i<argc;i++){
		if(argv[1][0]==':'){apps[compute_index].compute(NULL,argv[1]+1,NULL);continue;}
		if(!strcmp(argv[i],"-")){apps[compute_index].compute(stdin,"-",NULL);continue;}
		FILE *f=fopen(argv[i],"rb");
		if(!f){fprintf(stderr,"%s: cannot open\n",argv[i]);continue;}
		apps[compute_index].compute(f,argv[i],NULL);
		fclose(f);
	}
	return 0;
}
