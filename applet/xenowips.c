#include "../xenobox.h"

static unsigned int current_end;
static void wipswrite(const u8 *p, const unsigned int address, const unsigned int _size, FILE *wips){
	u8 head[7];head[0]=1;
	unsigned int size=_size,patchofs=0;
	while(size){
		//fprintf(stderr,"write 0x%06x %dbytes\n",address+patchofs,min(size,255));
		write32(head+1,address+patchofs-current_end);
		write16(head+5,min(size,65535));
		fwrite(head,1,7,wips);
		fwrite(p+address+patchofs,1,min(size,65535),wips);
		patchofs+=min(size,65535);
		size-=min(size,65535);
	}
	current_end=address+patchofs;
}

static int wipsmake(const u8 *pfile, const unsigned int sizfile, const u8 *pnewfile, const unsigned int siznewfile, FILE *wips){
	current_end=0;
	memset(buf,0,0x28);
	strcpy(cbuf,"WQSG-PATCH3.0");
	write64(buf+0x10,sizfile);
	//write32(buf+0x18,0); //crc unused
	write32(buf+0x1c,0x28);
	//write64(buf+0x20,0); //comment unused

	unsigned int i=0,address,final;
	for(;i<sizfile;i++){
		if(pfile[i]!=pnewfile[i]){
			address=final=i;
			for(;memcmp_fast(pfile+final,pnewfile+final,min(5,sizfile-final));final++)i++;
			wipswrite(pnewfile,address,final-address,wips);
		}
	}
	fflush(wips);
	return 0;
}

static int wipspatch(u8 *pfile, unsigned int sizfile, const u8 *_pwips, const unsigned int _sizwips){ //pass pfile=NULL to get sizfile.
	if(_sizwips<0x29||memcmp(_pwips,"WQSG-PATCH3.0\0\0\0",16)||read32(_pwips+0x1c)!=0x28)return 2;
	//0x20-0x28 is comment offset, unused.

	//header check
	const u8 *pwips=_pwips;
	unsigned int sizwips=_sizwips;
	if(sizfile && sizfile!=read64(_pwips+0x10))return 2;

	//parse body from 0x28
	pwips+=0x28;
	const u8 *pwipsend=pwips+sizwips;
	u64 offset=0;
	u16 length=0;
	for(;;){
		if(pwips==pwipsend)return 0; //to fear something
		if(*pwips==0xff){
			return 0;
		}else if(*pwips==1){
			if(pwips+7>pwipsend)return 1;
			offset+=read32(pwips+1);
			length=read16(pwips+5);
			pwips+=7;
		}else if(*pwips==2){
			if(pwips+11>pwipsend)return 1;
			offset=read64(pwips+1);
			length=read16(pwips+9);
			pwips+=11;
		}

		if(pwips+length>pwipsend)return 1;
		if(pfile){
			memcpy(pfile+offset,pwips,length);
		}
		pwips+=length;
	}
	return 0; ///
}

//bootstrap
static int _wipsmake(FILE *file, FILE *newfile, FILE *wips){
	u8 *pfile=NULL,*pnewfile=NULL;
	unsigned int sizfile,siznewfile;
	//int ret;

	sizfile=filelength(fileno(file));
	siznewfile=filelength(fileno(newfile));
	if(sizfile!=siznewfile){
		fprintf(stderr,"file size not the same\n");return 2;
	}

	pfile=(u8*)malloc(sizfile);
	pnewfile=(u8*)malloc(siznewfile);
	if(!pfile||!pnewfile){
		if(pfile)free(pfile);
		if(pnewfile)free(pnewfile);
		fprintf(stderr,"cannot allocate memory\n");return 2;
	}
	fread(pfile,1,sizfile,file);
	fread(pnewfile,1,siznewfile,newfile);

	//fprintf(stderr,"(%u bytes / %u bytes)\n",sizfile,siznewfile);
	wipsmake(pfile,sizfile,pnewfile,siznewfile,wips);

	fprintf(stderr,"Made successfully\n");
	free(pfile);free(pnewfile);
	return 0;
}

