#include "xenobox.h"

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#else
	int filelength(int fd){ //constant phrase
		struct stat st;
		fstat(fd,&st);
		return st.st_size;
	}
#endif

int sfilelength(const char *path){
	struct stat st;
	stat(path,&st);
	return st.st_size;
}

int filemode(int fd){
	struct stat st;
	fstat(fd,&st);
	return st.st_mode;
}

int sfilemode(const char *path){
	struct stat st;
	stat(path,&st);
	return st.st_mode;
}

unsigned char buf[BUFLEN];

/* main like busybox */

#define TRIVIAL_INDEX 14
#define USUAL_INDEX 23
static const applet apps[]={ //should be sorted
	{"applets",applets},
	{"list",applets},
	{"--list",applets},
	{"link",_link},
	{"--link",_link},
	{"link-full",_link},
	{"--link-full",_link},
	{"install",_install},
	{"--install",_install},
	{"license",license},
	{"--license",license},
	{"licence",license},
	{"--licence",license},
	{"moo",moo},

//trivial list
	{"belon",belon},
	{"dlsymtest",dlsymtest},
	{"gdd2011android",gdd2011android},
	{"mondayloader",wdayloader},
	{"r4brute",r4brute},
	{"wdayloader",wdayloader},
	{"xenoalloc",xenoalloc},
	{"xenodice",xenodice},
	{"yzpasscode",yzpasscode},

//usual list
	{"16bitbmp",_16bitbmp},
	{"a9_02000000",a9_02000000},
	{"akaio151",akaio151},
	{"akaiodec_v3a",akaiodec_v3a},
	{"akaiodec_v4",akaiodec_v4},
	{"akextract",akextract},
	{"akextract_ex4",akextract_ex4},
	{"akextract_wood",akextract_wood},
	{"akextract_wood2",akextract_wood2},
	{"androidlogo",androidlogo},
	//{"asn1derdump",asn1derdump},
	{"bin2cstdio",bin2cstdio},
	{"bin2sstdio",bin2sstdio},
	{"binreplace",binreplace},
	{"breaksplash",breaksplash},
	{"checkcheatsize",checkcheatsize},
	{"checkumdsize",checkumdsize},
	{"cmdini",cmdini},
	{"create",create},
	{"createff",createff},
	{"decievo",decievo},
	{"dldipatch",dldipatch},
	{"dldirename",dldirename},
	{"ds2decryptplug",ds2decryptplug},
	{"ds2makeplug",ds2makeplug},
	{"ds2splitplug",ds2splitplug},
	{"dsapfilt",dsapfilt},
	{"dsbize",dsbize},
	{"dstwo2ismm",dstwo2ismm},
	{"extinfo2binary",extinfo2binary},
	{"ezskinfix",ezskinfix},
	{"fatpatch",fatpatch},
	{"fcutdown",_cutdown},
	{"fextend",_extend},
	{"file++",filepp},
	{"fixheader",fixheader},
	{"fixloaderstartup",fixloaderstartup},
	{"ftruncate",_truncate},
	{"gbalzssrawstdio",gbalzssrawstdio},
	{"gencrctable",gencrctable},
	{"getdiscid",getdiscid},
	{"getdiscid_lite",getdiscid_lite},
	{"getebootid",getebootid},
	{"getjavacert",getjavacert},
	{"getyomecolleuid",getyomecolleuid},
	{"google2fa",google2fa},
	{"guidbreak",guidbreak},
	{"guidpatch",guidpatch},
	{"gunprx",gunprx},
	{"gzprx",gzprx},
	{"h4xms2",h4xms2},
	{"ilshield",ilshield_des},
	{"ilshield_aes",ilshield_aes},
	{"ilshield_des",ilshield_des},
	{"ilshield_3des",ilshield_3des},
	{"kallsymslookupsearch",kallsymslookupsearch},
	{"kauralngasm",kauralngasm},
	{"kauralngdis",kauralngdis},
	{"kawdecode",kawdecode},
	{"kawencode",kawencode},
	{"keyconvertnds",keyconvertnds},
	{"keyconvertpsp",keyconvertpsp},
	{"libfatreduce",libfatreduce},
	{"m3dec",m3dec},
	{"m3make",m3make},
	{"m3patch",m3patch},
	{"m4c2m4a",m4c2m4a},
	{"makeplug",ds2makeplug},
	{"makesplash",makesplash},
	{"mergecheat",mergecheat}, //C++(STL)
	{"mergecwc",mergecwc}, //C++(STL)
	{"mmcprotectsearch",mmcprotectsearch},
	{"modifybanner",modifybanner},
	{"mselink",mselink},
	{"msleep",_msleep},
	{"ndsarmsizefilter",ndsarmsizefilter},
	{"ndslink",ndslink},
	{"nidcalc",nidcalc},
	{"nsc",nsc},
	{"openpatch",openpatch}, //C++(STL)
	{"openpatch_single",openpatch_single},
	{"ovrcextract",ovrcextract},
	{"r4crypt",r4crypt},
	{"r4dec",r4crypt},
	{"r4enc",r4crypt},
	{"r4ilscrypt",r4crypt},
	{"r4ilsdec",r4crypt},
	{"r4ilsenc",r4crypt},
	{"r4isdhc",r4isdhc},
	{"replaceloader",replaceloader},
	{"satdecode",satdecode},
	{"satencode",satencode},
	{"savconv",savconv},
	{"sdatexpand",sdatexpand},
	{"splitbootimg",splitbootimg},
	{"sprotectsearch",sprotectsearch},
	{"sucnv2ips",sucnv2ips},
	{"tobinary",xenobase16},
	{"unandrobook",unandrobook},
	{"unlzop",unlzop},
	{"unsnowflake",unsnowflake},
	{"unxorhtml",unxorhtml},
	{"unyomecolle",unyomecolle},
	{"updatecheat",updatecheat}, //C++(STL)
	{"xenobase16",xenobase16},
	{"xenobase32",xenobase32},
	{"xenobase32hex",xenobase32hex},
	{"xenobase64",xenobase64},
	{"xenobase85",xenobase85},
	{"xenobase85rfc",xenobase85rfc},
	{"xenobase91",xenobase91},
	{"xenobf",xenobf},
	{"xenobootfw",xenobootfw},
	{"xenocal",xenocal},
	{"xenocat",xenocat},
	{"xenociso",xenociso},
	{"xenocrypt",xenocrypt},
	{"xenodate",xenodate},
	{"xenodaxcr",xenodaxcr},
	{"xenoexcelbase",xenoexcelbase},
	{"xenofrob",xenofrob},
	{"xenofunzip",xenofunzip},
	{"xenogbatrim",xenogbatrim},
	{"xenogzip",xenogzip},
	{"xenohead",xenohead},
	{"xenoips",xenoips},
	{"xenojiso",xenojiso},
	{"xenondstrim",xenondstrim},
	{"xenopbp",xenopbp},
	{"xenoppf",xenoppf},
	{"xenotee",xenotee},
	{"xenounubinize",xenounubinize},
	{"xenoups",xenoups},
	{"xenouuencode",xenouuencode},
	{"xenowips",xenowips},
	{"xenoxxencode",xenoxxencode},
	{"xenozeller",xenozeller},
	{"yspatch",yspatch},
	{"zlibrawstdio",zlibrawstdio},
	{"zlibrawstdio2",zlibrawstdio2},

//hash libraries
	{"xenoadler32",xenohash},
	{"xenobsdsum",xenohash},
	{"xenobz2crc32",xenohash},
	{"xenocksum",xenohash},
	{"xenocrc16",xenohash},
	{"xenocrc32",xenohash},
	{"xenoelf32",xenohash},
	{"xenomd5sum",xenohash},
	{"xenomoonguid",xenohash},
	{"xenosha1sum",xenohash},
	{"xenosha256sum",xenohash},
	{"xenosize",xenohash},
	{"xenosum32",xenohash},
	{"xenosysvsum",xenohash},
};
static const int appsize=sizeof(apps)/sizeof(applet);

