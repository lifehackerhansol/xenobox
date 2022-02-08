//never execute on big endian machine!

//iostream will cause huge binary on MinGW...
//#include <iostream>
//#include <sstream>
//#include <fstream>

#include <map>
#include <vector>
#include <string>

#include "../xenobox.h"

//static unsigned char head[512];

std::string validate1(std::string &s){
	std::string ret="";
	size_t i=0;
	for(;i<s.size();i++){
		if('0'<=s[i]&&s[i]<='9')ret+=(char)s[i];
		else if('a'<=s[i]&&s[i]<='f')ret+=(char)(s[i]-0x20);
		else if('A'<=s[i]&&s[i]<='F')ret+=(char)s[i];
		else break;
	}
	return ret;
}

std::string validate2(std::string &s){
	std::string ret="";
	size_t i=0;
	for(;i<s.size();i++){
		if('0'<=s[i]&&s[i]<='9')ret+=(char)s[i];
		else if('a'<=s[i]&&s[i]<='f')ret+=(char)(s[i]-0x20);
		else if('A'<=s[i]&&s[i]<='F')ret+=(char)s[i];
		//else break;
	}
	return ret;
}

extern "C" int openpatch(const int argc, const char **argv){
	//ifstream patch;
	std::string tmp;
	FILE *patch=NULL,*nds=NULL;
	if(argc<3){printf("openpatch patch.txt ROM.nds...\n");return 1;}
	if(!(patch=fopen(argv[1],"r"))){printf("cannot open patch\n");return 2;}

	printf("Parsing patch.txt...\n");
	std::map<unsigned int,std::vector< //CRC32
		std::pair<unsigned int, std::pair<//address
			std::vector<unsigned char>,std::vector<unsigned char> //before, after
		> >
	> >m;
	std::vector<std::pair<unsigned int,std::pair<std::vector<unsigned char>,std::vector<unsigned char> > > >v;
	//m[CRC32][i].first=address .second.first=before .second.second=after
	while(myfgets((char*)buf,BUFLEN,patch)){ //parse patch
//cout<<"---"<<buf<<"+"<<last<<"+"<<idx<<"+"<<idx2<<endl;
		int last=0,idx=0,idx2=0;
		std::vector<unsigned int>vcrc;
		for(;(idx=strchrindex((char*)buf,'[',last))!=-1;){ //parse header
			if(idx>(int)strlen((char*)buf)-9)break;
			idx2=strchrindex((char*)buf,']',idx+1);
			if(idx2==-1)break;
			last=idx+1;
			if(idx2-idx!=9)continue;
			//idx+1..last-1 is CRC32
			tmp=std::string((char*)buf+idx+1);
			std::string s=validate1(tmp);
			if(s.length()==8){
				unsigned int CRC32;
				sscanf(s.c_str(),"%08X",&CRC32);
//cout<<hex<<CRC32<<endl;
				vcrc.push_back(CRC32);
			}
		}
		if(vcrc.empty())continue; //nextline

		v.clear();
		int errflag=0;
		while(myfgets((char*)buf,BUFLEN,patch)){ //parse body
			if(buf[0]==0||buf[0]=='\r'){
				break;
			}
			std::vector<unsigned char> before,after;
			tmp=std::string((char*)buf);
			std::string s=validate1(tmp);
			if(s.length()>8){printf("address error: %s\n",s.c_str());errflag=1;}
//cout<<buf<<endl;
			unsigned int address;
			sscanf(s.c_str(),"%x",&address);
//cout<<address<<endl;
			tmp=tmp.substr(s.length());
			s=validate2(tmp);
			if(s.length()&3){printf("patch.txt error: length %d (not 4x)\n",(int)s.length());errflag=1;}
			int i=0;
			for(;i<(int)s.length()/4;i++){
				unsigned int b,a;
				sscanf(s.c_str()+i*2,"%02X",&b);
				sscanf(s.c_str()+i*2+s.length()/2,"%02X",&a);
				before.push_back((unsigned char)b);
				after.push_back((unsigned char)a);
			}
			v.push_back(make_pair(address,make_pair(before,after)));
		}
		//add
		int i=0;
		for(;i<(int)vcrc.size();i++){
			if(errflag){printf("warn: skipped %08X\n",vcrc[i]);continue;}
			if(m.find(vcrc[i])!=m.end()){printf("duplicate: skipped %08X\n",vcrc[i]);continue;}
			m[vcrc[i]]=v;
		}
	}
	printf("Done.\n");

	//unsigned char *buf=(unsigned char*)buf;
	//from here line can be used as free buffer
	int i=2;
	for(;i<argc;i++){
		printf("%s: ",argv[i]);
		if(!(nds=fopen(argv[i],"r+b"))){printf("cannot open nds\n");continue;}
		//calc CRC32
		unsigned int CRC32=0;//xffffffff;
		unsigned int j=filelength(fileno(nds));
		for(;j;j-=std::min(j,(u32)BUFLEN)){
			fread(buf,1,std::min(j,(u32)BUFLEN),nds);
			CRC32=crc32(CRC32,buf,std::min(j,(u32)BUFLEN));
		}

		printf("%08X ",CRC32);
		if(m.find(CRC32)!=m.end()){
			printf("found in DB.\nNow checking integrity.\n");
			v=m[CRC32];
			for(j=0;j<v.size();j++){
				unsigned int address=v[j].first;
				std::vector<unsigned char>before=v[j].second.first;
				fseek(nds,address,SEEK_SET);
				fread(buf,1,before.size(),nds);
				unsigned int k=0;
				for(;k<before.size();k++)
					if(before[k]!=buf[k]){printf("failed.\n");goto nextnds;}
			}
			for(j=0;j<v.size();j++){
				unsigned int address=v[j].first;
				std::vector<unsigned char>after=v[j].second.second;
				unsigned int k=0;
				for(;k<after.size();k++)buf[k]=after[k];
				fseek(nds,address,SEEK_SET);
				fwrite(buf,1,after.size(),nds);
				printf("%08X %dbytes\n",address,(int)after.size());
			}
			printf("Done.\n");
		}else{
			printf("not found in DB\n");
		}
nextnds:
		fclose(nds);
	}

	return 0;
}

