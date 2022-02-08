#include "../xenobox.h"

//based on split_bootimg.pl (C) William Enck

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

//magic(8), ksize, kaddr, rsize, raddr, ssize, saddr, tagaddr, pagesize, unused, unused
//boot_name(8), boot_args(512), id(SHA first 8bytes. bah splitbootimg can't be streamized!)

static void TRASH(FILE *in, unsigned int size){
	while(size>0)fread(buf,1,size<BUFLEN?size:BUFLEN,in),size-=(size<BUFLEN?size:BUFLEN);
}

static void DUMP(FILE *in, FILE *out, unsigned int size){
	while(size>0)fread(buf,1,size<BUFLEN?size:BUFLEN,in),fwrite(buf,1,size<BUFLEN?size:BUFLEN,out),size-=(size<BUFLEN?size:BUFLEN);
}

int splitbootimg(const int argc, const char **argv){
	if(argc<3 || isatty(fileno(stdin))){
		fprintf(stderr,
			"splitbootimg KERNEL RAMDISK [SECOND] <boot.img\n"
			"filenames can be NULL\n"
			"example [direct ramdisk extract]:\n"
			"xenounubinize <mtd/boot.img|splitbootimg NULL /dev/stdout|cpio -i\n"
		);return 1;
	}
	fread(buf,1,48,stdin);
	unsigned int counter=48;
	if(memcmp(buf,BOOT_MAGIC,BOOT_MAGIC_SIZE)){fprintf(stderr,"BootMagic wrong\n");return 1;}
	unsigned int ksize=read32(buf+8),kaddr=read32(buf+12),rsize=read32(buf+16),raddr=read32(buf+20),ssize=read32(buf+24),saddr=read32(buf+28);
	unsigned int pagesize=read32(buf+36);

	fprintf(stderr,"Page size:      %d (0x%08x)\n", pagesize, pagesize);
	fprintf(stderr,"Kernel addr:    %d (0x%08x)\n", kaddr, kaddr);
	fprintf(stderr,"Kernel size:    %d (0x%08x)\n", ksize, ksize);
	fprintf(stderr,"Ramdisk addr:   %d (0x%08x)\n", raddr, raddr);
	fprintf(stderr,"Ramdisk size:   %d (0x%08x)\n", rsize, rsize);
	fprintf(stderr,"Second addr:    %d (0x%08x)\n", saddr, saddr);
	fprintf(stderr,"Second size:    %d (0x%08x)\n", ssize, ssize);

	fread(buf,1,BOOT_NAME_SIZE,stdin);buf[BOOT_NAME_SIZE]=0;counter+=BOOT_NAME_SIZE;
	fprintf(stderr,"Board name: %s\n",cbuf);
	fread(buf,1,BOOT_ARGS_SIZE,stdin);buf[BOOT_ARGS_SIZE]=0;counter+=BOOT_ARGS_SIZE;
	fprintf(stderr,"Comamnd line: %s\n",cbuf);

	int mul=1;
	unsigned int koffset=pagesize*mul,roffset=align2p(pagesize*mul,koffset+ksize),soffset=align2p(pagesize*mul,roffset+rsize);
	if(strcmp(argv[1],"NULL")){
		TRASH(stdin,koffset-counter);
		FILE *f=fopen(argv[1],"wb");
		DUMP(stdin,f,ksize);
		counter=koffset+ksize;
		fclose(f);
	}
	if(strcmp(argv[2],"NULL")){
		TRASH(stdin,roffset-counter);
		FILE *f=fopen(argv[2],"wb");
		DUMP(stdin,f,rsize);
		counter=roffset+rsize;
		fclose(f);
	}
	if(argc>3&&strcmp(argv[3],"NULL")&&ssize>0){
		TRASH(stdin,soffset-counter);
		FILE *f=fopen(argv[3],"wb");
		DUMP(stdin,f,ssize);
		counter=soffset+ssize;
		fclose(f);
	}
	return 0;
}
