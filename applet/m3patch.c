#include "../xenobox.h"
static u8 patch[8];
static u8 check[512][64];
static int checksize[512];
static int n=0;

static int _memcmp(void *p, void *q, int n){
	int i=0,x;
	for(;i<n;i++)if(x=*((u8*)p+i)-*((u8*)q+i))return x;
	return 0;
}

static void _m3patch(u8 *p, int size){
	int i=0,j;
	for(;i<size-15;i+=4){
		if(!(i&0xfff))fprintf(stderr,"%7d/%7d\r",i,size);
		for(j=0;j<n;j++)
			if(!_memcmp(p+i,check[j],checksize[j])){
				printf("Detected at 0x%08x (%d)\n",i,j+1);
				memcpy(p+i,patch,8);
				i+=checksize[j]-4;
				j=n;
				break;
			}
	}
	printf("%7d/%7d Done.\n",size,size);
}

static int txt2bin(const char *src, u8 *dst){
	int i=0;
	for(;i<64;i++){
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
	if(i<8||i&3){printf("m3p line length must be 8,12,16,20,...\n");exit(-1);}
	return i;
//puts("");
}

int m3patch(const int argc, const char **argv){
	struct stat st;
	int i=2;
	u8 *p;
	FILE *f;

	char z[512];

	memset(patch,0,sizeof(patch));
	memset(check,0,sizeof(check));
	memset(checksize,0,sizeof(checksize));

	if(argc<3){printf("m3patch patch.m3p file...\n");return 1;}

	f=fopen(argv[1],"r");
	if(!f||!myfgets(z,512,f)){if(f)fclose(f);printf("Cannot open patch\n");return 2;}
	txt2bin(z,patch);

	while(myfgets(z,512,f))checksize[n]=txt2bin(z,check[n]),n++;

	for(i=2;i<argc;i++){
		printf("Patching %s...",argv[i]);
		f=fopen(argv[i],"rb+");
		if(!f){printf(" Cannot open.\n");continue;}
		fstat(fileno(f),&st);
		p=(u8*)malloc(st.st_size);
		fread(p,1,st.st_size,f);
		rewind(f);
		printf("\n");
		_m3patch(p,st.st_size);
		fwrite(p,1,st.st_size,f);
		fclose(f);
	}
	return 0;
}
