// mergecheat: build a usrcheat.dat with combining several usrcheat files.
// Usage: mergecheat [:TARGET_CODE] target.dat usrcheat.dat merge.dat...
// TARGET_CODE = GBK / BIG5 / SJIS / UTF8\n"
// If TARGET_CODE is not set, it will be set to first usrcheat's charcode.
// mergecheat has a feature to convert charcode, allowing to merge Japanese and Chinese cheat.
// For this feature, mergecheat dynamic-links libiconv.
// Make sure you are using libiconv >=1.13 or convertion will fail (resulting in corrupted dat).
// Note: never execute on big endian machine!

//vector is the easiest way to store this kind of data.

#include <vector>
#include <string>

#include "../xenobox.h"
#define ICONV_BUFFER_SIZE 1024
//static char _iconv_src[ICONV_BUFFER_SIZE];
static char _iconv_dst[ICONV_BUFFER_SIZE];
static const char *from_code;
static const char *to_code;

//0xd5534159
#define SIG_GBK "\xd5SAY"
//0xf5534159
#define SIG_BIG5 "\xf5SAY"
//0x75534159
#define SIG_SJIS "uSAY"
//0x55734159
#define SIG_UTF8 "UsAY"

#define ICONV_ARG "//TRANSLIT"

//#define STANDALONE

#ifdef STANDALONE
//#include <iostream> //debug
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#else
int filelength(int fd){ //constant phrase
	struct stat st;
	fstat(fd,&st);
	return st.st_size;
}
#endif
unsigned char buf[BUFLEN];
#endif

typedef int    (*ticonv_open)(const char*, const char*);
typedef int    (*ticonv_close)(int);
typedef size_t (*ticonv)(int, const char**, size_t*, char**, size_t*);
ticonv_open piconv_open;
ticonv_close piconv_close;
ticonv piconv;

typedef struct{
	unsigned int gamecode;
	unsigned int crc32;
	unsigned long long int offset;
}cheatindex;

#if 0
typedef std::vector<
	std::pair<
		std::pair<
			cheatindex,
			std::string, //game title
		>, std::vector<
			std::pair<
				std::pair<
					u32, //folder count
					std::pair<std::string,std::string> //folder title/note
				>, std::vector<
					std::pair<
						std::pair<
							u32, //whether cheat is ON
							std::pair<std::string,std::string> //cheat title/note
						>, std::vector<u32> //actual cheat
					>
				>
			>
		>
	>
> usrcheat;
#endif

typedef std::pair<
	std::pair<
		u32, //whether cheat is ON
		std::pair<std::string,std::string> //cheat title/note
	>, std::vector<u32> //actual cheat
> cheat;

typedef std::pair<
	std::pair<
		u32, //folder count
		std::pair<std::string,std::string> //folder title/note
	>, std::vector<cheat>
> cheatfolder;

typedef std::pair<
	std::pair<
		cheatindex,
		std::string //game title
	>, std::vector<cheatfolder>
> gamecheat;

typedef std::vector<gamecheat> usrcheat;

static const char *convert(const char *p){
	if(!piconv || !strcmp(from_code,"") || !strcmp(to_code,"") || !memcmp(from_code,to_code,3))return p; //don't convert
	int icd=piconv_open(to_code,from_code);
	if(icd==-1)return p;
	//strcpy(_iconv_src,p);
	size_t size=strlen(p),osize=ICONV_BUFFER_SIZE-1;
	const char *iconv_src=p;
	char *iconv_dst=_iconv_dst;
	memset(_iconv_dst,0,ICONV_BUFFER_SIZE);
	//while(size>0)
		if(piconv(icd,&iconv_src,&size,&iconv_dst,&osize)==(size_t)-1){
			piconv_close(icd);
			return _iconv_dst;
			//return p; //convertion failed. (very unlikely because of //TRANSLIT)
		}
	piconv_close(icd);
	return _iconv_dst;
}

static unsigned int *skipstring(unsigned int *o, std::string &s){
	char *p=(char*)o;
	s=convert(p);
	return (unsigned int*)( align4((size_t)p+strlen(p)+1) );
}

static unsigned int *skipnote(unsigned int *o, std::string &s1, std::string &s2){
	char *p=(char*)o;
	s1=convert(p);
	p=p+strlen(p)+1; //skip title
	s2=convert(p);
	return (unsigned int*)( align4((size_t)p+strlen(p)+1) ); //skip additional note
}

static unsigned int *writestring(unsigned int *o, std::string &s){
	char *p=(char*)o;
	strcpy(p,s.c_str());
	return (unsigned int*)( align4((size_t)p+strlen(p)+1) );
}

