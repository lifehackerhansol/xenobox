#include "../xenobox.h"
#include <zlib.h>

typedef struct
{
	unsigned char magic[4];		/* +00 : 'D','A','X', 0                            */
	unsigned int total_bytes;	/* +04 : Original data size                        */
	unsigned int ver;		/* +08 : Version 0x00000001                        */
	unsigned int nNCareas;		/* +12 : Number of non-compressed areas            */
	unsigned int reserved[4];	/* +16 : Reserved for future uses                  */
} DAX_H;
#define DAX_HEADER_SIZE (0x20)
#define DAX_BLOCK_SIZE  (8192)

static int _compress(FILE *in, FILE *out, int level){
	fprintf(stderr,"compression won't be implemented\n");return -1;
}

static int _decompress(FILE *in, FILE *out){
	DAX_H header;
	fread(&header,1,sizeof(header),in);
	if(memcmp(header.magic,"DAX\0",4)){fprintf(stderr,"not DAX\n");return 1;}
	if(header.nNCareas){fprintf(stderr,"NC not supported\n");return 1;}
	int total_block=align2p(DAX_BLOCK_SIZE,header.total_bytes)/DAX_BLOCK_SIZE;
	fread(buf,4,total_block+1,in);
	int i=0;
	int counter=sizeof(header)+4*(total_block+1);
	for(;i<total_block;i++){
		u32 index=read32(buf+4*i);
		u32 size=read32(buf+4*(i+1)) - index;
		fread(__compbuf,1,size,in);counter+=size;

		//if(header.compression_algorithm){
			z_stream z;
			int status;

			z.zalloc = Z_NULL;
			z.zfree = Z_NULL;
			z.opaque = Z_NULL;

			if(inflateInit(&z) != Z_OK){
				fprintf(stderr,"inflateInit: %s\n", (z.msg) ? z.msg : "???");
				return 1;
			}

			z.next_in = __compbuf;
			z.avail_in = size;
			z.next_out = __decompbuf;
			z.avail_out = DAX_BLOCK_SIZE;

			status = inflate(&z, Z_FINISH);
			if(status != Z_STREAM_END && status != Z_OK){
				fprintf(stderr,"inflate: %s\n", (z.msg) ? z.msg : "???");
				return 10;
			}

			if(inflateEnd(&z) != Z_OK){
				fprintf(stderr,"inflateEnd: %s\n", (z.msg) ? z.msg : "???");
				return 2;
			}
			fwrite(__decompbuf,1,DAX_BLOCK_SIZE-z.avail_out,out);
		//}else{
		//	if(lzo1x_decompress_oneshot(decompbuf, DAX_BLOCK_SIZE-z.avail_out, compbuf, size)
		//		fprintf(stderr,"lzo decode error\n");
		//}
		if((i+1)%256==0)fprintf(stderr,"%d / %d\r",i+1,total_block);
	}
	fprintf(stderr,"%d / %d done.\n",i,total_block);
	return 0;
}

int xenodaxcr(const int argc, const char **argv){
	char mode,level;
	if(argc<2)goto argerror;
	mode=argv[1][0],level=argv[1][1];
	if(!mode)goto argerror;
	if(mode=='-')mode=argv[1][1],level=argv[1][2];
	if(mode!='e'&&mode!='c'&&mode!='d')goto argerror;
	if(isatty(fileno(stdin))||isatty(fileno(stdout)))goto argerror;

	return mode=='d'?_decompress(stdin,stdout):_compress(stdin,stdout,level?level-'0':9);

argerror:
	fprintf(stderr,
		"xenodaxcr e/d < in > out\n"
		"You can also use -e,-c,-d.\n"
		"compression won't be available...\n"
	);
	return -1;
}
