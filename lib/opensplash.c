#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "libmshlsplash.h"

int zlib_decompress(const void *compbuf, const u32 compsize, void *decompbuf, const u32 decompsize){
	z_stream z;
	int status;

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;

	if(inflateInit(&z) != Z_OK){
		fprintf(stderr,"inflateInit: %s\n", (z.msg) ? z.msg : "???");
		return(-1);
	}

	z.next_in = (u8*)compbuf;
	z.avail_in = compsize;
	z.next_out = (u8*)decompbuf;
	z.avail_out = decompsize;
	for(;;){
		status = inflate(&z, Z_NO_FLUSH);
		if(status == Z_STREAM_END)break;
		if(status != Z_OK){
			fprintf(stderr,"inflate: %s\n", (z.msg) ? z.msg : "???");
			return(-1);
		}
		if(z.avail_out == 0){
			fprintf(stderr,"deflate buffer overflow.\n");
			return(-1);
		}
	}
    
	u32 decsize=decompsize-z.avail_out;

	if(inflateEnd(&z) != Z_OK){
		fprintf(stderr,"inflateEnd: %s\n", (z.msg) ? z.msg : "???");
		return(-1);
	}
	return(decsize);
}

int zlib_compress(const void *decompbuf, const u32 decompsize, void *compbuf, const u32 compsize){
	z_stream z;
	int status;

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;

	if(deflateInit(&z,9) != Z_OK){
		fprintf(stderr,"deflateInit: %s\n", (z.msg) ? z.msg : "???");
		return(-1);
	}

	z.next_in = (u8*)decompbuf;
	z.avail_in = decompsize;
	z.next_out = (u8*)compbuf;
	z.avail_out = compsize;
	for(;;){
		status = deflate(&z, Z_FINISH);
		if(status == Z_STREAM_END)break;
		if(status != Z_OK){
			fprintf(stderr,"deflate: %s\n", (z.msg) ? z.msg : "???");
			return(-1);
		}
		if(z.avail_out == 0){
			fprintf(stderr,"inflate buffer overflow.\n");
			return(-1);
		}
	}
    
	u32 encsize=compsize-z.avail_out;

	if(deflateEnd(&z) != Z_OK){
		fprintf(stderr,"inflateEnd: %s\n", (z.msg) ? z.msg : "???");
		return(-1);
	}
	return(encsize);
}

int closesplash(splash *p){
	if(!p)return -1;
	if(p->sig!=libmshlsplash_signature)return -1;
	fclose(p->f);
	free(p->compbuf);
	free(p->decompbuf);
	free(p->firstbuf);
	free(p->head);
	free(p);
	return 0;
}

splash* opensplash(const char *path){
	int i=0,filesize,size1total=0,size2total=0,flag1=0,flag2=0;

	splash *p=(splash*)malloc(sizeof(splash));
	if(!p)return NULL;
	p->f=fopen(path,"rb");
	p->compbuf=(u16*)malloc(256*192*2);
	p->decompbuf=(u16*)malloc(256*192*2);
	p->firstbuf=(u16*)malloc(256*192*2);
	if(!p->f||!p->compbuf||!p->decompbuf||!p->firstbuf){
		if(p->f)fclose(p->f);
		if(p->compbuf)free(p->compbuf);
		if(p->decompbuf)free(p->decompbuf);
		if(p->firstbuf)free(p->firstbuf);
		free(p);return NULL;
	}
	p->sig=libmshlsplash_signature;
	filesize=filelength(fileno(p->f));
	if(fread(&p->flags,1,4,p->f)<4||fread(&p->frame,1,4,p->f)<4||p->frame>999||!(p->head=(splash_frame*)malloc(16*p->frame))){
		fclose(p->f);
		free(p->compbuf);
		free(p->decompbuf);
		free(p->firstbuf);
		free(p);return NULL;
	}

	if(p->flags==0x496e614d){ //M3Sakura
		if(p->frame>126){closesplash(p);return NULL;}
		for(i=0;i<p->frame;i++){
			p->fps=15; // ^^;
			p->head[i].vsync=i*60/p->fps;
			fread(&p->head[i].offset,1,4,p->f);
			p->head[i].size1=p->head[i].size2=0x18000;
		}
		p->flags=2;
		p->version=SPLASH_M3SAKURA;
		return p;
	}

	if(fread(p->head,16,p->frame,p->f)<p->frame){
		closesplash(p);
		return NULL;
	}
	size1total=size2total=8+16*p->frame;
	for(i=0;i<p->frame;i++){
		int size1=p->head[i].size1;
		int size2=p->head[i].size2*(i==0?1:2);
		if(p->head[i].size1>0xc000)flag1=1;
		if(p->head[i].size2>0x18000)flag2=1;
		size1total+=size1;
		size2total+=size2;
	}
	p->fps=60*(p->frame-1)/p->head[p->frame-1].vsync;

	fprintf(stderr,"size1=%d size2=%d real=%d\n",size1total,size2total,filesize);
	if(size1total==filesize){
		if(flag2){
			closesplash(p);
			return NULL;
		}
		fseek(p->f,p->head[0].offset,SEEK_SET);
		fread(p->compbuf,1,4,p->f);
		if((read32(p->compbuf)&0xffffff00)){
			p->version=SPLASH_MOONSHELL1;
			return p;
		}
		p->version=SPLASH_ISAKUREAL;
		return p;
	}
	if(size2total==filesize){
		if(flag1){
			closesplash(p);
			return NULL;
		}
		fseek(p->f,p->head[0].offset,SEEK_SET);
		fread(p->compbuf,1,p->head[0].size2,p->f);
		zlib_decompress(p->compbuf,p->head[0].size2,p->firstbuf,p->head[0].size1*2); //Cache first
		p->version=SPLASH_MOONSHELL2;
		return p;
	}
	closesplash(p);
	return NULL;
}