int applets(const int argc, const char **argv){
	int i=TRIVIAL_INDEX;
	for(;i<appsize;i++){fprintf(stdout,"%s\n",apps[i].name);} //if(i<appsize-1)fputc(',',stdout);}
	return 0;
}

int _link(const int argc, const char **argv){
	if(argc<2){fprintf(stderr,"tell me xenobox path\n");return 1;}
	int i=USUAL_INDEX;
	if(!strcmp(argv[0],"link-full")||!strcmp(argv[0],"--link-full"))i=TRIVIAL_INDEX;
#if defined(WIN32) || !defined(__GNUC__)
	//int f=1;
	typedef int (__stdcall*CreateHardLinkA_type)(const char*, const char*, void*);
	CreateHardLinkA_type pCreateHardLinkA=(CreateHardLinkA_type)GetProcAddress(GetModuleHandleA("kernel32"),"CreateHardLinkA");
	if(!pCreateHardLinkA)fprintf(stderr,"link isn't supported, fallback to copy...\n");
	for(;i<appsize;i++){
		sprintf(cbuf,"%s.exe",apps[i].name);
		if(pCreateHardLinkA){
			int ret;
			ret=DeleteFileA(cbuf);
			//if(!ret)fprintf(stderr,"delete %s failed\n",cbuf);
			ret=pCreateHardLinkA(cbuf,argv[1],NULL);
			if(!ret){
				fprintf(stderr,
					"linking failed, fallback to copy...\n"
					"if NTFS I hope copying also fails\n"
				);
				pCreateHardLinkA=NULL;goto copy;
			}
		}else{
copy:
			CopyFileA(argv[1],cbuf,0);
		}
	}
#else
	for(;i<appsize;i++){
		sprintf(cbuf,"ln -sf %s %s",argv[1],apps[i].name);
		system(cbuf);
	}
#endif
	return 0;
}

