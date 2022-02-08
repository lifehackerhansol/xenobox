//never execute on big endian machine!

//how can I use find() within C?
#ifdef USTL
#include <ustl.h>
using namespace ustl;
template <typename T>
struct VECTOR: public vector<T>{
public:
	bool operator<(const VECTOR<T> &R) const{
		for(unsigned int i=0;i<this->size();i++)
			if(this->at(i)!=R.at(i))return this->at(i)<R.at(i);
		return this->size()<R.size();
	}
};
#else
#include <vector>
#include <set>
using namespace std;
#define VECTOR vector
#endif

#include "../xenobox.h"

#if 1
typedef struct{
	unsigned int gamecode;
	unsigned int crc32;
	unsigned long long int offset;
}cheatindex;
#endif

static unsigned int *skipstring(unsigned int *o){
	char *p=(char*)o;
	return (unsigned int*)( align4((size_t)p+strlen(p)+1) );
}

static unsigned int *skipnote(unsigned int *o){
	char *p=(char*)o;
	p=p+strlen(p)+1; //skip title
	return (unsigned int*)( align4((size_t)p+strlen(p)+1) ); //skip additional note
}

static int stage2(unsigned int *o, unsigned int *n){ //parse usrcheat member
	set<VECTOR<unsigned int> >s;
	VECTOR<unsigned int> v;
	int ret=0;

	//read all o
	o=skipstring(o);
	unsigned int ocount=*o&0x0fffffff;
	//unsigned int oenable=*o&0xf0000000;
	o+=9;
	unsigned int i=0;
	for(;i<ocount;){
		unsigned int foldercount=1;
		if(*o&0x10000000){//folder
			//if(*o&0x01000000){} //folder-choice
			foldercount=*o&0x00ffffff;
			o++;
			o=skipnote(o);
			i++;
		}
		for(;foldercount;foldercount--){
			unsigned int oflag=*o&0xff000000; //fixme
			unsigned int *onext=o+1+(*o&0x00ffffff);
			o++;
			o=skipnote(o);
			unsigned int cheatlen=*o;
			o++;

			//add o..o+cheatlen if oflag.
			if(oflag){
				v.clear();
				for(;cheatlen;cheatlen--,o++)v.push_back(*o);
				s.insert(v);
			}

			i++;
			o=onext;
		}
	}

	//compare with all n
	n=skipstring(n);
	unsigned int ncount=*n&0x0fffffff;
	//unsigned int nenable=*n&0xf0000000;
	n+=9;
	for(i=0;i<ncount;){
		unsigned int foldercount=1;
		if(*n&0x10000000){//folder
			//if(*n&0x01000000){} //folder-choice
			foldercount=*n&0x00ffffff;
			n++;
			n=skipnote(n);
			i++;
		}
		for(;foldercount;foldercount--){
			unsigned int *x=n;
			//unsigned int nflag=*n&0xff000000;
			unsigned int *nnext=n+1+(*n&0x00ffffff);
			n++;
			n=skipnote(n);
			unsigned int cheatlen=*n;
			n++;

			//search s for n..n+cheatlen.
			v.clear();
			for(;cheatlen;cheatlen--,n++)v.push_back(*n);
			if(s.find(v)!=s.end())*x=0x01000000|(*x&0x00ffffff),ret++;

			i++;
			n=nnext;
		}
	}
	return ret;
}

static int stage1(FILE *_old, FILE *_new){ //parse usrcheat header
	unsigned char buf[12];
	fread(buf,1,12,_old);
	if(memcmp(buf,"R4 CheatCode",12)){fprintf(stderr,"usrcheat_old wrong format\n");return 3;}
	fread(buf,1,12,_new);
	if(memcmp(buf,"R4 CheatCode",12)){fprintf(stderr,"usrcheat_new wrong format\n");return 3;}

	cheatindex ocur,onext,ncur,nnext;
	fseek(_old,0x100,SEEK_SET);
	fread(&onext,1,sizeof(onext),_old);
	for(;;){
		memcpy(&ocur,&onext,sizeof(ocur));
		fread(&onext,1,sizeof(onext),_old);

		fseek(_new,0x100,SEEK_SET);
		fread(&nnext,1,sizeof(nnext),_new);
		for(;;){
			memcpy(&ncur,&nnext,sizeof(ncur));
			fread(&nnext,1,sizeof(nnext),_new);
			if(ocur.gamecode==ncur.gamecode&&ocur.crc32==ncur.crc32){
				//puts("");
				//printf("[%c%c%c%c:%08x] ",ocur.gamecode&0xff,(ocur.gamecode>>8)&0xff,(ocur.gamecode>>16)&0xff,(ocur.gamecode>>24),ocur.crc32);
				unsigned int osize=(onext.offset?onext.offset:filelength(fileno(_old)))-ocur.offset;
				unsigned int nsize=(nnext.offset?nnext.offset:filelength(fileno(_new)))-ncur.offset;
				unsigned int *o=(unsigned int*)malloc(osize);
				unsigned int *n=(unsigned int*)malloc(nsize);

				unsigned int z=ftell(_old);
				fseek(_old,ocur.offset,SEEK_SET);
				fseek(_new,ncur.offset,SEEK_SET);
				fread(o,1,osize,_old);
				fread(n,1,nsize,_new);
				int ret=stage2(o,n);
				if(ret){
					fseek(_old,ocur.offset,SEEK_SET);
					fseek(_new,ncur.offset,SEEK_SET);
					fwrite(o,1,osize,_old);
					fwrite(n,1,nsize,_new);
				}
				free(o),free(n);
				fseek(_old,z,SEEK_SET);
				break;
			}
			if(!nnext.offset)break;
		}
		if(!onext.offset)break;
	}
	return 0;
}

extern "C" int updatecheat(const int argc, const char **argv){
	FILE *_old=NULL,*_new=NULL;
	if(argc!=3){fprintf(stderr,"updatecheat usrcheat_old.dat usrcheat_new.dat\n");return 1;}
	if(!(_old=fopen(argv[1],"rb"))||!(_new=fopen(argv[2],"r+b"))){
		if(_old)fclose(_old);if(_new)fclose(_new);
		fprintf(stderr,"cannot open cheat file\n");return 2;
	}
	int ret=stage1(_old,_new);
	fclose(_old);fclose(_new);
	return ret;
}
