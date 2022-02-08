#include "../xenobox.h"

/*
 * XenoPatch driver
 * Now supports IPS/PPF/UPF/WIPS
 * xdelta and bsdiff are delta format, so they won't be supported.
 * Moreover, xdelta.exe is much more reliable than ips patchers.
*/

static int RLE(const u8 *p, const unsigned int size, const unsigned int check, unsigned int *length){ //if>8bytes, fill operation will be used.
	int i=0,j;
	if(size<3||check<3)return -1;
	for(;i<size-check;i++){
		for(j=i+1;j<size;j++)
			if(p[i]!=p[j])break;
		if(j>=i+check){*length=j-i;return i;}
	}
	return -1;
}

static void ipswrite(const u8 *p, const unsigned int address, const unsigned int _size, FILE *ips){
	u8 head[5];
	unsigned int size=_size,patchofs=0;
	while(size){
		fprintf(stderr,"write 0x%06x %dbytes\n",address+patchofs,min(size,65535));
		write24be(head,address+patchofs);
		write16be(head+3,min(size,65535));
		fwrite(head,1,5,ips);
		fwrite(p+address+patchofs,1,min(size,65535),ips);
		patchofs+=min(size,65535);
		size-=min(size,65535);
	}
}

static void ipsfill(const u8 p, const unsigned int address, const unsigned int _size, FILE *ips){
	u8 head[7];
	unsigned int size=_size,patchofs=0;
	while(size){
		fprintf(stderr,"fill  0x%06x %dbytes\n",address+patchofs,min(size,65535));
		write24be(head,address+patchofs);
		head[3]=head[4]=0;
		write16be(head+5,min(size,65535));
		fwrite(head,1,7,ips);
		fwrite(&p,1,1,ips);
		patchofs+=min(size,65535);
		size-=min(size,65535);
	}
}

//libxenoips
static int ipsmake(const u8 *pfile, const unsigned int sizfile, const u8 *pnewfile, const unsigned int siznewfile, FILE *ips){
	unsigned int i=0,address=0,final;
	unsigned int size,_size,patchofs;
	fwrite("PATCH",1,5,ips);
	for(;i<min(sizfile,siznewfile);i++){
		if(pfile[i]!=pnewfile[i]){
			address=final=i;
			//for(gap=0;gap<6&&i-1<min(sizfile,siznewfile);gap++){//Up to 5 bytes can be gap. To make small IPS.
			//	i++;
			//	if(pfile[i]!=pnewfile[i])gap=0,final=i+1; //if !memcmp(pfile+i,pnewfile+i,5) then break
			//}
			for(;memcmp_fast(pfile+final,pnewfile+final,min(5,min(sizfile,siznewfile)-final));final++)i++;

			_size=size=final-address;
			patchofs=0;
			while(size){
				unsigned int rleaddr=-1,rlelength=0;
				//OK Let's consume the buffer (do I have to use "fill"?)
				if(size>3)rleaddr=RLE(pnewfile+address+patchofs,_size-patchofs,min(size,7),&rlelength);
				if(rleaddr!=-1){
					if(rleaddr)ipswrite(pnewfile,address+patchofs,rleaddr,ips);
					ipsfill(pnewfile[address+patchofs+rleaddr],address+patchofs+rleaddr,rlelength,ips);
					size-=rleaddr+rlelength;
					patchofs+=rleaddr+rlelength;
					continue;
				}
				ipswrite(pnewfile,address+patchofs,size,ips);
				break; //
			}
		}
	}

	if(siznewfile>sizfile){
		address=sizfile;
		_size=size=siznewfile-sizfile;
		patchofs=0;
			while(size){
				unsigned int rleaddr=-1,rlelength=0;
				//OK Let's consume the buffer (do I have to use "fill"?)
				rleaddr=RLE(pnewfile+address+patchofs,_size-patchofs,size>3?min(size,7):3,&rlelength);
				if(rleaddr!=-1){
					if(rleaddr)ipswrite(pnewfile,address+patchofs,rleaddr,ips);
					ipsfill(pnewfile[address+patchofs+rleaddr],address+patchofs+rleaddr,rlelength,ips);
					size-=rleaddr+rlelength;
					patchofs+=rleaddr+rlelength;
					continue;
				}
				ipswrite(pnewfile,address+patchofs,size,ips);
				break; //
			}
	}

	fwrite("EOF",1,3,ips);
	fflush(ips);
	return 0;
}

static int ipspatch(u8 *pfile, unsigned int *sizfile, const u8 *pips, const unsigned int sizips){ //pass pfile=NULL to get sizfile.
	unsigned int offset=5,address=0,size=-1,u;
	if(sizips<8||memcmp(pips,"PATCH",5))return 2;
	if(!pfile)*sizfile=0;
	while(1){
		if(offset+3>sizips)return 1;address=read24be(pips+offset);offset+=3;
		if(address==0x454f46&&offset==sizips)break;
		if(offset+2>sizips)return 1;size=read16be(pips+offset);offset+=2;
		if(size){
			if(offset+size>sizips)return 1;
			if(!pfile){
				if(*sizfile<address+size)*sizfile=address+size;
				offset+=size;
				continue;
			}
			if(address+size>*sizfile)return 1;
			fprintf(stderr,"write 0x%06x %dbytes\n",address,size);
			memcpy(pfile+address,pips+offset,size);
			offset+=size;
		}else{
			if(offset+3>sizips)return 1;size=read16be(pips+offset);offset+=2;
			if(!pfile){
				if(*sizfile<address+size)*sizfile=address+size;
				offset++;
				continue;
			}
			if(address+size>*sizfile)return 1;
			fprintf(stderr,"fill  0x%06x %dbytes\n",address,size);
			for(u=address;u<address+size;u++)pfile[u]=pips[offset];
			offset++;
		}
	}
	return 0;
}