static unsigned int *writenote(unsigned int *o, std::string &s1, std::string &s2){
	char *p=(char*)o;
	strcpy(p,s1.c_str());
	p=p+strlen(p)+1; //skip title
	strcpy(p,s2.c_str());
	return (unsigned int*)( align4((size_t)p+strlen(p)+1) ); //skip additional note
}

static int parse(FILE *f,usrcheat &usr){ //parse usrcheat
	//unsigned char buf[12];
	fread(buf,1,12,f);
	if(memcmp(buf,"R4 CheatCode",12)){fprintf(stderr,"usrcheat wrong format ");return 3;}
	fseek(f,76,SEEK_SET);
	fread(buf,1,4,f);
	from_code="";
	if(!memcmp(buf,SIG_GBK,4)){
		from_code="GBK";fprintf(stderr,"Encoding=%s ",from_code);
		if(!strcmp(to_code,"")){
			fprintf(stderr,"Target encoding set to %s. ",from_code);
			to_code="GBK" ICONV_ARG;
		}
	}
	if(!memcmp(buf,SIG_BIG5,4)){
		from_code="BIG5";fprintf(stderr,"Encoding=%s ",from_code);
		if(!strcmp(to_code,"")){
			fprintf(stderr,"Target encoding set to %s. ",from_code);
			to_code="BIG5" ICONV_ARG;
		}
	}
	if(!memcmp(buf,SIG_SJIS,4)){
		from_code="SJIS";fprintf(stderr,"Encoding=%s ",from_code);
		if(!strcmp(to_code,"")){
			fprintf(stderr,"Target encoding set to %s. ",from_code);
			to_code="SJIS" ICONV_ARG;
		}
	}
	if(!memcmp(buf,SIG_UTF8,4)){
		from_code="UTF-8";fprintf(stderr,"Encoding=%s ","UTF8");
		if(!strcmp(to_code,"")){
			fprintf(stderr,"Target encoding set to %s. ","UTF8");
			to_code="UTF-8" ICONV_ARG;
		}
	}

	cheatindex cur,next;
	//usrcheat usr;
	fseek(f,0x100,SEEK_SET);
	fread(&next,1,sizeof(next),f);
	for(;;){
		memcpy(&cur,&next,sizeof(cur));
		fread(&next,1,sizeof(next),f);
		unsigned int size=(next.offset?next.offset:filelength(fileno(f)))-cur.offset;
		unsigned int *_p=(unsigned int*)buf; //(unsigned int*)malloc(size);
		unsigned int z=ftell(f);
		fseek(f,cur.offset,SEEK_SET);
		fread(_p,1,size,f);

		{ //ok, let's parse internal structure.
			unsigned int *p=_p;
			gamecheat g;
			g.first.first=cur;
			p=skipstring(p,g.first.second);

			unsigned int count=*p&0x0fffffff; //sorry but global cheat setting will be set as ON...
			p+=9; //I believe MASTER config should be OK
			unsigned int i=0;
			for(;i<count;){
				cheatfolder folder;
				unsigned int foldercount=1;
				folder.first.first=1;
				if(*p&0x10000000){//folder
					//if(*p&0x01000000){} //folder-choice
					folder.first.first=*p;
					foldercount=*p&0x00ffffff;
					p++;
					p=skipnote(p,folder.first.second.first,folder.first.second.second);
					i++;
				}

				for(;foldercount;foldercount--){
					cheat C;
					C.first.first=*p;
					//unsigned int flag=*p&0xff000000; //fixme
					unsigned int *pnext=p+1+(*p&0x00ffffff);
					p++;
					p=skipnote(p,C.first.second.first,C.first.second.second);
					unsigned int cheatlen=*p;
					p++;

					for(;cheatlen;cheatlen--,p++)C.second.push_back(*p);
					i++;
					p=pnext;
					folder.second.push_back(C);
				}
				g.second.push_back(folder);
				//break; //test
			}

			usr.push_back(g); //parsed!
			//break; //test
		}

		//free(_p);
		fseek(f,z,SEEK_SET);
		if(!next.offset)break;
	}
	return 0;
}

