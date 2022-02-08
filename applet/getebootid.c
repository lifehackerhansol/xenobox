// getebootid based on http://nsdev.jp/psp/178.html

#ifdef PSP
#include <pspkernel.h>
#include <pspreg.h>
#include <stdio.h>
#include <string.h>
#include <pspsysmem_kernel.h>
#include <pspthreadman_kernel.h>
#include <pspctrl.h>
#include <pspiofilemgr_kernel.h>
static unsigned int read32(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8)|(x[2]<<16)|(x[3]<<24);
}
static unsigned short read16(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8);
}
#else
#include "../xenobox.h"
#include "pspiofilemgr_stdio.h"
static int _argc;
static char *_arg;
#endif

static char __disc_id[256];
static char head[16];
#ifdef PSP
static int get_disc_id(){
	void* (*sceKernelGetGameInfo_k)();
	memset(__disc_id, 0, sizeof(__disc_id));
	sceKernelGetGameInfo_k = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemForKernel", 0xCD617A94);
	memcpy(__disc_id, ((char*)sceKernelGetGameInfo_k()) + 0x44, 9);
	if(*__disc_id)return 0;
	char *eboot_name=((char*)sceKernelGetGameInfo_k()) + 0x74;
#else
static int get_disc_id(const char *eboot_name){
#endif
	SceUID fd=sceIoOpen(eboot_name,PSP_O_RDONLY,0777);
	sceIoRead(fd,head,16);
	if(memcmp(head,"\0PBP",4)){//||read32(head+4)!=0x00010000){ //LittleBigPlanet uses 0x00010001... shouldn't check lol
		sceIoClose(fd);
		goto end;
	}
	int param_offset=read32(head+8);
	int param_size=read32(head+12)-param_offset;
#ifdef PSP
	SceUID uid=sceKernelAllocPartitionMemory(2,"EBOOTReader",PSP_SMEM_Low,param_size,NULL);
	if(uid<0){
		sceIoClose(fd);
		goto end;
	}
	char *p=sceKernelGetBlockHeadAddr(uid);
#else
	char *p=malloc(param_size);
	if(!p){
		sceIoClose(fd);
		goto end;
	}
#endif
	sceIoLseek(fd,param_offset,SEEK_SET);
	sceIoRead(fd,p,param_size);
	sceIoClose(fd);

	if(memcmp(p,"\0PSF",4)||read32(p+4)!=0x00000101){
#ifdef PSP
		sceKernelFreePartitionMemory(uid);
#else
		free(p);
#endif
		goto end;
	}

#ifndef PSP
	if(_argc>2&&(_arg[0]=='p'||_arg[0]=='P')&&!isatty(fileno(stdout))){
		fwrite(p,1,param_size,stdout);
		free(p);
		return 0;
	}
#endif

	int label_offset=read32(p+8);
	int data_offset=read32(p+12);
	int nlabel=read32(p+16);
	int i=0;
	for(;i<nlabel;i++){
#ifdef PSP
		if(!strcmp(p+label_offset+read16(p+20+16*i),"DISC_ID")){ //seems to be 16bytes long
#else
		if(!strcmp(p+label_offset+read16(p+20+16*i),_argc>2?"TITLE":"DISC_ID")){ //seems to be 16bytes long
#endif
			int datasize=read32(p+20+16*i+8);
			//if(datasize>19)datasize=19;
			memcpy(__disc_id,p+data_offset+read32(p+20+16*i+12),datasize);
			break;
		}
	}
#ifdef PSP
	sceKernelFreePartitionMemory(uid);
#else
	free(p);
#endif
	if(i==nlabel)return 2;
	return 0;
end:
	return 1;
}

#ifndef PSP
int getebootid(const int argc, const char **argv){
	if(argc<2){fprintf(stderr,"getebootid EBOOT.PBP [T|P]\n");return -1;}
	_argc=argc;
	_arg=argc>2?(char*)argv[2]:NULL;
	int ret=get_disc_id(argv[1]);
	if(ret){fprintf(stderr,ret==1?"parse failed\n":"no DISC_ID label\n");return ret;}
	if(!isatty(fileno(stdout)))printf("%s",__disc_id); //can use this value in shell
	else puts(__disc_id);
	return 0;
}
#endif