static int _wipspatch(FILE *file, FILE *wips){
	u8 *pfile=NULL,*pwips=NULL;
	unsigned int sizfile,sizwips;
	int ret;

	if(~ftell(wips)){
		sizwips=filelength(fileno(wips));
		pwips=(u8*)malloc(sizwips);
		if(!pwips){
			fprintf(stderr,"cannot allocate memory\n");return 4;
		}
		fread(pwips,1,sizwips,wips);
	}else{ //if not file
		sizwips=0;
		pwips=NULL;
		for(;;){
			unsigned int readlen=fread(buf,1,BUFLEN,stdin);
			if(!readlen)break;
			u8 *tmp=(u8*)malloc(sizwips+readlen);
			if(!tmp){if(pwips)free(pwips);fprintf(stderr,"cannot allocate memory\n");return 4;}
			memcpy(tmp,pwips,sizwips);
			memcpy(tmp+sizwips,buf,readlen);
			if(pwips)free(pwips);
			pwips=tmp,tmp=NULL;
			sizwips+=readlen;
			if(readlen<BUFLEN)break;
		}
	}

	fprintf(stderr,"(WIPS %u bytes)\n",sizwips);
	ret=wipspatch(NULL,0,pwips,sizwips);
	switch(ret){
		case 0:
			sizfile=filelength(fileno(file));
			pfile=(u8*)malloc(sizfile);
			if(!pfile){
				free(pwips);
				fprintf(stderr,"cannot allocate memory\n");return 4;
			}
			fread(pfile,1,sizfile,file); //might occur error, but OK
			rewind(file);
			unsigned int crc=read32(pwips+0x18);
			if(crc && crc!=crc32(0,pfile,sizfile)){free(pfile);fprintf(stderr,"incorrect file\n");return 4;}
			if(!wipspatch(pfile,sizfile,pwips,sizwips))
				fwrite(pfile,1,sizfile,file);
			free(pfile);
			fprintf(stderr,"Patched successfully\n");
			break;
		case 2:
			fprintf(stderr,"\nwips not valid\n");
			break;
		case 1:
			fprintf(stderr,"\nPatch failed (corrupted / truncated)\n");
			break;
	}
	free(pwips);
	return ret;
}

//library bootstrap
int xenowips(const int argc, const char** argv){
	int flag,ret;
	FILE *file,*newfile=NULL,*wips;

	//startup
	if((argc<2||isatty(fileno(stdin)))&&argc<3){fprintf(stderr,
			"XenoWIPS - yet another wips maker/patcher v1\n"
			"Usage: xenowips file <wips\n"
			"       xenowips file wips\n"
			"       xenowips file newfile >wips\n"
			"       xenowips file newfile wips\n"
	);msleep(500);return 1;}

	//check running mode
	if(argc==2){
		file=fopen(argv[1],"r+b");if(!file)goto failfile;
		wips=stdin;
		flag=0x00;
	}else if(argc==3){
		if(isatty(fileno(stdout))){
			file=fopen(argv[1],"r+b");if(!file)goto failfile;
			wips=fopen(argv[2],"rb");if(!wips){fclose(file);goto failfile;}
			flag=0x01;
		}else{
			file=fopen(argv[1],"rb");if(!file)goto failfile;
			newfile=fopen(argv[2],"rb");if(!newfile){fclose(file);goto failfile;}
			wips=stdout;
			flag=0x10;
		}
	}else{
		file=fopen(argv[1],"rb");if(!file)goto failfile;
		newfile=fopen(argv[2],"rb");if(!newfile){fclose(file);goto failfile;}
		wips=fopen(argv[3],"wb");if(!wips){fclose(file);fclose(newfile);goto failfile;}
		flag=0x11;
	}

	if(flag&0x10){
		fprintf(stderr,"Source: %s\nTarget: %s\n",argv[1],argv[2]);
		ret=_wipsmake(file,newfile,wips);
		fclose(file);fclose(newfile);
		if(flag==0x11)fclose(wips);
		msleep(1000);return ret?ret|0x20:0;
	}else{
		fprintf(stderr,"File: %s\n",argv[1]);
		ret=_wipspatch(file,wips);
		fclose(file);
		if(flag==0x01)fclose(wips);
		msleep(1000);return ret?ret|0x10:0;
	}

failfile:
	fprintf(stderr,"Cannot open file\n");msleep(1000);return 2;
}