static int output(FILE *f,usrcheat &usr){ //output usrcheat
	//construct header
	memset(buf,0,256);
	memcpy(buf,"R4 CheatCode",12);
	buf[13]=1;
	strcpy((char*)buf+16,"xenobox mergecheat");
	memcpy(buf+76,SIG_GBK,4);
	if(!strcmp(to_code,"BIG5" ICONV_ARG))memcpy(buf+76,SIG_BIG5,4);
	if(!strcmp(to_code,"SJIS" ICONV_ARG))memcpy(buf+76,SIG_SJIS,4);
	if(!strcmp(to_code,"UTF-8" ICONV_ARG))memcpy(buf+76,SIG_UTF8,4);
	buf[80]=1;
	fwrite(buf,1,256,f);

	//write fake index
	unsigned int i_usr;
	memset(buf,0,16);
	for(i_usr=0;i_usr<usr.size();i_usr++){fwrite(buf,1,16,f);}
	fwrite(buf,1,16,f);

	//write body
	//vector<cheatindex> index;
	for(i_usr=0;i_usr<usr.size();i_usr++){
		gamecheat &g=usr[i_usr]; //must use & for avoiding ctor. also below.
		memset(buf,0,BUFLEN);
		unsigned int *_p=(unsigned int*)buf; //(unsigned int*)malloc(size);
		unsigned int *p=_p; //cheat size = p-_p
		p=writestring(p,g.first.second);

		//re-calculate cheatcount.
		u32 cheatcount=0;
		unsigned int i_folder;
		for(i_folder=0;i_folder<g.second.size();i_folder++){
			cheatfolder &folder=g.second[i_folder];
			if(folder.first.first&0x10000000)
				cheatcount+=folder.first.first&0x00ffffff;
			cheatcount++;
		}
		*p=cheatcount|0xf0000000;p++;
		p++;
		*p=0x01000000;p++;
		p++;
		p++;
		p++;
		p++;
		p++;
		p++;
		//output each folder
		for(i_folder=0;i_folder<g.second.size();i_folder++){
			cheatfolder &folder=g.second[i_folder];
			if(folder.first.first&0x10000000){
				*p=folder.first.first;p++;
				p=writenote(p,folder.first.second.first,folder.first.second.second);
			}
			unsigned int i_cheat;
			for(i_cheat=0;i_cheat<folder.second.size();i_cheat++){
				cheat &C=folder.second[i_cheat];
				unsigned int *size=p;p++;
				p=writenote(p,C.first.second.first,C.first.second.second);
				//cout<< C.first.second.first <<endl;
				*p=C.second.size();p++;
				unsigned int i=0;
				for(;i<C.second.size();i++){*p=C.second[i];p++;}
				*size=(C.first.first&0xff000000)|(p-size-1);
			}
		}
		g.first.first.offset=ftell(f);
		//cheatindex _index=g.first.first;
		//index.push_back(_index);
		//cout << g.first.first.offset << endl;
		fwrite(_p,4,p-_p,f); //written!
	}

	//write true index
	fseek(f,256,SEEK_SET);
	for(i_usr=0;i_usr<usr.size();i_usr++){
		gamecheat &g=usr[i_usr];
		fwrite(&g.first.first,1,16,f);
		//cheatindex &_index=index[i_usr];
		//fwrite(&_index,1,16,f);
	}
	return 0;
}

static int merge(usrcheat &dst,usrcheat &src){ //merge DB
	unsigned int i,j,l,m,n,o;
	for(i=0;i<src.size();i++){
		gamecheat sg=src[i]; //must not use &, because src will be destructed later.
		for(j=0;j<dst.size();j++){
			if(dst[j].first.first.gamecode==sg.first.first.gamecode&&dst[j].first.first.crc32==sg.first.first.crc32)break;
		}
		if(j==dst.size()){dst.push_back(sg);continue;} //merge game

		//merge content
		gamecheat &dg=dst[j];
		for(l=0;l<sg.second.size();l++){
			cheatfolder &sf=sg.second[l];
			for(m=0;m<dg.second.size();m++){
				cheatfolder &df=dg.second[m];
				if(sf.first.first==1){
					if(df.first.first==1&&sf.second[0].second==df.second[0].second)break; //found cheat
				}else{
					if(sf.first.second.first==df.first.second.first){ //folder name found
						//merge folder
						//need to re-calculate df.first.first after merging
						for(n=0;n<sf.second.size();n++){
							cheat &sC=sf.second[n];
							for(o=0;o<df.second.size();o++){
								cheat &dC=df.second[o];
								if(sC.second==dC.second)break;
							}
							if(o==df.second.size())df.second.push_back(sC);
						}
						df.first.first=(df.first.first&0xff000000)|df.second.size();
						break;
					}
				}
			}
			if(m==dg.second.size())dg.second.push_back(sf);
		}
	}
	return 0;
}