static const char *_linkarg[]={
	"--link-full",
	"",
};
int _install(const int argc, const char **argv){
	const char *exe=argv[-1]; //bah!
	if(access(exe,0)){
		fprintf(stderr,
			"Fatal: cannot detect xenobox's presense.\n"
			"Perhaps you invoked xenobox from PATH.\n"
			"Please invoke xenobox using filepath.\n"
		);return 1;
	}
	_linkarg[1]=exe;
	return _link(2,_linkarg);
}

int license(const int argc, const char **argv){
	fprintf(stdout,
		"XenoBox itself is licensed under CC0 (aka Public Domain).\n"
		"libmshlsplash/memstream's licenses are the same :)\n"
		"\n"
		"zlib by Jean-loup Gailly / Mark Adler (zlib/libpng license)\n"
		"des cipher by Dr. B R Gladman\n"
		"gbalzss by Haruhiko Okumura / Andre Perrot\n"
		"hc128 cipher by Hongjun Wu\n"
		"libnsbmp by Richard Wilson / Sean Fox (MIT License)\n"
		"md5 digest by RSA Data Security (BSD-like License)\n"
		"minIni by ITB CompuPhase (Apache License 2.0)\n"
		"rijndael cipher by Philip J. Erdelsky\n"
		"sha1 digest by WIDE Project (BSD License)\n"
		"sha256 digest by Igor Pavlov\n"
		"\n"
		"DSTwo encryption by CuteMiyu\n"
		"rc4 cipher by Pekka Pessi\n"
		"iso reader by takka (as iso_tool)\n"
		"nitrofs expander by loveemu (MIT License)\n"
		"funzip by Info-ZIP (BSD-like License)\n"
	);
	return 0;
}

int moo(const int argc, const char **argv){
	fprintf(stdout,
" -----\n"
"| moo |\n"
" -----\n"
"         ^__^\n"
"         (oo)\\_______\n"
"         (__)\\       )\\/\\\n"
"             ||-----||   *\n"
"             ||     ||\n"
	);
	return 0;
}

__attribute__((noreturn)) static void usage(){
	const char *arch=
#if defined(__i386__)
	"i386"
#elif defined(__amd64__)
	"amd64"
#elif defined(__ia64__)
	"ia64"
#elif defined(__arm__)
	"arm"
#elif defined(__mips__)
	"mips"
#else
	"other"
#endif
	;
	int bits=sizeof(size_t)*CHAR_BIT;
	fprintf(stderr,
		"XenoBox multi-call binary under CC0 (%s %dbit)\n"
		"Revision %d (Built on "__DATE__")\n"
		"xenobox [applet] [arg]\n"
		"xenobox --list: show list of applets\n"
		"xenobox --link[-full] /path/to/xenobox\n"
		"/path/to/xenobox --install: make applet link to xenobox\n"
		"xenobox --license: show license notice\n"
	,arch,bits,XENOBOX_REVISION);
	exit(-1);//while(1);
}

int main(const int argc, const char **argv){
	int f=0,i=0;
#if defined(WIN32) || !defined(__GNUC__)
	char sep='\\';
#else
	char sep='/';
#endif
	char *exe;
	initstdio();
	xor_srand((unsigned int)time(NULL)^(getpid()<<16));

	for(i=strlen(argv[0])-1;i>=0;i--){
		if(argv[0][i]==sep)break;
		if(!f&&argv[0][i]=='.')*(char*)(argv[0]+i)=f=0;
	}
	exe=(char*)argv[0]+i+1;
	for(i=TRIVIAL_INDEX;i<appsize;i++)if(!strcasecmp(exe,apps[i].name))return apps[i].func(argc,argv);
	if(argc<2)usage();
	for(i=0;i<appsize;i++)if(!strcasecmp(argv[1],apps[i].name))return apps[i].func(argc-1,argv+1);
	usage();
}
