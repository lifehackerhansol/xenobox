/*
 * openpatch_single
 * vs openpatch (STL):
 * pros written in C, not need to calc entire CRC, reverse-patch
 * cons GameList has to be V2(GameListV2Builder.pl), GameList.txt is read (2*games) times
 */

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
#include "xenofile.h"
#define printf _consolePrintf2
#define buf libprism_buf
#else
#include "../xenobox.h"
#endif

static int validate1(char *s){
	int i=0;
	for(;i<strlen(s);i++){
		if('0'<=s[i]&&s[i]<='9');
		else if('a'<=s[i]&&s[i]<='f')s[i]-=0x20;
		else if('A'<=s[i]&&s[i]<='F');
		else break;
	}
	return i;
}

static int validate2(char *ret, char*s){
	int i=0,j=0;
	for(;i<strlen(s);i++){
		if('0'<=s[i]&&s[i]<='9')ret[j++]=s[i];
		else if('a'<=s[i]&&s[i]<='f')ret[j++]=s[i]-0x20;
		else if('A'<=s[i]&&s[i]<='F')ret[j++]=s[i];
		//else break;
	}
	ret[j]=0;return j;
}

//openpatch_single: C test for implementing to XenoFile
//This uses Game ID rather than Game CRC.
static unsigned char openpatch_buf[2048],openpatch_comp[1024];
int _openpatch_single(const char *dbname,const char *arg){
	char scrc32[9];scrc32[4]=0;//scrc32[8]=0;
	FILE *db=fopen(dbname,"rb");
	if(!db){printf("cannot open %s\n",dbname);return 1;}
	FILE *nds=fopen(arg,"r+b");
	if(!nds){printf("cannot open %s\n",arg);return 2;}

	//calc CRC32
#if 0
	unsigned int CRC32=0;//xffffffff;
	unsigned int j=filelength(fileno(nds));
	for(;j;j-=min(j,BUFLEN)){
		fread(buf,1,min(j,BUFLEN),nds);
		CRC32=crc32(CRC32,buf,min(j,BUFLEN));
	}
	printf("%s: %08X ",arg,CRC32);
#endif
	char sID[5];sID[4]=0;
	fseek(nds,0x0c,SEEK_SET);
	fread(sID,1,4,nds);
	printf("%s: %s ",arg,sID);

search:
	for(;myfgets((char*)buf,BUFLEN,db);){
		int last=0,idx=0,idx2=0;
#if 0
		for(;(idx=strchrindex((char*)buf,'[',last))!=-1;){ //parse header
			if(idx>strlen((char*)buf)-9)break;
			idx2=strchrindex((char*)buf,']',idx+1);
			if(idx2==-1)break;
			last=idx+1;
			if(idx2-idx!=9)continue;
			//idx+1..last-1 is CRC32
			strncpy(scrc32,(char*)buf+idx+1,8);
			if(validate1(scrc32)==8){
				unsigned int _CRC32;
				sscanf(scrc32,"%08X",&_CRC32);
				if(_CRC32==CRC32)goto patch;
			}
		}
#endif
		for(;(idx=strchrindex((char*)buf,'{',last))!=-1;){ //parse header
			if(idx>strlen((char*)buf)-5)break;
			idx2=strchrindex((char*)buf,'}',idx+1);
			if(idx2==-1)break;
			last=idx+1;
			if(idx2-idx!=5)continue;
			//idx+1..last-1 is CRC32
			strncpy(scrc32,(char*)buf+idx+1,4);
			//if(validate1(scrc32)==8){
			//	unsigned int _CRC32;
			//	sscanf(scrc32,"%08X",&_CRC32);
				if(!memcmp(scrc32,sID,4))goto patch;
			//}
		}
	}
	fclose(db);
	fclose(nds);
	printf("not found in DB\n");return 3;

patch:
	printf("found in DB.\nChecking integrity... ");
	u32 offset=ftell(db);
	int differ_before=0;
	int differ_after=0;
	for(;myfgets((char*)buf,BUFLEN,db);){
		if(!*buf)continue;
		if(strchr((char*)buf,'['))break; //I need insurance if GameListV2Builder.pl fails
		if(strchr((char*)buf,'{'))break;
		int addrlen=validate1((char*)buf);
		if(addrlen>8){
			printf("bad DB: addrlen>8. Halt.\n");
			goto fail;
		}
		u32 address,patchlen;
		sscanf((char*)buf,"%x",&address);
		patchlen=validate2((char*)openpatch_buf,(char*)buf+addrlen+1);
		if(patchlen&3){
			printf("bad DB: patch length isn't 4x. Halt.\n");
			goto fail;
		}
		patchlen>>=2;
		int i;
		for(i=0;i<patchlen*2;i++){
			unsigned int c;
			sscanf((char*)openpatch_buf+i*2,"%02X",&c);
			openpatch_buf[i]=(unsigned char)c;
		}
		unsigned char *before=openpatch_buf,*after=openpatch_buf+patchlen;
		fseek(nds,address,SEEK_SET);
		fread(openpatch_comp,1,patchlen,nds);
		if(memcmp(before,openpatch_comp,patchlen))differ_before=1;
		if(memcmp(after,openpatch_comp,patchlen))differ_after=1;
		if(differ_before&&differ_after){
			printf("failed. Continue search.\n");
			goto search;
		}
	}
	printf("done.\n");

	if(differ_before){
		printf("Your ROM is already patched.\n");
		printf("Do you want to reverse-patch? ");
#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
		printf("(a/B)\n");
		u32 key;
	#ifdef LIBVALKYRIA_H
		for(;scanKeys(),!(key=keysDown());)swiWaitForVBlank();
	#else
		for(;!(key=IPCZ->keysdown);)swiWaitForVBlank();
	#endif
		if(key&KEY_A);
		else{printf("Cancelled.\n");goto end;}	
#else
		printf("(y/N) ");
		int c=getchar();
		if(c=='y'||c=='Y');
		else{printf("Cancelled.\n");goto end;}
#endif
	}

	fseek(db,offset,SEEK_SET);
	//re-read db.
	for(;myfgets((char*)buf,BUFLEN,db);){
		if(!*buf)continue;
		if(strchr((char*)buf,'['))break; //I need insurance if GameListV2Builder.pl fails
		if(strchr((char*)buf,'{'))break;
		int addrlen=validate1((char*)buf);

		u32 address,patchlen;
		sscanf((char*)buf,"%x",&address);
		patchlen=validate2((char*)openpatch_buf,(char*)buf+addrlen+1);

		patchlen>>=2;
		int i;
		for(i=0;i<patchlen*2;i++){
			unsigned int c;
			sscanf((char*)openpatch_buf+i*2,"%02X",&c);
			openpatch_buf[i]=(unsigned char)c;
		}
		unsigned char *before=openpatch_buf,*after=openpatch_buf+patchlen;
		fseek(nds,address,SEEK_SET);
		fwrite(differ_before?before:after,1,patchlen,nds);
		printf("%08X %dbytes\n",address,patchlen);
	}

end:
	fclose(db);
	fclose(nds);
	return 0;

fail:
	fclose(db);
	fclose(nds);
	return 4;
}

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
int openpatch_single(const char *arg){
	if(!strcpy_safe(buf,findpath(6,(char*[]){"/","/_dstwoplug/","/ismartplug/","/_iMenu/_ini/","/_plugin_/",mypath},"GameList.txt"))){printf("not found GameList.txt\n");return -1;}
	return _openpatch_single(buf,arg);
}
#else
int openpatch_single(const int argc, const char **argv){
	if(argc<3){printf("openpatch_single patch.txt ROM.nds...\n");return -1;}
	if(access(argv[1],0)){printf("patch.txt isn't accesible\n");return -1;}
	int i=2;
	for(;i<argc;i++){
		_openpatch_single(argv[1],argv[i]);
	}
	return 0;
}
#endif
