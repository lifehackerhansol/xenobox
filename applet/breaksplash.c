#include <stdio.h>
#include <string.h>
#include "../lib/libmshlsplash.h"

int breaksplash(const int argc, const char **argv){
	char prefix[768],name[768];
	FILE *out;
	int i,j,curfile;
	u16 x;
	if(argc<3){
		fprintf(stderr,
			"BreakSplash - inverse of MakeSplash\n"
			"\tgbalzss by Haruhiko Okumura / Andre Perrot\n"
			"\tzlib by Jean-loup Gailly / Mark Adler\n"
			"breaksplash splash.ani dir [prefix]\n"
		);
		return 1;
	}
	strcpy(prefix,argc>3?argv[3]:"Splash");
	splash *p=opensplash(argv[1]);
	if(!p){fprintf(stderr,"cannot open splash");return 2;}
#ifdef WIN32
	mkdir(argv[2]);
#else
	mkdir(argv[2],0755);
#endif
	chdir(argv[2]);
	fprintf(stderr,"%s splash.\n",
		p->version==SPLASH_MOONSHELL1?"MoonShell1 (gbalzss)":
		p->version==SPLASH_MOONSHELL2?"MoonShell2 (zlib + diff encode)":
		p->version==SPLASH_ISAKUREAL?"iSakuReal (lzss varient)":
		p->version==SPLASH_M3SAKURA?"M3Sakura (raw)":
		""); //unreachable

	out=fopen("splash.ini","w");
	fprintf(out,
		"[ConvertSetting]\n"
		"SourceFileMask=%s???.bmp\n"
		"SourceFPS=%d\n"
		"WaitForTerminate=%d\n"
		"AlreadyAllDraw=%d\n"
		";breaksplashed images have no meaning to get dithered\n"
		"UseDither24to15bit=0\n",
		prefix,p->fps,(p->flags&1)?1:0,(p->flags&2)?1:0);
	fclose(out);

	for(curfile=0;curfile<p->frame;curfile++){
		u32 imagepixels;
		fprintf(stderr,"Decoding %03d / %03d\r",curfile,p->frame-1);
		decompressimage(p,curfile);
//		sprintf(name,"%s%03d.ppm",prefix,curfile);
		sprintf(name,"%s%03d.bmp",prefix,curfile);
		imagepixels=getimagesize(p,curfile)/2;
		out=fopen(name,"wb");
/*
		ffprintf(stderr,out,"P6\n%d %d\n255\n",256,imagepixels/256);
		for(i=0;i<imagepixels;i++){
			x=p->decompbuf[i];
			fputc(((x>>0)&0x1f)<<3,out);
			fputc(((x>>5)&0x1f)<<3,out);
			fputc(((x>>10)&0x1f)<<3,out);
		}
*/

		//an easy Windows header writer
		memset(buf,0,54);
		buf[0]='B',buf[1]='M';
		write32(buf+2,54+256*(imagepixels/256)*3);
		write32(buf+10,54);
		write32(buf+14,40);
		write32(buf+18,256);
		write32(buf+22,imagepixels/256);
		write16(buf+26,1);
		write16(buf+28,24);
		write32(buf+34,256*(imagepixels/256)*3);
		fwrite(buf,1,54,out);
		for(i=imagepixels/256-1;i>=0;i--){
			for(j=0;j<256;j++){
				x=p->decompbuf[i*256+j];
				//This order is OK; NDS uses BGR555.
				fputc(((x>>10)&0x1f)<<3,out);
				fputc(((x>>5)&0x1f)<<3,out);
				fputc(((x>>0)&0x1f)<<3,out);
			}
		}
		fclose(out);
	}
	closesplash(p);
	fprintf(stderr,"Decode done.          \n");
	return 0;
}
