#include "../xenobox.h"
#include "../lib/libnsbmp.h"

static void *bitmap_create(int width, int height, unsigned int state){return malloc(width*height*4);}

static void bitmap_set_suspendable(void *bitmap, void *private_word,
			     void (*invalidate)(void *bitmap, void *private_word)){}

//static void invalidate(void *bitmap, void *private_word){}

static unsigned char *bitmap_get_buffer(void *bitmap){return bitmap;}

static size_t bitmap_get_bpp(void *bitmap){return 4;}

static void bitmap_destroy(void *bitmap){free(bitmap);}

static char bak[768];
static int convert(const char *name, int dither){
	struct stat st;
	int x,y;
	FILE *in=fopen(name,"rb");
	if(!in)return -1;
	fstat(fileno(in),&st);
	void *imgbuf=malloc(st.st_size);
	fread(imgbuf,1,st.st_size,in);
	fclose(in);

	bmp_image gif;
	bmp_bitmap_callback_vt vt={
		bitmap_create,
		bitmap_destroy,
		bitmap_set_suspendable,
		bitmap_get_buffer,
		bitmap_get_bpp
	};

	bmp_create(&gif,&vt);
	if(bmp_analyse(&gif, st.st_size, imgbuf)||gif.width!=40||gif.height!=42||bmp_decode(&gif)){bmp_finalise(&gif);free(imgbuf);return 1;}

	free(imgbuf);
	strcpy(bak,name);
	strcpy(bak+strlen(bak)-4,".bak");
	rename(name,bak);
	FILE *out=fopen(name,"wb");

	//an easy Windows header writer
	memset(buf,0,54);
	buf[0]='B',buf[1]='M';
	write32(buf+2,54+align4(32*2)*32);
	write32(buf+10,54);
	write32(buf+14,40);
	write32(buf+18,32);
	write32(buf+22,32);
	write16(buf+26,1);
	write16(buf+28,16);
	write32(buf+34,align4(32*2)*32);
	fwrite(buf,1,54,out);

	for(y=37;y>=6;y--){
		for(x=4;x<36;x++){
			u32 coor=y*gif.width+x;
			u8 b=((((u32*)gif.bitmap)[coor]&0xff0000)>>16)&0xff;
			u8 g=((((u32*)gif.bitmap)[coor]&0x00ff00)>>8)&0xff;
			u8 r=((((u32*)gif.bitmap)[coor]&0x0000ff)>>0)&0xff;
			//if(dither){
			//	r=DitherTable4(r,x,y);g=DitherTable4(g,x,y);b=DitherTable4(b,x,y);
			//}
			write16(buf,(0<<15)|((r>>3)<<10)|((g>>3)<<5)|((b>>3)<<0));
			fwrite(buf,1,2,out);
		}
		if(gif.width&1){
			write16(buf,0);
			fwrite(buf,1,2,out);
		}
	}
	fclose(out);
	bmp_finalise(&gif);

	return 0;
}

int dstwo2ismm(const int argc, const char **argv){
	int dither=0,i=1;
	if(argc==1)goto usage;
	//if(!strcmp(argv[1],"-d")){
	//	dither=1;i=2;
	//	if(argc==2)goto usage;
	//}
	for(;i<argc;i++){
		fprintf(stderr,"Converting %s... ",argv[i]);
		fprintf(stderr,convert(argv[i],dither)?"Failed.\n":"Done.\n");
	}
	fprintf(stderr,"Press ENTER key.");getchar();
	return 0;

usage:
	fprintf(stderr,
		"DSTWO 2 iSMM\n"
		"dstwo2ismm bmps...\n"
		"Press ENTER key."
	);getchar();
	return 0;
}
