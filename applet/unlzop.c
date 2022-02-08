/*
  unlzop - LZOP decoder under public domain
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../lib/libmshlsplash.h"
#include "unlzop.h"

/* lzo1xy_decompress - easy replacement for lzo1x_dec* */
int lzo1xy_decompress(memstream *mdec, memstream *menc, int lzo1y){
	int c=0,arg1,arg2,count,back,state=0,error=0;

	if(!mdec||!menc||!mavail(mdec)||!mavail(menc))return -1;
	c=mgetc(menc);
	if(c>17){
		if(mcopy(mdec,c-17,menc)<c-17){error=1;goto done;}
		if((c=mgetc(menc))==EOF){error=1;goto done;}
		if(c<16){error=1;goto done;}
	}
	while(!error){
		if(c>63){
			if((arg1=mgetc(menc))==EOF){error=1;break;}
			if(lzo1y){
				count=(c>>4)-3;
				back=((c>>2)&3)+(arg1<<2)+1;
			}else{
				count=(c>>5)-1;
				back=((c>>2)&7)+(arg1<<3)+1;
			}
		}else if(c>31){
			count=c&31;
			if(!count){ //c==32
				for(;;){
					arg1=mgetc(menc);
					if(arg1==EOF){error=-1;goto done;}
					if(arg1)break;
					count+=255;
				}
				count+=arg1+31;
			}
			if((arg1=mgetc(menc))==EOF || (arg2=mgetc(menc))==EOF){error=1;break;}
      			back=(arg1>>2)+(arg2<<6)+1;
			c=arg1;
		}else if(c>15){
			count=c&7;
			if(!count){ //c==16,24
				for(;;){
					arg1=mgetc(menc);
					if(arg1==EOF){error=-1;goto done;}
					if(arg1)break;
					count+=255;
				}
				count+=arg1+7;
			}
			if((arg1=mgetc(menc))==EOF || (arg2=mgetc(menc))==EOF){error=1;break;}
			back=(1<<14)+((c&8)<<11);
			back+=(arg1>>2)+(arg2<<6);
			c=arg1;
			if(back==1<<14){
				if(count!=1)error=1;
				break;
			}
		}else if(!state){
			count=c&15;
			if(!count){
				for(;;){
					arg1=mgetc(menc);
					if(arg1==EOF){error=-1;goto done;}
					if(arg1)break;
					count+=255;
				}
				count+=arg1+15;
			}
			if(mcopy(mdec,count+3,menc)<count+3){error=1;break;}
			if((c=mgetc(menc))==EOF){error=1;break;}
			if(c>15)continue;
			count=1;
			if((arg1=mgetc(menc))==EOF){error=1;break;}
			back=(1<<11)+(c>>2)+(arg1<<2)+1;
		}else{
			count=0;
			if((arg1=mgetc(menc))==EOF){error=1;break;}
			back=(c>>2)+(arg1<<2)+1;
		}
		if(mtell(mdec)<back||back<=0){error=2;break;}
		mwrite(
			(unsigned char*)(mdec->p)+mtell(mdec)-back,
			count+2,mdec);
		state=count=c&3;
		if(mcopy(mdec,count,menc)<count){error=1;break;}
		if((c=mgetc(menc))==EOF){error=1;break;}
	}
done:
	return error;
}

int lzo1x_decompress_oneshot(char* o_buff, int o_size, const char* i_buff, int i_size){
	memstream in,out;
	mopen((char*)i_buff, i_size, &in);
	mopen(o_buff, o_size, &out);
	return lzo1xy_decompress(&out, &in, 0)?-1:0;
}

static inline int reader_fread32(reader *r, unsigned int *x){
	unsigned char b[4];
	int e;
	if(!r)return -1;
	e=fread(b,1,4,r->f);
	r->adler32=adler32(r->adler32,b,4);
	r->crc32=crc32(r->crc32,b,4);
	if(x)*x=(b[0]<<24)+(b[1]<<16)+(b[2]<<8)+b[3];
	return e;
}

static inline int reader_fread16(reader *r, unsigned short *x){
	unsigned char b[2];
	int e;
	if(!r)return -1;
	e=fread(b,1,2,r->f);
	r->adler32=adler32(r->adler32,b,2);
	r->crc32=crc32(r->crc32,b,2);
	if(x)*x=(b[0]<<8)+b[1];
	return e;
}

static inline int reader_fread8(reader *r, unsigned char *x){
	unsigned char b;
	int e;
	if(!r)return -1;
	e=fread(&b,1,1,r->f);
	r->adler32=adler32(r->adler32,&b,1);
	r->crc32=crc32(r->crc32,&b,1);
	if(x)*x=b;
	return e;
}

static inline int reader_freadstr(reader *r, unsigned char *p, int len){
	int e;
	if(!r||!p)return -1;
	e=fread(p,1,len,r->f);
	r->adler32=adler32(r->adler32,p,len);
	r->crc32=crc32(r->crc32,p,len);
	return e;
}

