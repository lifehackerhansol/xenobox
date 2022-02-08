#include "../xenobox.h"

int xenondstrim(const int argc, const char **argv){
	int i=1;//2;
	int j;

	FILE *f;
	//struct stat st;

	fprintf(stderr,"xenondstrim\n");
	if(argc<i+1){fprintf(stderr,"Usage: xenondstrim rom...\n");return -1;}
	for(;i<argc;i++){
		fprintf(stderr,"%s: ",argv[i]);
		f=fopen(argv[i],"rb+");
		if(!f){fprintf(stderr,"Cannot open\n");continue;}
		u32 size=filelength(fileno(f));
		if(size<0x1000){fclose(f);fprintf(stderr,"Not valid rom\n");continue;}
		fread(buf,1,0x1000,f);
		fprintf(stderr,"\n");

		u32 array[16];
		fprintf(stderr,"arm9 end:        %08x\n",array[0]=read32(buf+0x20)+read32(buf+0x2c));
		fprintf(stderr,"arm7 end:        %08x\n",array[1]=read32(buf+0x30)+read32(buf+0x3c));
		fprintf(stderr,"FNT end:         %08x\n",array[2]=read32(buf+0x40)+read32(buf+0x44));
		fprintf(stderr,"FAT end:         %08x\n",array[3]=read32(buf+0x48)+read32(buf+0x4c));
		fprintf(stderr,"arm9overlay end: %08x\n",array[4]=read32(buf+0x50)+read32(buf+0x54));
		fprintf(stderr,"arm7overlay end: %08x\n",array[5]=read32(buf+0x58)+read32(buf+0x5c));
		fprintf(stderr,"icon end:        %08x\n",array[6]=read32(buf+0x68)+2112);
		fprintf(stderr,"NTR size:        %08x\n",array[7]=read32(buf+0x80));
		fprintf(stderr,"arm9i end:       %08x\n",array[8]=read32(buf+0x1c0)+read32(buf+0x1cc));
		fprintf(stderr,"arm7i end:       %08x\n",array[9]=read32(buf+0x1d0)+read32(buf+0x1dc));
		fprintf(stderr,"Digest NTR end:  %08x\n",array[10]=read32(buf+0x1e0)+read32(buf+0x1e4));
		fprintf(stderr,"Digest TWL end:  %08x\n",array[11]=read32(buf+0x1e8)+read32(buf+0x1ec));
		fprintf(stderr,"Digest SecH end: %08x\n",array[12]=read32(buf+0x1f0)+read32(buf+0x1f4));
		fprintf(stderr,"Digest BloH end: %08x\n",array[13]=read32(buf+0x1f8)+read32(buf+0x1fc));
		fprintf(stderr,"Modcrypt1 end:   %08x\n",array[14]=read32(buf+0x220)+read32(buf+0x224));
		fprintf(stderr,"Modcrypt2 end:   %08x\n",array[15]=read32(buf+0x228)+read32(buf+0x22c));
		u32 s=array[0];
		for(j=1;j<16;j++)if(s<array[j])s=array[j];
		if(size>s){
			fprintf(stderr,"Trimmed %08x -> %08x\n",size,s);
			ftruncate(fileno(f),s);
		}else{
			fprintf(stderr,"%08x -> %08x, cannot trim\n",size,s);
		}
		fclose(f);
	}
	return 0;
}
