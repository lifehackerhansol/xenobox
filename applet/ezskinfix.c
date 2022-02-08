#include "../xenobox.h"

static const char *binreplacearg1[]={
	"binreplace",
	"",
	"/x14SCB_BG.bmpSCB_BG.b15",
	"/x0aSCB_BG.b15/0/0/0/0/0/0/0/0/0/0",
};

static const char *binreplacearg2[]={
	"binreplace",
	"",
	"/x14SCD_BG.bmpSCD_BG.b15",
	"/x0aSCD_BG.b15/0/0/0/0/0/0/0/0/0/0",
};

static const char *binreplacearg3[]={
	"binreplace",
	"",
	"/x14SCE_BG.bmpSCE_BG.b15",
	"/x0aSCE_BG.b15/0/0/0/0/0/0/0/0/0/0",
};

static const char *binreplacearg4[]={
	"binreplace",
	"",
	"/x14SCN_BG.bmpSCN_BG.b15",
	"/x0aSCN_BG.b15/0/0/0/0/0/0/0/0/0/0",
};

static const char *binreplacearg5[]={
	"binreplace",
	"",
	"/x14SCW_BG.bmpSCW_BG.b15",
	"/x0aSCW_BG.b15/0/0/0/0/0/0/0/0/0/0",
};

static const char *binreplacearg6[]={
	"binreplace",
	"",
	"/x20progress_bar.bmpprogress_bar.b15",
	"/x10progress_bar.b15/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0",
};

int ezskinfix(const int argc, const char **argv){
	int i=1;//2;

	//fprintf(stderr,"ezskinfix\n");
	if(argc<i+1){
		fprintf(stderr,
			"Usage: ezskinfix skn...\n"
		);
		return -1;
	}
	for(;i<argc;i++){
		fprintf(stderr,"[%s]\n",argv[i]);
		binreplacearg1[1]=binreplacearg2[1]=binreplacearg3[1]=
		binreplacearg4[1]=binreplacearg5[1]=binreplacearg6[1]=argv[i];
		binreplace(4,binreplacearg1);
		binreplace(4,binreplacearg2);
		binreplace(4,binreplacearg3);
		binreplace(4,binreplacearg4);
		binreplace(4,binreplacearg5);
		binreplace(4,binreplacearg6);
	}
	return 0;
}
