// Dedicated to EZ team... They require tiny arm9 game for iSP firmware.
#include "../xenobox.h"

#define ARM9SIZE 400000

static bool wildmatch(const char *pattern, const char* compare){
	switch(*pattern){
		case '?': //0x3f
			return wildmatch(pattern+1,compare+1);
		case '*': //0x2a
			return wildmatch(pattern+1,compare)||(*compare&&wildmatch(pattern,compare+1));
		default:
			if(!*pattern&&!*compare)return true;
			if(*pattern!=*compare)return false;
			return wildmatch(pattern+1,compare+1);
	}
}

int ndsarmsizefilter(const int argc, const char **argv){
	char *wild="*.nds";
	char head[512];
	int size;
	FILE *f;

	fprintf(stderr,"ndsarmsizefilter [dir]\n");

	if(argc>1)chdir(argv[1]);
	struct dirent *d;
	DIR *dir=opendir(".");
	for(;d=readdir(dir);){
		if(wildmatch(wild,d->d_name)){
			f=fopen(d->d_name,"rb");
			size=fread(head,1,512,f);
			fclose(f);
			if(size==512 && read32(head+0x2c)<ARM9SIZE)printf("%s %d\n",d->d_name,read32(head+0x2c));
		}
	}
	closedir(dir);
	return 0;
}