int decode_stdin(){
	unsigned char buf[11],l;
	header h;
	reader r;
	unsigned int cs1,cs2;

	memset(&h,0,sizeof(header));
	memset(&r,0,sizeof(reader));
	h.exver=V1;
	h.level=0;
	r.f=stdin;

	//header
	if(fread(buf,1,9,stdin)<9)return 1;
	if(!(
		buf[0]==0x89 && buf[1]==0x4c && buf[2]==0x5a && buf[3]==0x4f && buf[4]==0x00 &&
		buf[5]==0x0d && buf[6]==0x0a && buf[7]==0x1a && buf[8]==0x0a))return 2;

	//need to start hashing...
	r.adler32=1;
	r.crc32=0;
	if(reader_fread16(&r,&h.ver)<2)return 1;
	if(h.ver<V1)return 2;
	if(reader_fread16(&r,&h.libver)<2)return 1;
	if(h.ver>=V2){
		if(reader_fread16(&r,&h.exver)<2)return 1;
		if(h.exver<V1)return 2;
	}
	if(reader_fread8(&r,&h.method)<1)return 1;
	if(0x10<=h.method&&h.method<=0x2f)return -1; //nrv is not supported.
	if(h.ver>=V2)
		if(reader_fread8(&r,&h.level)<1)return 1;
	if(reader_fread32(&r,&h.flags)<4)return 1;
	h.os=h.flags>>24;
	if(h.flags&F_H_FILTER){
		if(reader_fread32(&r,&h.filter)<4)return 1;
		if(h.filter<17)return 2;
	}
	if(reader_fread32(&r,&h.mode)<4)return 1;
	if(h.flags&F_STDIN)h.mode=0;
	if(reader_fread32(&r,&h.mtlow)<4)return 1;
	if(h.ver>=V2){
		if(reader_fread32(&r,&h.mthigh)<4)return 1;
		if(h.ver<0x1020){
			if(h.mtlow==-1)h.mtlow=0;
			h.mthigh=0;
		}
	}
	if(reader_fread8(&r,&l)<1)return 1;
	if(l)
		if(reader_freadstr(&r,(unsigned char*)&h.name,l)<l)return 1;
	cs1=h.flags&F_H_CRC32?r.crc32:r.adler32;
	if(reader_fread32(&r,&cs2)<4)return 1;
	if(cs1!=cs2)return 2;
	if(h.flags&F_H_EXTRA_FIELD){
		unsigned int u;
		r.adler32=1;
		r.crc32=0;
		if(reader_fread32(&r,&u)<4)return 1;
		for(;u--;)if(reader_fread8(&r,NULL)<1)return 1;
		cs1=h.flags&F_H_CRC32?r.crc32:r.adler32;
		if(reader_fread32(&r,&cs2)<4)return 1;
		if(cs1!=cs2)return 2;
	}
	if(h.flags&0x000fc000)return 2;

	//body. let's decompress stream.
	for(;;){
		unsigned int dlen,clen,dadler32=1,dcrc32=0,cadler32=1,ccrc32=0,d;
		unsigned char *pc,*pd,b[16];
		int e;

		if(reader_fread32(&r,&dlen)<4)return 1;
		if(dlen==0)break;
		if(reader_fread32(&r,&clen)<4)return 1;
		if(h.flags&F_ADLER32_D)
			if(reader_fread32(&r,&dadler32)<4)return 1;
		if(h.flags&F_CRC32_D)
			if(reader_fread32(&r,&dcrc32)<4)return 1;
		if(dlen>clen){
			if(h.flags&F_ADLER32_C)
				if(reader_fread32(&r,&cadler32)<4)return 1;
			if(h.flags&F_CRC32_C)
				if(reader_fread32(&r,&ccrc32)<4)return 1;
		}else{
			cadler32=dadler32; ccrc32=dcrc32;
		}

		pc=(unsigned char*)malloc(clen);
		r.adler32=1;
		r.crc32=0;
		if(reader_freadstr(&r,pc,clen)<clen){free(pc);return 1;}
		if(h.flags&F_ADLER32_C && r.adler32!=cadler32){free(pc);return 2;}
		if(h.flags&F_CRC32_C && r.crc32!=ccrc32){free(pc);return 2;}
		if(dlen==clen){
			if(h.flags&F_ADLER32_D && r.adler32!=dadler32){free(pc);return 2;}
			if(h.flags&F_CRC32_D && r.crc32!=dcrc32){free(pc);return 2;}
			pd=pc;
		}else{
			unsigned int tmp=dlen;
			//unsigned int cs;
			pd=(unsigned char*)malloc(dlen);
			memstream *mc=mopen(pc,clen,NULL),*md=mopen(pd,dlen,NULL);
			e=h.method==0x80?uncompress(pd,(unsigned long*)&tmp,pc,clen):lzo1xy_decompress(md,mc,0);
			mclose(mc),mclose(md);
			free(pc);
			if(e){free(pd);return e;}
			//if(d!=(h.method==0x80?dlen:0)){free(pd);return 3;}
			//if(h.flags&F_ADLER32_D && dadler32!=adler32(1,pd,dlen)){free(pd);return 2;}
			//if(h.flags&F_CRC32_D && dcrc32!=crc32(0,pd,dlen)){free(pd);return 2;}
		}

		if(h.flags&F_H_FILTER){ //process pd with special filter
			pc=pd;
			memset(b,0,h.filter); //init
			for(e=d=0;d<dlen;pc++,e++,d++){
				if(e>=h.filter)e=0; //rotate
				*pc-=b[e],b[e]+=*pc;
			}
		}

		fwrite(pd,1,dlen,stdout);
		free(pd);
	}

	return 0;
}

int unlzop(const int argc, const char **argv){
	int i;
	if(isatty(fileno(stdin))||isatty(fileno(stdout))){
		//fprintf(stderr,"unlzop V.05.090203 Final - LZOP decoder under public domain\n");
		fprintf(stderr,"unlzop 110421 - LZOP decoder under public domain\n");
		fprintf(stderr,"Both stdin and stdout have to be redirected\n");return -2;
	}
	i=decode_stdin();
	switch(i){
		case 0: fprintf(stderr,"Everything is Ok\n");break;
		case 1: fprintf(stderr,"Unexpected EOF\n");break;
		case 2: fprintf(stderr,"File Corrupted\n");break;
		case 3: fprintf(stderr,"Decode Error\n");break;
		case -1: fprintf(stderr,"NRV is not supported\n");break;
		default: fprintf(stderr,"Unknown Error\n");
	}
	return i;
}
