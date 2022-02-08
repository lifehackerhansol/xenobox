#include "../xenobox.h"

typedef struct{
	unsigned int gamecode;
	unsigned int crc32;
	unsigned long long int offset;
}cheatindex;

static void check(FILE *f, unsigned int fsize, unsigned int gamecode, unsigned int CRC32){
	cheatindex cur,next;
	fread(&next,1,sizeof(next),f);
	fprintf(stderr,"[%c%c%c%c:%08x] ",gamecode&0xff,(gamecode>>8)&0xff,(gamecode>>16)&0xff,(gamecode>>24),CRC32);
	for(;;){
		memcpy(&cur,&next,sizeof(cur));
		fread(&next,1,sizeof(next),f);
		if(gamecode==cur.gamecode&&(CRC32==cur.crc32||!CRC32)){
			fprintf(stderr,LLU" bytes\n",(next.offset?next.offset:fsize)-cur.offset);
			break;
		}
		if(!next.offset){
			fprintf(stderr,"not found\n");
			break;
		}
	}
}

static int txt2bin(const char *src, unsigned char *dst){ //from m3patch
	int i=0;
	for(;i<4;i++){
		unsigned char src0=src[2*i];
		if(!src0||src0==' '||src0=='\t'||src0=='\r'||src0=='\n'||src0=='#'||src0==';'||src0=='\''||src0=='"')break;
		if(0x60<src0&&src0<0x7b)src0-=0x20;
		if(!( isdigit(src0)||(0x40<src0&&src0<0x47) )){fprintf(stderr,"Invalid character %c\n",src0);exit(-1);}
		src0=isdigit(src0)?(src0-'0'):(src0-55);

		unsigned char src1=src[2*i+1];
		if(0x60<src1&&src1<0x7b)src1-=0x20;
		if(!( isdigit(src1)||(0x40<src1&&src1<0x47) )){fprintf(stderr,"Invalid character %c\n",src1);exit(-1);}
		src1=isdigit(src1)?(src1-'0'):(src1-55);
		dst[i]=(src0<<4)|src1;
		//fprintf(stderr,"%02X",dst[i]);
	}
	return i;
//fprintf(stderr,"\n");
}

int checkcheatsize(const int argc, const char **argv){
	int i=2;
	FILE *f,*rom;
	if(argc<3){fprintf(stderr,"checkcheatsize usrcheat.dat [game.nds|GAME:aabbccdd]...\n");return 1;}
	if(!(f=fopen(argv[1],"rb"))){fprintf(stderr,"cannot open cheat file\n");return 2;}
	fread(buf,1,12,f);
	if(memcmp(buf,"R4 CheatCode",12)){fprintf(stderr,"not usrcheat format\n");return 3;}
	unsigned int fsize=filelength(fileno(f));
	for(;i<argc;i++){
		fseek(f,0x100,SEEK_SET);
		if(argv[i][4]!=':'){
			fprintf(stderr,"%s: ",argv[i]);
			if(!(rom=fopen(argv[i],"rb"))){fprintf(stderr,"cannot open\n");continue;}
			fread(buf,1,512,rom);
			fclose(rom);
			fprintf(stderr,"\n");
			//memset(buf+0x160,0,0xa0); /////
			check(f,fsize,read32(buf+12),crc32(0,buf,512)^0xffffffff);
		}else{
			if(strlen(argv[i])!=13){fprintf(stderr,"argument format error\n");continue;}
			unsigned char crc_bin[4],c;
			txt2bin(argv[i]+5,crc_bin);
			c=crc_bin[0],crc_bin[0]=crc_bin[3],crc_bin[3]=c;
			c=crc_bin[1],crc_bin[1]=crc_bin[2],crc_bin[2]=c;
			check(f,fsize,read32(argv[i]),read32(crc_bin));
		}
	}
	fclose(f);
	return 0;
}
