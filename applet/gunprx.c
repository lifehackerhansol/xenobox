#include "../xenobox.h"

// [Unpack] gunprx xxx.prx xxx.hdr > xxx_elf.prx
// [Repack] gzprx xxx.prx xxx.hdr xxx_elf.prx #gzprx is alternative of procfw's pspgz.py

static unsigned char head[0x150];

int gunprx(const int argc, const char **argv){
	int fd,readlen;
	struct stat st;
	unsigned char magic[4];
	FILE *hdr;

	if(argc<2){
		fprintf(stderr,"gunprx in_gzip_prx [out_hdr] > out_elf_prx\n");
		return -1;
	}

	//fprintf(stderr,"%s: ",argv[i]);
	fd=open(argv[1],OPEN_BINARY|O_RDONLY);
	if(fd==-1){
		fprintf(stderr,"Cannot open %s\n",argv[1]);
		return 1;//continue;
	}
	fstat(fd,&st);
	if(st.st_size<0x153){
		close(fd);
		fprintf(stderr,"Too small\n");
		return 2;
	}
	//lseek(fd,0x150,SEEK_SET);
	read(fd,head,0x150);
	if(memcmp(head,"~PSP",4)&&memcmp(head,"~SCE",4)){
		close(fd);
		fprintf(stderr,"Not prx\n");
		return 3;
	}
	read(fd,magic,4);
	lseek(fd,0x150,SEEK_SET);
	if(magic[0]==0x1f&&magic[1]==0x8b&&magic[2]==0x08){
		gzFile gz=gzdopen(fd,"rb");
		if(!gz){close(fd);fprintf(stderr,"cannot alloc gzip handle\n");return 4;}
		for(;(readlen=gzread(gz,buf,BUFLEN))>0;){
			fwrite(buf,1,readlen,stdout);
		}
		gzclose(gz);
	}else if(magic[0]==0x7f&&magic[1]=='E'&&magic[2]=='L'&&magic[3]=='F'){
		for(;(readlen=read(fd,buf,BUFLEN))>0;){
			fwrite(buf,1,readlen,stdout);
		}
		close(fd);
	}else{
		close(fd);
		fprintf(stderr,"Not gzip/elf\n");
		return -2;
	}

	if(argc>2){
		hdr=fopen(argv[2],"wb");
		if(!hdr){
			fprintf(stderr,"Cannot open %s\n",argv[2]);
			return 10;
		}
		fwrite(head,1,0x150,hdr);
		fclose(hdr);
	}
	return 0;
}
