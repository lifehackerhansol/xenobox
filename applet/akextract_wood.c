#include "../xenobox.h"

/*
[arm7]
dec      enc
00000001 00010000
00000010 00001000
00000100 10000000
00001000 01000000
00010000 00000010
00100000 00000001
01000000 00000100
10000000 00100000
[arm9] unknown...
*/

extern unsigned char ndshead[512];

static u8 null256[256];

static int search(const char *nds){
	int i=0,j,s,l7=0,l9=0;
	FILE *f=fopen(nds,"rb")/*,*f7,*f9*/;
	u8 *p;
	unsigned z,o,o7=0,o9=0,pad7,pad9;
	if(!f){fprintf(stderr,"Cannot open %s\nusage: akextract_wood [akmenu4.nds] [> akloader.nds]\n",nds);return 1;}
	s=filelength(fileno(f));
	p=malloc(s);
	if(!p){fclose(f);fprintf(stderr,"Cannot malloc %dbytes\n",s);return 2;}
	fread(p,1,s,f);
	fclose(f);
	o=read32(p+0x20),z=read32(p+0x24);
	for(;i<s;i+=4)
		if(read32(p+i)==0x023fa000){
			for(j=i+4;j<i+40;j+=4)
				if(read32(p+j)==0x02FFFE34)break; ///^^;
			if(j<i+40)break;
		}
	if(i>=s){free(p);fprintf(stderr,"Search error\n");return 3;}
	i-=8;
	fprintf(stderr,"Seeked to 0x%08x\n",i);
	for(j=i;j<i+48;j+=4){
		unsigned a=read32(p+j);
		if(0x02000000<=a&&a<=0x02000000+s){
			if(!(read32(p+a-z+o)&0xff000000)){
				if(!l7)fprintf(stderr,"l7=0x%08x\n",l7=read32(p+a-z+o));
				else if(!l9)fprintf(stderr,"l9=0x%08x\n",l9=read32(p+a-z+o));
			}else{
				if(!o7)fprintf(stderr,"o7=0x%08x\n",o7=a-z+o);
				else if(!o9)fprintf(stderr,"o9=0x%08x\n",o9=a-z+o);
			}
		}
	}
	if(!l7||!l9||!o7||!o9){free(p);fprintf(stderr,"Cannot find offset\n");return 4;}

	pad9=0x100-(l9&0xff);
	pad7=0x100-(l7&0xff);

if(isatty(fileno(stdout))){
/*
	f9=fopen("tmp.9","wb");
	fwrite(p+o9,1,l9,f9);
	//if(pad9)fwrite(null256,1,pad9,f9);
	fclose(f9);
	f7=fopen("tmp.7","wb");
	fwrite(p+o7,1,l7,f7);
	//if(pad7)fwrite(null256,1,pad7,f7);
	fclose(f7);
*/

		f=fopen("akloader.nds","wb");
}else{
		f=stdout;
}
		write32(ndshead+0x24,0x023c0000);
		write32(ndshead+0x28,0x023c0000);
		write32(ndshead+0x2c,l9/*+pad9*/);
		write32(ndshead+0x30,l9+pad9+0x200);
		write32(ndshead+0x34,0x023fa000);
		write32(ndshead+0x38,0x023fa000);
		write32(ndshead+0x3c,l7/*+pad7*/);
		write32(ndshead+0x80,0x200+l9+pad9+l7+pad7);
		write16(ndshead+0x15e,crc16(0xffff,ndshead,0x15e));
		fwrite(ndshead,1,0x200,f);
		fwrite(p+o9,1,l9,f);
		if(pad9)fwrite(null256,1,pad9,f);

		// now let's try to decrypt it.
		if((p[o7+3]&0xf0)!=0xe0){
			fprintf(stderr,"Let me try to decrypt arm7.\n");
			int i=0;
			for(;i<4;i++){
				p[o7+i]=
					(((p[o7+i]>>4)&1)<<0) | (((p[o7+i]>>3)&1)<<1) | (((p[o7+i]>>7)&1)<<2) | (((p[o7+i]>>6)&1)<<3) | 
					(((p[o7+i]>>1)&1)<<4) | (((p[o7+i]>>0)&1)<<5) | (((p[o7+i]>>2)&1)<<6) | (((p[o7+i]>>5)&1)<<7);
			}
			if((p[o7+3]&0xf0)!=0xe0){
				fprintf(stderr,"failed.\n");
				return 1;
			}
			for(;i<l7;i++){
				//if((i&3)==0 && p[o7+i]==0xff && p[o7+i+1]==0xff && p[o7+i+2]==0xff && p[o7+i+3]==0xff)break;
				p[o7+i]=
					(((p[o7+i]>>4)&1)<<0) | (((p[o7+i]>>3)&1)<<1) | (((p[o7+i]>>7)&1)<<2) | (((p[o7+i]>>6)&1)<<3) | 
					(((p[o7+i]>>1)&1)<<4) | (((p[o7+i]>>0)&1)<<5) | (((p[o7+i]>>2)&1)<<6) | (((p[o7+i]>>5)&1)<<7);
			}
		}
		fwrite(p+o7,1,l7,f);
		if(pad7)fwrite(null256,1,pad7,f);
if(isatty(fileno(stdout))){
		fclose(f);
}

	free(p);
	return 0;
}

int akextract_wood(const int argc, const char **argv){
	const char *nds=argc<2?"akmenu4.nds":argv[1];
	return search(nds);
}
