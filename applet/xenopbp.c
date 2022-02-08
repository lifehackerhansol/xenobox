#include "../xenobox.h"

static char *filename[8] = {
   "PARAM.SFO",
   "ICON0.PNG",
   "ICON1.PMF",
   "PIC0.PNG",
   "PIC1.PNG",
   "SND0.AT3",
   "DATA.PSP",
   "DATA.PSAR"
};
static unsigned int fileoffset[8];
static unsigned int filesize[8];

static void fcopy(FILE *in,FILE *out,unsigned int size){
	//int flag=0;
	unsigned int readlen;
	for(;size>0;size-=BUFLEN){
		readlen=fread(buf,1,size<BUFLEN?size:BUFLEN,in);
		fwrite(buf,1,readlen,out);
		if(readlen<(size<BUFLEN?size:BUFLEN)||size<BUFLEN)break;
	}
}

int xenopbp(const int argc, const char **argv){
	int i;
	unsigned char head[40];
	FILE *in,*out;

	if(argc!=2 && argc!=10){
		fprintf(stderr,
			"xenopbp EBOOT.PBP [PARAM.SFO ICON0.PNG ICON1.PMF PIC0.PNG PIC1.PNG SND0.AT3 DATA.PSP DATA.PSAR]\n"
		);
		return -1;
	}
	if(argc==2){
		in=fopen(argv[1],"rb");
		if(!in){
			fprintf(stderr,"Cannot open %s\n",argv[1]);
			return 1;
		}
		unsigned int fsize = filelength(fileno(in));
		if(fsize<40||fread(head,1,40,in)<40||read32(head+36)>fsize||head[0]||memcmp(head+1,"PBP",3)){
			fclose(in);
			fprintf(stderr,"wrong format %s\n",argv[1]);
			return 2;
		}
		for(i=0;i<8;i++)fileoffset[i]=read32(head+8+4*i);
		for(i=0;i<7;i++)filesize[i]=fileoffset[i+1]-fileoffset[i];
		filesize[7]=fsize-fileoffset[7];
		for(i=0;i<8;i++){
			if(!filesize[i]){fprintf(stderr,"[%d] %10d bytes | %s\n",i,filesize[i],filename[i]);continue;}
			out=fopen(filename[i],"wb");
			if(!out){
				fclose(in);
				fprintf(stderr,"Cannot open %s\n",filename[i]);
				return 2;
			}
			fprintf(stderr,"[%d] %10d bytes | %s\n",i,filesize[i],filename[i]);
			fseek(in,fileoffset[i],SEEK_SET);
			fcopy(in,out,filesize[i]);
			fclose(out);
		}
	}
	if(argc==10){
		out=fopen(argv[1],"wb");
		if(!out){
			fprintf(stderr,"Cannot open %s\n",argv[1]);
			return 1;
		}
		memset(head,0,40);
		head[1]='P',head[2]='B',head[3]='P',head[6]=1;
		fwrite(head,1,40,out);
		for(i=0;i<8;i++){
			fileoffset[i]=ftell(out);
			if(!strcmp(argv[2+i],"NULL")||!strcmp(argv[2+i],"null")){fprintf(stderr,"[%d] %10d bytes | %s\n",i,0,argv[2+i]);continue;}
			in=fopen(argv[2+i],"rb");
			if(!in){
				//fclose(out);
				fprintf(stderr,"Cannot open %s\n",argv[2+i]);
				continue;
			}
			fprintf(stderr,"[%d] %10d bytes | %s\n",i,(int)filelength(fileno(in)),argv[2+i]);
			fcopy(in,out,filelength(fileno(in)));
			fclose(in);
		}
		for(i=0;i<8;i++)write32(head+8+4*i,fileoffset[i]);
		fseek(out,8,SEEK_SET);
		fwrite(head+8,1,32,out);
		fclose(out);
	}
	return 0;
}
