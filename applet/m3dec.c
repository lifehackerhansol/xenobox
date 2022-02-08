#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int m3dec(const int argc, const char **argv){
	int i=0,j,key;
	unsigned char enc[512],dec[512];
	FILE *in,*out;

	if(argc!=2&&argc!=4){fprintf(stderr,
		"m3dec in [out] [new-key]\n"
		"ex) to turn iTouchDS into R4iRTS:\n"
		"m3dec boot.eng loader.eng 0x72\n\n"
		"Note: if new-key format is invalid, 0 is used.\n");
	return 1;}

	in=fopen(argv[1],"rb");

	if(!in){fprintf(stderr,"Cannot open %s\n",argv[1]);return 2;}
	fprintf(stderr,"Decrypting %s... ",argv[1]);
	fread(enc,1,512,in);

	for(;i<0x100;i++){
		for(j=0xa0;j<0xa8;j++)
			dec[j]=enc[j]^i;
		if(!memcmp(dec+0xa0,"DSBooter",8))
			{fprintf(stderr,"key = 0x%02x\n",i);break;}
	}
	if(i==0x100){fclose(in);fprintf(stderr,"Cannot decode\n");return 3;}
	if(argc==2){fclose(in);return 0;}

	key=strtol(argv[3],NULL,0);
	//if(!key){fclose(in);fprintf(stderr,"new-key format invalid\n");return 4;}

	out=fopen(argv[2],"wb");

	if(!out){fclose(in);fprintf(stderr,"Cannot open %s\n",argv[2]);return 5;}
	for(j=0x000;j<0x200;j++)
		dec[j]=(enc[j]^i)^key;
	fwrite(dec,1,512,out);
	while(key=fread(enc,1,512,in))fwrite(enc,1,key,out);
	fclose(out);fclose(in);
	return 0;
}