int decompressimage(splash* p, int idx){
	if(!p)return -1;
	if(p->sig!=libmshlsplash_signature)return -1;
	if(idx<0||p->frame<=idx)return -1;
	switch(p->version){
		case SPLASH_MOONSHELL1:{
			memstream mcomp,mdecomp;
			fseek(p->f,p->head[idx].offset,SEEK_SET);
			fread(p->compbuf,1,p->head[idx].size1,p->f);
			mopen(p->compbuf,p->head[idx].size1,&mcomp);
			mopen(p->decompbuf,p->head[idx].size2,&mdecomp);
			gbalzssDecode(&mcomp,&mdecomp);
			return 0;
		}
		case SPLASH_ISAKUREAL:{
			memstream mcomp,mdecomp,mdecompbuf;
			u32 palcnt;
			fseek(p->f,p->head[idx].offset,SEEK_SET);
			fread(p->compbuf,1,p->head[idx].size1,p->f);
			palcnt=read32(p->compbuf);
			mopen(p->compbuf+2+palcnt,p->head[idx].size1,&mcomp);
			mopen(p->decompbuf,p->head[idx].size2,&mdecomp);
			mopen(p->decompbuf,p->head[idx].size2,&mdecompbuf);
			if(iSakuRealDecode(&mcomp,&mdecomp,&mdecompbuf,palcnt,p->compbuf+2))return 1;
			return 0;
		}
		case SPLASH_MOONSHELL2:{
			memcpy(p->decompbuf,p->firstbuf,p->head[0].size1*2);
			if(idx>0){
				int i=0,dest=0;
				fseek(p->f,p->head[idx].offset,SEEK_SET);
				fread(p->compbuf,1,p->head[idx].size2*2,p->f);
				for(;i<p->head[idx].size2;i++){
					if(!(p->compbuf[i]&0x8000))dest+=p->compbuf[i];
					else p->decompbuf[dest]=p->compbuf[i],dest++;
				}
			}
			return 0;
		}
		case SPLASH_M3SAKURA:{
			fseek(p->f,p->head[idx].offset,SEEK_SET);
			fread(p->compbuf,1,0x18000,p->f);
			memcpy(p->decompbuf,p->compbuf,0x18000);
			return 0;
		}
	}
	return -1;
}

u32 getimagesize(splash* p, int idx){
	if(!p)return -1;
	if(p->sig!=libmshlsplash_signature)return -1;
	if(idx<0||p->frame<=idx)return -1;
	switch(p->version){
		case SPLASH_MOONSHELL1:
		case SPLASH_ISAKUREAL:{
			return p->head[idx].size2;
		}
		case SPLASH_MOONSHELL2:{
			return p->head[idx].size1*2;
		}
		case SPLASH_M3SAKURA:{
			return 0x18000;
		}
	}
	return -1;
}