#ifndef STANDALONE
extern "C" int mergecheat(const int argc, const char **argv){
#else
extern "C" int main(const int argc, const char **argv){
#endif
	piconv_open=NULL;
	piconv_close=NULL;
	piconv=NULL;
	to_code="";
#ifndef NODLOPEN
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	#define ICONV "libiconv"
	HMODULE hiconv=LoadLibraryA("libiconv2.dll");
	if(!hiconv)hiconv=LoadLibraryA("libiconv-2.dll");
	if(!hiconv)hiconv=LoadLibraryA("libiconv.dll");
#else
	#define ICONV "iconv"
	void* hiconv=LoadLibraryA("libiconv.so.2"); //Generic
	if(!hiconv)hiconv=LoadLibraryA("libiconv.so");
	if(!hiconv)hiconv=LoadLibraryA("libiconv.so.3"); //FreeBSD
	if(!hiconv)hiconv=LoadLibraryA("libiconv.so.5");
	if(!hiconv)hiconv=LoadLibraryA("libiconf.so.3");
	if(!hiconv)hiconv=LoadLibraryA("libiconf.so");
	if(!hiconv)hiconv=LoadLibraryA("libiconv.2.dylib"); //OSX
	if(!hiconv)hiconv=LoadLibraryA("libiconv.dylib");
	if(!hiconv)hiconv=LoadLibraryA("libc.so.6"); //even if C library, need to make sure to load iconv part via dlopen(), due to license.
	if(!hiconv)hiconv=LoadLibraryA("libc.so");
	if(!hiconv)hiconv=LoadLibraryA("iconv"); //FeOS(lol?)
#endif
	if(!hiconv){fprintf(stderr,"dlopen(libiconv) failed. cannot convert charcode.\n");}
	else{
		piconv_open=(ticonv_open)GetProcAddress(hiconv,ICONV"_open");
		piconv_close=(ticonv_close)GetProcAddress(hiconv,ICONV"_close");
		piconv=(ticonv)GetProcAddress(hiconv,ICONV);
		if(!piconv||!piconv_open||!piconv_close){
			piconv=NULL,piconv_open=NULL,piconv_close=NULL;
			FreeLibrary(hiconv);hiconv=NULL;
			fprintf(stderr,"dlsym(libiconv,iconv) failed. cannot convert charcode.\n");
		}
	}
#endif
	FILE *f=NULL,*out=NULL;
	if(argc<3){
		fprintf(stderr,
			"mergecheat [:TARGET_CODE] target.dat usrcheat.dat merge.dat...\n"
			"TARGET_CODE = GBK / BIG5 / SJIS / UTF8\n"
		);
		return 1;
	}

	int i=1;
	char conv[128];
	if(argv[i][0]==':'){
		strcpy(conv,argv[i]+1);
		if(!strcmp(conv,"GBK")||!strcmp(conv,"BIG5")||!strcmp(conv,"SJIS")||!strcmp(conv,"UTF8")){
			fprintf(stderr,"Target encoding set to %s.\n",conv);
			if(!strcmp(conv,"UTF8"))strcpy(conv,"UTF-8"); //for some compatibility... UTF8 might not be allowed. Let's use UTF-8 instead.
			strcat(conv,ICONV_ARG);
			to_code=conv;
		}else{
			fprintf(stderr,"Invalid encoding %s.\n",conv);
		}
		i++;
	}
	const int I=i; //target

	i++;
	if(!(f=fopen(argv[i],"rb"))){
		fprintf(stderr,"cannot open base cheat file\n");return 2;
	}

	usrcheat usr,_merge;
	int ret=0;
	fprintf(stderr,"Reading %s... ",argv[i]);
	if(ret=parse(f,usr)){fprintf(stderr,"Failed.\n");return ret;}
	fprintf(stderr,"Done.\n");
	fclose(f);
	i++;

	for(;i<argc;i++){
		fprintf(stderr,"Reading %s... ",argv[i]);
		if(!(f=fopen(argv[i],"rb"))){
			fprintf(stderr,"cannot open.\n");
			continue;
		}
		if(parse(f,_merge)){fprintf(stderr,"Failed.\n");fclose(f);continue;}
		fprintf(stderr,"Done.\n");
		fclose(f);
		merge(usr,_merge);
	}
	if(!(out=fopen(argv[I],"wb"))){
		fprintf(stderr,"cannot open target cheat file\n");return 2;
	}
	fprintf(stderr,"Writing new usrcheat... ");
	output(out,usr);
	fprintf(stderr,"Done.\n");
	fclose(out);
#ifndef NODLOPEN
	if(hiconv)FreeLibrary(hiconv);
#endif
	fprintf(stderr,"Everything is Ok.\n");
	return ret;
}
