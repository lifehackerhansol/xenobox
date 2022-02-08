#include "../xenobox.h"

int xenodice(const int argc, const char **argv){
	u32 i=0,d=0,t=0;

	if(argc<2)goto xenodice_err;

	sscanf(argv[1],"%ud%u",&i,&d);
	if(i<1||d<1)sscanf(argv[1],"%uD%u",&i,&d);
	if(i<1||d<1)goto xenodice_err;

	printf("%s (expectation %u%s):\n",argv[1],(i+1)*d/2,(((i+1)*d)&1)?".5":"");

	for (;d;d--){
		int n=xor_rand()%i+1;
		printf("%u",n);
		if(d>1)putchar(' ');
		t+=n;
	}
	printf("\nresult: %u\n",t);

	return 0;

xenodice_err:
	fprintf(stderr,
		"DiceExpression\n"
		"Usage: xenodice ?d?/?D?\n"
	);
	return -1;
}
