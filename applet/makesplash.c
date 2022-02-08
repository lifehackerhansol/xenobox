#include <stdio.h>
#include "../lib/libmshlsplash.h"

int makesplash(const int argc, const char **argv){
	int type=SPLASH_MOONSHELL2,ret;
	if(argc<2){
		fprintf(stderr,
			"MakeSplash in C\n"
			"\tgbalzss by Haruhiko Okumura / Andre Perrot\n"
			"\tzlib by Jean-loup Gailly / Mark Adler\n"
			//"\tlibpng by Glenn Randers-Pehrson\n"
			"\tlibnsbmp by Richard Wilson / Sean Fox\n"
			"\tminIni by ITB CompuPhase\n"
			"makesplash splash.ini [1|2|m]\n"
			"1:MoonShell1 2:MoonShell2 s:iSakuReal (test) m:M3Sakura\n"
			"* If you don't specify type, MoonShell2 is automatically selected. *\n"
		);
		return 1;
	}
	if(argc>2){
		switch(argv[2][0]){
			case '1':type=SPLASH_MOONSHELL1;break;
			case '2':type=SPLASH_MOONSHELL2;break;
			case 's':type=SPLASH_ISAKUREAL;break;
			case 'm':type=SPLASH_M3SAKURA;break;
		}
	}
	ret=createsplash(argv[1],type);
	switch(ret){
		case 1:fprintf(stderr,"version error. please use 1 or 2.\n");break;
		case 2:fprintf(stderr,"configuration error\n");break;
		case 3:fprintf(stderr,"m3sakura can have up to 126 frames\n");break;
		case 10:fprintf(stderr,"file search error\n");break;
		case 20:fprintf(stderr,"decode error\n");break;
	}
	return ret;
}
