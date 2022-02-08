#include <stdlib.h>
#include <stdio.h>
//#include <fcntl.h> //stdin/stdout are already initialized by xenobox.

#define BE 1 /*LE -> 0 BE -> 1 */
#define out(a,b) if(BE){fputc((a),stdout);fputc((b),stdout);}else{fputc((b),stdout);fputc((a),stdout);}

int unxorhtml(const int argc, const char **argv){
	int a,r=0,e=0,k;

	if(argc<2){
		fprintf(stderr,
			"UnXorHtml V.03 Usage: unxorhtml k < s.in >out.txt\n"
			"Output is UTF16 with BOM.\n"
			"Please specify magic number(k).\n"
		);
		return 1;
	}

	k=strtol(argv[1],NULL,0);
	out(0xfe,0xff);

	while(!feof(stdin)){
		a=fgetc(stdin);
		if(a==36){
			r=1;
			a=fgetc(stdin);
		}

		if(a==32){
			a=(fgetc(stdin)-48)^k;
		}else if(a==33){
			a=(fgetc(stdin)+77)^k;
		}else if(a==35){
			a=(fgetc(stdin)+141)^k;
		}else{
			a=a^k;
		}

		if(r==1){
			r=2;
			e=a;
		}else if(r==2){
			r=0;
			out(a,e);
		}else{
			out(0,a);
		}
	}
	return 0;
}
