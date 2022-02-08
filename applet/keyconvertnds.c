#include "../xenobox.h"
enum KEYPAD_BITS{
	_KEY_A = BIT(0), _KEY_B = BIT(1), _KEY_SELECT = BIT(2), _KEY_START = BIT(3),
	_KEY_RIGHT = BIT(4), _KEY_LEFT = BIT(5), _KEY_UP = BIT(6), _KEY_DOWN = BIT(7),
	_KEY_R = BIT(8), _KEY_L = BIT(9), _KEY_X = BIT(10), _KEY_Y = BIT(11),
	_KEY_TOUCH = BIT(12), _KEY_LID = BIT(13)
};

int keyconvertnds(const int argc, const char **argv){
	if(argc<2){fprintf(stderr,"keyconvertnds [int|keys]\n");return 1;}
	unsigned int keys=strtoul(argv[1],NULL,0);
	int i=0;
	if(keys){ //int to keys
#define INTTOKEY(KEY,KEY_PRINT) if(keys&_KEY_##KEY){if(i)putchar('+');i++;printf(#KEY_PRINT);}
			INTTOKEY(A,A)
			INTTOKEY(B,B)
			INTTOKEY(SELECT,SELECT)
			INTTOKEY(START,START)
			INTTOKEY(RIGHT,RIGHT)
			INTTOKEY(LEFT,LEFT)
			INTTOKEY(UP,UP)
			INTTOKEY(DOWN,DOWN)
			INTTOKEY(R,R)
			INTTOKEY(L,L)
			INTTOKEY(X,X)
			INTTOKEY(Y,Y)
			INTTOKEY(TOUCH,TOUCH)
			INTTOKEY(LID,LID)
#undef INTTOKEY
		putchar('\n');
	}else{ //keys to val
		char *buttons=(char*)argv[1];
		for(;*buttons;){
			for(i=0;buttons[i];i++){if(!buttons[i]||buttons[i]==','||buttons[i]=='+'||buttons[i]=='|')break;}
#define KEYTOINT(KEY_PRINT,KEY) if(strlen(#KEY_PRINT)==i&&!strncasecmp(buttons,#KEY_PRINT,i)){keys|=_KEY_##KEY;}
			KEYTOINT(A,A)
			KEYTOINT(B,B)
			KEYTOINT(SELECT,SELECT)
			KEYTOINT(START,START)
			KEYTOINT(RIGHT,RIGHT)
			KEYTOINT(LEFT,LEFT)
			KEYTOINT(UP,UP)
			KEYTOINT(DOWN,DOWN)
			KEYTOINT(R,R)
			KEYTOINT(L,L)
			KEYTOINT(X,X)
			KEYTOINT(Y,Y)
			KEYTOINT(TOUCH,TOUCH)
			KEYTOINT(LID,LID)
#undef KEYTOINT
			buttons+=i+1;
		}
		printf("0x%08x\n",keys);
	}
	return 0;
}
