/*
	m3sakura_make v3
*/

#include "../xenobox.h"

extern unsigned char ndshead[512];

static const byte *dsbmagic=(byte*)"DSBooter\0\0\0\0";
static const byte null256[256]={0};

//const char *tp="ndstool -c \"%s\" -r9 0x%08x -r7 0x%08x -9 tmp.9 -7 tmp.7";
//char arg[1024];

static int c=1;
static int make(const byte *nds,const int ndslen,const char *p){
	byte *pA=NULL,*p7=NULL,*p9=NULL;
	u32 l7,l9,a7,a9,pad7,pad9;
	FILE /**f7=NULL,*f9=NULL,*/*f=NULL;
	int i;

	for(i=0;i<ndslen-0x80;i+=4){
		if(!memcmp(nds+i,dsbmagic,12)){pA=(byte*)nds+i+0x28;break;}
	}
	if(!pA){/*fprintf(stderr,"not found DSBooter\n");*/return 1;}

	p9=pA+read32(pA+0x00);
	l9=read32(pA+0x08);
	fprintf(stderr,"Depth %d...\n",c++);
	fprintf(stderr,"p9=0x%08x\n",(u32)(p9-nds));
	fprintf(stderr,"l9=0x%08x\n",l9);
	if(!make(p9,l9,p))return 0;

	fprintf(stderr,"a9=0x%08x\n",a9=read32(pA+0x04));
	fprintf(stderr,"p7=0x%08x\n",(u32)((p7=pA+read32(pA+0x0c)+0x0c)-nds));
	fprintf(stderr,"l7=0x%08x\n",l7=read32(pA+0x14));
	fprintf(stderr,"a7=0x%08x\n",a7=read32(pA+0x10));

	pad9=0x100-(l9&0xff);if(pad9==0x100)pad9=0;
	pad7=0x100-(l7&0xff);if(pad7==0x100)pad7=0;

/*
	f9=fopen("tmp.9","wb");
	fwrite(p9,1,l9,f9);
	//if(pad9)fwrite(null256,1,pad9,f9);
	fclose(f9);
	f7=fopen("tmp.7","wb");
	fwrite(p7,1,l7,f7);
	//if(pad7)fwrite(null256,1,pad7,f7);
	fclose(f7);
*/

	if(p)f=fopen(p,"wb");
	else if(!isatty(fileno(stdout)))f=stdout;

	if(f){ //This buffer can be used as ldfBuf in ret_menu_Gen.
		//sfprintf(stderr,(arg,tp,p,a9,a7);
		//system(arg);
		write32(ndshead+0x24,a9);
		write32(ndshead+0x28,a9);
		write32(ndshead+0x2c,l9/*+pad9*/);
		write32(ndshead+0x30,l9+pad9+0x200);
		write32(ndshead+0x34,a7);
		write32(ndshead+0x38,a7);
		write32(ndshead+0x3c,l7/*+pad7*/);
		write32(ndshead+0x80,0x200+l9+pad9+l7+pad7);
		write16(ndshead+0x15e,crc16(0xffff,ndshead,0x15e));
		fwrite(ndshead,1,0x200,f);
		fwrite(p9,1,l9,f);
		if(pad9)fwrite(null256,1,pad9,f);
		fwrite(p7,1,l7,f);
		if(pad7)fwrite(null256,1,pad7,f);
		fclose(f);
	}

	return 0;
}

int m3make(const int argc, const char **argv){
	FILE *f;
	struct stat st;
	byte *p;
	unsigned char dec[512];

	if(argc<2){
		fprintf(stderr,"m3sakura_make v4\n");
		fprintf(stderr,"m3make in [>] [out]\n");
		return 1;
	}

	if(!(f=fopen(argv[1],"rb+"))){fprintf(stderr,"cannot open %s\n",argv[1]);return 1;}
	fstat(fileno(f),&st);
	if(!(p=malloc(st.st_size))){fprintf(stderr,"cannot allocate %d bytes for %s\n",(int)st.st_size,argv[1]);return 2;}
	fread(p,1,st.st_size,f);
	fclose(f);
	fprintf(stderr,"Decrypting %s... ",argv[1]);

	{
		int i=0,j;
		for(;i<0x100;i++){
			for(j=0xa0;j<0xa8;j++)
				dec[j]=p[j]^i;
			if(!memcmp(dec+0xa0,dsbmagic,8))
				{fprintf(stderr,"key = 0x%02x\n",i);break;}
		}
		if(i==0x100){fprintf(stderr,"Cannot decode or not DSBooter\n");return 3;}
		for(j=0x000;j<0x200;j++)
			p[j]=p[j]^i;
	}
	make(p,st.st_size,argc>2?argv[2]:NULL);
	free(p);
	return 0;
}
