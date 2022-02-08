#include "../xenobox.h"

// [Unpack] gunprx xxx.prx xxx.hdr > xxx_elf.prx
// [Repack] gzprx xxx.prx xxx.hdr xxx_elf.prx #gzprx is alternative of procfw's pspgz.py

static unsigned char head[0x150];

int gzprx(const int argc, const char **argv){
	int fd,readlen;
	struct stat st;
	unsigned char magic[4];
	FILE *hdr,*elf;

	if(argc<4){
		fprintf(stderr,
			"gzprx out_gzip_prx in_hdr in_elf_prx [modulename] [moduleattr]\n\n"
			"Please note gzprx is useful only for development\n"
			"and hexediting existing prx.\n"
			"To compress new elf, you should use M33's psp-packer.\n"
		);
		return -1;
	}

	hdr=fopen(argv[2],"rb");
	if(!hdr){
		fprintf(stderr,"Cannot open %s\n",argv[2]);
		return 1;
	}
	if(fread(head,1,0x150,hdr)<0x150){
		fclose(hdr);
		fprintf(stderr,"Cannot read 0x150 bytes from %s\n",argv[2]);
		return 2;
	}
	fclose(hdr);
	if(memcmp(head,"~PSP",4)&&memcmp(head,"~SCE",4)){
		fprintf(stderr,"hdr invalid\n");
		return 3;
	}

	elf=fopen(argv[3],"rb");
	if(!elf){
		fprintf(stderr,"Cannot open %s\n",argv[3]);
		return 10;
	}
	if(fread(magic,1,4,elf)<4||magic[0]!=0x7f||magic[1]!='E'||magic[2]!='L'||magic[3]!='F'){
		fclose(elf);
		fprintf(stderr,"input isn't ELF (wrong file or already compressed)\n");
		return 11;
	}
	fseek(elf,0,SEEK_SET);

	//fprintf(stderr,"%s: ",argv[i]);
	fd=open(argv[1],OPEN_BINARY|O_WRONLY|O_CREAT|O_TRUNC);
	if(fd==-1){
		fprintf(stderr,"Cannot open %s\n",argv[1]);
		return 20;
	}
	write(fd,head,0x150);
	gzFile gz=gzdopen(fd,"wb9");
	if(!gz){fclose(elf);close(fd);fprintf(stderr,"cannot alloc gzip handle\n");return 21;}
	for(;(readlen=fread(buf,1,BUFLEN,elf))>0;){
		gzwrite(gz,buf,readlen);
	}
	gzclose(gz);
	fstat(fileno(elf),&st);
	fclose(elf);

	elf=fopen(argv[1],"r+b");
	fseek(elf,0x4,SEEK_SET);
	unsigned attr=read32(head+4);
	if(argc>5)attr=strtoul(argv[5],NULL,0);
	attr|=0x10000; //compressed
	write32(buf,attr);
	fwrite(buf,1,4,elf);

	if(argc>4){
		memset(cbuf,0,28);
		strncpy(cbuf,argv[4],28);
		fwrite(cbuf,1,28,elf);
	}

	fseek(elf,0x28,SEEK_SET);
	write32(buf,st.st_size);
	fwrite(buf,1,4,elf); //decompsize
	fstat(fileno(elf),&st);
	write32(buf,st.st_size);
	fwrite(buf,1,4,elf); //filesize

	fseek(elf,0xb0,SEEK_SET);
	write32(buf,st.st_size-0x150);
	fwrite(buf,1,4,elf); //compsize
	//fixme: ignore 0x140-0x150
	fclose(elf);
	return 0;
}
