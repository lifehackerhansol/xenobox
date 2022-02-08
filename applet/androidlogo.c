#include "../xenobox.h"
#include "../lib/libnsbmp.h"

static void *bitmap_create(int width, int height, unsigned int state){return malloc(width*height*4);}

static void bitmap_set_suspendable(void *bitmap, void *private_word,
			     void (*invalidate)(void *bitmap, void *private_word)){}

//static void invalidate(void *bitmap, void *private_word){}

static unsigned char *bitmap_get_buffer(void *bitmap){return bitmap;}

static size_t bitmap_get_bpp(void *bitmap){return 4;}

static void bitmap_destroy(void *bitmap){free(bitmap);}

int androidlogo(const int argc, const char **argv){
	if(argc<3){
		fprintf(stderr,
			"androidlogo rle/raw in.bmp >out.rle\n"
			"androidlogo width rle/raw out.bmp [16] <in.rle\n"
		);
		return -1;
	}

	int pixels=0,count=0,color,last;
	if(argc==3){ //encode
		struct stat st;
		int x,y;
		FILE *in=fopen(argv[2],"rb");
		if(!in){
			fprintf(stderr,"cannot open %s\n",argv[2]);
			return 1;
		}
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
		if(bmp_analyse(&gif, st.st_size, imgbuf)||bmp_decode(&gif)){bmp_finalise(&gif);free(imgbuf);return 1;}
		free(imgbuf);

		for(y=0;y<gif.height;y++){
			for(x=0;x<gif.width;x++){
				u32 coor=y*gif.width+x;
				u8 b=((((u32*)gif.bitmap)[coor]&0xff0000)>>16)&0xff;
				u8 g=((((u32*)gif.bitmap)[coor]&0x00ff00)>>8)&0xff;
				u8 r=((((u32*)gif.bitmap)[coor]&0x0000ff)>>0)&0xff;
				color=((r>>3)<<11)|((g>>2)<<5)|((b>>3)<<0);
				if(!strcasecmp(argv[1],"raw")){
					write16(buf,color);
					fwrite(buf,1,2,stdout);
				}else{
					if(count){
						if(color==last&&count<65535){
							count++;continue;
						}else{
							write16(buf,count);
							write16(buf+2,last);
							fwrite(buf,1,4,stdout);
						}
					}
					last=color;
					count=1;
				}
            }
        }
		if(count){
			write16(buf,count);
			write16(buf+2,color);
			fwrite(buf,1,4,stdout);
		}
		bmp_finalise(&gif);
	}else{ //decode
		int width=strtol(argv[1],NULL,0),height;
		if(!width||(width&3)){
			fprintf(stderr,"width must be 4x...\n");
			return 1;
		}
		FILE *out=fopen(argv[3],"wb");
		if(!out){
			fprintf(stderr,"cannot open %s\n",argv[3]);
			return 1;
		}
		int bytesPerPixel=3;
		if(argc>4)bytesPerPixel=2;
		memset(buf,0,54);
		fwrite(buf,1,54,out);
		for(;fread(buf,1,2,stdin)>1;){
			if(!strcasecmp(argv[2],"raw")){
				count=1;
				color=read16(buf);
			}else{
				count=read16(buf);
				fread(buf,1,2,stdin);
				color=read16(buf);
			}
			pixels+=count;
			for(;count;count--){
				if(bytesPerPixel==3){
					//transform RGB565 to BGR888
					fputc(((color>>0)&0x1f)<<3,out);
					fputc(((color>>5)&0x3f)<<2,out);
					fputc(((color>>11)&0x1f)<<3,out);
				}else{
					int col16=(((color>>11)&0x1f)<<10) | (((color>>6)&0x1f)<<5) | (((color>>0)&0x1f)<<0);
					write16(buf,col16);
					fwrite(buf,1,2,out);
				}
			}
		}
		height=pixels/width;
		fseek(out,0,SEEK_SET);
		buf[0]='B',buf[1]='M';
		write32(buf+2,54+width*height*bytesPerPixel);
		write32(buf+10,54);
		write32(buf+14,40);
		write32(buf+18,width);
		write32(buf+22,-height); //argh...
		write16(buf+26,1);
		write16(buf+28,bytesPerPixel*8);
		write32(buf+34,width*height*bytesPerPixel);
		fwrite(buf,1,54,out);
		fclose(out);
	}
	return 0;
}
