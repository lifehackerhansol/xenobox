#include "../xenobox.h"

static const byte *OverlayHeader_MS=(byte*)"OVRC MoonShell OverlayCode";
static const byte *OverlayHeader_FS=(byte*)"OVRC FishShell OverlayCode";
//extern unsigned char ndshead[512];
static const byte null256[256]={0};

int ovrcextract(const int argc, const char **argv){
	FILE *f;
	struct stat st;
	byte *p;
	const byte *magic=OverlayHeader_FS;

	if(argc<2){
		fprintf(stderr,"ovrcextract nds [-m] [>overlay.dll] [2>arm7_overlay.bin]\n");
		fprintf(stderr,"*** nds must not have nitrofs. it will be stripped. ***\n");
		return 1;
	}

	if(argc>2)magic=OverlayHeader_MS;

	if(!(f=fopen(argv[1],"rb+"))){fprintf(stderr,"cannot open %s\n",argv[1]);return 1;}
	fstat(fileno(f),&st);
	if(!(p=malloc(st.st_size))){fprintf(stderr,"cannot allocate %d bytes for %s\n",(int)st.st_size,argv[1]);return 2;}
	fread(p,1,st.st_size,f);

{
	byte *pA=NULL,*p7=NULL,*p9=NULL;
	u32 r7,r9,a7,a9,l7,l9,pad7,pad9,icon;
	int i;

	p9=p+read32(p+0x20);
	r9=read32(p+0x24);
	a9=read32(p+0x28);
	l9=read32(p+0x2c);
	p7=p+read32(p+0x30);
	r7=read32(p+0x34);
	a7=read32(p+0x38);
	l7=read32(p+0x3c);
	icon=read32(p+0x68);

	if(isRedirected(stdout)){
		pA=NULL;
		for(i=0;i<l9-0x80;i+=4){
			if(!memcmp(p9+i,magic,26)){
				pA=(byte*)p9+i;break;
			}
		}
		if(pA){
			fwrite(pA,1,l9-i,stdout);
			l9=i;
		}
	}
	if(isRedirected(stderr)){
		pA=NULL;
		for(i=0;i<l7-0x80;i+=4){
			if(!memcmp(p7+i,magic,26)){
				pA=(byte*)p7+i;break;
			}
		}
		if(pA){
			fwrite(pA,1,l7-i,stdout);
			l7=i;
		}
	}

	pad9=0x100-(l9&0xff);if(pad9==0x100)pad9=0;
	pad7=0x100-(l7&0xff);if(pad7==0x100)pad7=0;

	//delay truncate to avoid data loss
	rewind(f);
	ftruncate(fileno(f),0);

#define HEAD p
//#define HEAD ndshead
	write32(HEAD+0x20,0x200);
	write32(HEAD+0x24,r9);
	write32(HEAD+0x28,a9);
	write32(HEAD+0x2c,l9/*+pad9*/);
	write32(HEAD+0x30,l9+pad9+0x200);
	write32(HEAD+0x34,r7);
	write32(HEAD+0x38,a7);
	write32(HEAD+0x3c,l7/*+pad7*/);
	write32(HEAD+0x68,icon?(0x200+l9+pad9+l7+pad7):0);
	write32(HEAD+0x80,0x200+l9+pad9+l7+pad7+(icon?2112:0));
	write16(HEAD+0x15e,crc16(0xffff,HEAD,0x15e));
	fwrite(HEAD,1,0x200,f);
#undef HEAD
	fwrite(p9,1,l9,f);
	if(pad9)fwrite(null256,1,pad9,f);
	fwrite(p7,1,l7,f);
	if(pad7)fwrite(null256,1,pad7,f);
	if(icon)fwrite(p+icon,1,2112,f);
}

	free(p);
	fclose(f);
	return 0;
}