//bootstrap
static int _ipsmake(FILE *file, FILE *newfile, FILE *ips){
	u8 *pfile=NULL,*pnewfile=NULL;
	unsigned int sizfile,siznewfile;
	//int ret;

	if((sizfile=filelength(fileno(file)))>0x1000000||(siznewfile=filelength(fileno(newfile)))>0x1000000){
		fprintf(stderr,"file is too big(16MB)\n");return 1;
	}
	pfile=(u8*)malloc(sizfile);
	pnewfile=(u8*)malloc(siznewfile);
	if(!pfile||!pnewfile){
		if(pfile)free(pfile);
		if(pnewfile)free(pnewfile);
		fprintf(stderr,"cannot allocate memory\n");return 2;
	}
	memset(pfile,0,sizfile);
	fread(pfile,1,sizfile,file);
	memset(pnewfile,0,siznewfile);
	fread(pnewfile,1,siznewfile,newfile);

	fprintf(stderr,"(%u bytes / %u bytes)\n",sizfile,siznewfile);
	ipsmake(pfile,sizfile,pnewfile,siznewfile,ips);

	fprintf(stderr,"Made successfully\n");
	free(pfile);free(pnewfile);
	return 0;
}

static int _ipspatch(FILE *file, FILE *ips){
	u8 *pfile=NULL,*pips=NULL;
	unsigned int sizfile,sizips;
	int ret;

	if(~ftell(ips)){
		sizips=filelength(fileno(ips));
		pips=(u8*)malloc(sizips);
		if(!pips){
			fprintf(stderr,"cannot allocate memory\n");return 4;
		}
		fread(pips,1,sizips,ips);
	}else{ //if not file
		sizips=0;
		pips=NULL;
		for(;;){
			unsigned int readlen=fread(buf,1,BUFLEN,stdin);
			if(!readlen)break;
			u8 *tmp=(u8*)malloc(sizips+readlen);
			if(!tmp){if(pips)free(pips);fprintf(stderr,"cannot allocate memory\n");return 4;}
			memcpy(tmp,pips,sizips);
			memcpy(tmp+sizips,buf,readlen);
			if(pips)free(pips);
			pips=tmp,tmp=NULL;
			sizips+=readlen;
			if(readlen<BUFLEN)break;
		}
	}

	fprintf(stderr,"(IPS %u bytes) ",sizips);
	ret=ipspatch(NULL,&sizfile,pips,sizips);
	switch(ret){
		case 0:
			if(!sizfile){
				free(pips);
				fprintf(stderr,"ips empty\n");return 5;
			}
			fprintf(stderr,"allocfilesize=%u\n",sizfile);
			pfile=(u8*)malloc(sizfile);
			if(!pfile){
				free(pips);
				fprintf(stderr,"cannot allocate memory\n");return 4;
			}
			fread(pfile,1,sizfile,file); //might occur error, but OK
			rewind(file);
			ipspatch(pfile,&sizfile,pips,sizips);
			fwrite(pfile,1,sizfile,file);
			free(pfile);
			fprintf(stderr,"Patched successfully\n");
			break;
		case 2:
			fprintf(stderr,"\nips not valid\n");
			break;
		case 1:
			fprintf(stderr,"\nPatch failed (corrupted / truncated)\n");
			break;
	}
	free(pips);
	return ret;
}

//library bootstrap
int xenoips(const int argc, const char** argv){
	int flag,ret;
	FILE *file,*newfile=NULL,*ips;

	//startup
	if((argc<2||isatty(fileno(stdin)))&&argc<3){fprintf(stderr,
			"XenoIPS - yet another IPS maker/patcher rev2 v2\n"
			"Usage: xenoips file <ips\n"
			"       xenoips file ips\n"
			"       xenoips file newfile >ips\n"
			"       xenoips file newfile ips\n"
	);msleep(500);return 1;}

	//check running mode
	if(argc==2){
		file=fopen(argv[1],"r+b");if(!file)goto failfile;
		ips=stdin;
		flag=0x00;
	}else if(argc==3){
		if(isatty(fileno(stdout))){
			file=fopen(argv[1],"r+b");if(!file)goto failfile;
			ips=fopen(argv[2],"rb");if(!ips){fclose(file);goto failfile;}
			flag=0x01;
		}else{
			file=fopen(argv[1],"rb");if(!file)goto failfile;
			newfile=fopen(argv[2],"rb");if(!newfile){fclose(file);goto failfile;}
			ips=stdout;
			flag=0x10;
		}
	}else{
		file=fopen(argv[1],"rb");if(!file)goto failfile;
		newfile=fopen(argv[2],"rb");if(!newfile){fclose(file);goto failfile;}
		ips=fopen(argv[3],"wb");if(!ips){fclose(file);fclose(newfile);goto failfile;}
		flag=0x11;
	}

	if(flag&0x10){
		fprintf(stderr,"Source: %s\nTarget: %s\n",argv[1],argv[2]);
		ret=_ipsmake(file,newfile,ips);
		fclose(file);fclose(newfile);
		if(flag==0x11)fclose(ips);
		msleep(1000);return ret?ret|0x20:0;
	}else{
		fprintf(stderr,"File: %s\n",argv[1]);
		ret=_ipspatch(file,ips);
		fclose(file);
		if(flag==0x01)fclose(ips);
		msleep(1000);return ret?ret|0x10:0;
	}

failfile:
	fprintf(stderr,"Cannot open file\n");msleep(1000);return 2;
}
