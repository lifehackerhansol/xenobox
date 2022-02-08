// ds2plug.c provides ds2(decrypt|make)plug(), based on (decrypt|make)plug.cpp by CuteMiyu.

// Theoretically,
// mm_decPlugin supports all input/output as stdin/stdout.
// mm_splitPlugin supports all input/output as stdin/stdout.
// mm_makePlugin supports plg output as stdout.
// What about frontends...?

#include "../xenobox.h"

static void ds2crypt(int fcrypt, u32 Offset, u32 Key, u32* DataBuf){
	u32 OutData;
	u32 OutDataRotate;

	Offset += lrotr(Offset, 7);
	Offset += lrotr(Offset, 2);
	OutDataRotate = 0;

	int i=0;
	for(; i<128; i++){
		OutData = DataBuf[i] ^ OutDataRotate ^ Key ^ Offset;
		Key = lrotr(Key, 10);
		OutDataRotate = lrotr(fcrypt?OutData:DataBuf[i], 15); ///
		Offset += 0x561A9C1A;
		DataBuf[i] = OutData;
	}
}

static int mm_decPlugin(const char *plugName, const char *rawName){
	int Result;
	u32 Seed;
	u32 Data[128];
	//u32 outData[128];

	FILE *fp_in;
	FILE *fp_out;

	fp_in = fopen(plugName, "rb");
	if(fp_in){
		fp_out = fopen(rawName, "wb");
		if(fp_out){
			//fread(&Seed, 1, sizeof(Seed), fp_in);
			//fseek(fp_in, 0, SEEK_SET);
			fread(Data, 1, sizeof(Data), fp_in);
			Seed = read32(Data)^0x8B12BAB6;
			u32 in_pos=0;
			u32 Key = Seed;
			ds2crypt(0, in_pos, Key, Data);
			fwrite(Data, 1, 512, fp_out);
			in_pos+=sizeof(Data);
			for(;;){
				Key = Seed ^ ((in_pos >> 16) + in_pos + (in_pos >> 8) + (in_pos >> 24));
				memset(Data, 0, sizeof(Data));
				size_t n = fread(Data, 1, sizeof(Data), fp_in);
				if(!n)break;
				ds2crypt(0, in_pos, Key, Data);
				fwrite(Data, 1, n, fp_out);
				in_pos+=sizeof(Data);
			}
			fclose(fp_in);
			fclose(fp_out);
			Result = 0;
		}else{
			fclose(fp_in);
			Result = -2;
		}
	}else{
		Result = -1;
	}
	return Result;
}

//this should be used for debugging. For ordinally things, please use ds2splitplug instead.
int ds2decryptplug(int argc, const char **argv){
	int ErrRet = 0;
	if (argc > 2){
		ErrRet = mm_decPlugin(argv[1], argv[2]);
		switch (ErrRet){
		case -1:
			fprintf(stderr,"can't open: %s\n", argv[1]);
			break;
		case -2:
			fprintf(stderr,"can't create: %s\n", argv[2]);
			break;
		default:			
			fprintf(stderr,"generate output file: %s success\n", argv[2]);
			break;
		}
	}else{
		fprintf(stderr,"argument error...\n");
		fprintf(stderr,"correct format is: ds2decryptplug source.plg raw_output\n");
	}
	return 0;
}

static int mm_splitPlugin(const char *plugName, const char *rawName, const char *firmwareName){
	int Result;
	u32 Seed;
	u32 Head[128];
	u32 Data[128];
	//u32 outData[128];

	FILE *fp_in;
	FILE *fp_out;
	FILE *fp_firmware;

	fp_in = fopen(plugName, "rb");
	if(fp_in){
		fp_out = fopen(rawName, "wb");
		if(fp_out){
			//fread(&Seed, 1, sizeof(Seed), fp_in);
			//fseek(fp_in, 0, SEEK_SET);
			fread(Head, 1, sizeof(Head), fp_in);
			Seed = read32(Head)^0x8B12BAB6;
			u32 in_pos=0,size;
			u32 Key = Seed;
			ds2crypt(0, in_pos, Key, Head);
			in_pos+=sizeof(Head);

			if((Head[4]&0x1ff) || (Head[8]&0x1ff)){
				fclose(fp_in);
				fclose(fp_out);
				return -1;
			}

			for(;in_pos!=Head[4];){
				fread(Data, 1, sizeof(Data), fp_in);
				in_pos+=sizeof(Data);
			}

			for(size=Head[5];size;){
				Key = Seed ^ ((in_pos >> 16) + in_pos + (in_pos >> 8) + (in_pos >> 24));
				memset(Data, 0, sizeof(Data));
				fread(Data, 1, sizeof(Data), fp_in);
				ds2crypt(0, in_pos, Key, Data);
				fwrite(Data, 1, min(size,sizeof(Data)), fp_out);
				in_pos+=sizeof(Data);
				size-=min(size,sizeof(Data));
			}

			if(firmwareName){fp_firmware = fopen(firmwareName, "wb");
			if(fp_firmware){
				for(;in_pos!=Head[8];){
					fread(Data, 1, sizeof(Data), fp_in);
					in_pos+=sizeof(Data);
				}

				for(size=Head[9];size;){
					Key = Seed ^ ((in_pos >> 16) + in_pos + (in_pos >> 8) + (in_pos >> 24));
					memset(Data, 0, sizeof(Data));
					fread(Data, 1, sizeof(Data), fp_in);
					ds2crypt(0, in_pos, Key, Data);
					fwrite(Data, 1, min(size,sizeof(Data)), fp_firmware);
					in_pos+=sizeof(Data);
					size-=min(size,sizeof(Data));
				}
				fclose(fp_firmware);
			}}

			fclose(fp_in);
			fclose(fp_out);
			Result = 0;
		}else{
			fclose(fp_in);
			Result = -2;
		}
	}else{
		Result = -1;
	}
	return Result;
}

int ds2splitplug(int argc, const char **argv){
	int ErrRet = 0;
	if (argc > 2){
		ErrRet = mm_splitPlugin(argv[1], argv[2], argc>3?argv[3]:NULL);
		switch (ErrRet){
		case 1:
			fprintf(stderr,"format error\n");
			break;
		case -1:
			fprintf(stderr,"can't open: %s\n", argv[1]);
			break;
		case -2:
			fprintf(stderr,"can't create: %s\n", argv[2]);
			break;
		default:			
			fprintf(stderr,"generate output file: %s success\n", argv[2]);
			break;
		}
	}else{
		fprintf(stderr,"argument error...\n");
		fprintf(stderr,"correct format is: ds2splitplug source.plg raw_output [ds2_firmware.dat]\n");
	}
	return 0;
}

static int mm_makePlugin(const char *binName, const char *firmwareName, const char *plgName, u32 Addr1, u32 Addr2){
	int Result;
	FILE *fp_in;
	FILE *fp_out;
	FILE *fp_firmware;
	u32 binSize;
	u32 firmwareSize;
	u32 Data[128];
	//u32 outData[128];

	fp_in = fopen(binName, "rb");
	if(fp_in){
		fp_firmware = fopen(firmwareName, "rb");
		if(fp_firmware){
			fp_out = fopen(plgName/*".0temp0"*/, "wb");
			if(fp_out){
				//fseek(fp_in, 0, SEEK_END);
				binSize = filelength(fileno(fp_in)); //ftell(fp_in);
				//fseek(fp_firmware, 0, SEEK_END);
				firmwareSize = filelength(fileno(fp_firmware)); //ftell(fp_firmware);
				//fseek(fp_in, 0, SEEK_SET);
				//fseek(fp_firmware, 0, SEEK_SET);

				//srand((unsigned int)time(NULL));
				int Seed = xor_rand() ^ 0x79FA8917;
				u32 in_pos = 0;
				u32 Key = Seed;

				memset(Data, 0, sizeof(Data));
				Data[0] = 0x8B12BAB6;
				Data[1] = 0x93D7DE9B;
				Data[2] = 0xCDD8D0D2;
				Data[3] = 0x7E5DEB16;
				Data[4]  = 0x200;
				Data[5]  = binSize;
				Data[6]  = Addr1;
				Data[7]  = Addr2;
				Data[8]  = align512(binSize+0x200); //(binSize + 0x3FF) & 0xFFFFFE00;
				Data[9]  = firmwareSize;
				ds2crypt(1, in_pos, Key, Data);
				fwrite(Data, 1, sizeof(Data), fp_out);
				in_pos+=sizeof(Data);

				for(;;){
					memset(Data, 0, sizeof(Data));

					Key = Seed ^ ((in_pos >> 16) + in_pos + (in_pos >> 8) + (in_pos >> 24));
					size_t n = fread(Data, 1, sizeof(Data), fp_in);
					if(!n)break;
					ds2crypt(1, in_pos, Key, Data);
					fwrite(Data, 1, sizeof(Data)/*n*/, fp_out);
					in_pos+=sizeof(Data);
				}
#if 0
				u32 patSize = 0x200-(binSize&0x1ff);if(patSize==0x200)patSize=0;//Data[8] - ftell(fp_out);
				memset(Data, 0, sizeof(Data));
				fwrite(Data, 1, patSize, fp_out);
#endif
				for(;;){
					Key = Seed ^ ((in_pos >> 16) + in_pos + (in_pos >> 8) + (in_pos >> 24));
					size_t n = fread(Data, 1, sizeof(Data), fp_firmware);
					if(!n)break;
					ds2crypt(1, in_pos, Key, Data);
					fwrite(Data, 1, n, fp_out);
					in_pos+=sizeof(Data);
				}
				fclose(fp_in);
				fclose(fp_firmware);
				fclose(fp_out);
				Result = 0;
#if 0
				fp_in = fopen(".0temp0", "rb");
				if (fp_in)
				{
					fp_out = fopen(plgName, "wb");
					if (fp_out)
					{
						//fseek(fp_in, 0, SEEK_SET);
						//fseek(fp_out, 0, SEEK_SET);

						for(;;){
							u32 Key = Seed ^ ((in_pos >> 16) + in_pos + (in_pos >> 8) + (in_pos >> 24));
							memset(Data, 0, sizeof(Data));
							size_t n = fread(Data, 1, sizeof(Data), fp_in);
							if(!n)break;
							ds2crypt(1, in_pos, Key, Data);
							fwrite(Data, 1, n, fp_out);
							in_pos+=sizeof(Data);
						}
						fclose(fp_in);
						fclose(fp_out);
						unlink(".0temp0");
						Result = 0;
					}
					else
					{
						fclose(fp_in);
						Result = -5;
					}
				}
				else
				{
					Result = -4;
				}
#endif
			}
			else
			{
				fclose(fp_in);
				fclose(fp_firmware);
				Result = -3;
			}
		}
		else
		{
			fclose(fp_in);
			Result = -2;
		}
	}
	else
	{
		Result = -1;
	}

	return Result;
}

static char firmwareName[768];
//static char binName[768];
static char plgName[768];
int ds2makeplug(const int argc, const char **argv){
	//printf("start\n");

	if (argc > 2){
		//strcpy(binName, argv[1]);

		if(argc > 3){
			strcpy(firmwareName,argv[2]);
			strcpy(plgName, argv[3]);
		}else{
#if !defined(_DEBUG)
			strcpy(firmwareName, argv[0]);
			char *pSlash = strrchr(firmwareName, '/');
			if(!pSlash)pSlash = strrchr(firmwareName, '\\');
			if(pSlash)
				strcpy(pSlash+1, "ds2_firmware.dat");
			else
#endif
				strcpy(firmwareName, "ds2_firmware.dat");
			strcpy(plgName, argv[2]);
		}

		const char *pExtName = strrchr(plgName, '.');

		if (pExtName)
		{
			if (strcasecmp(pExtName, ".plg"))
			{
				pExtName = NULL;
				strcat(plgName, ".plg");
			}
		}
		else
		{
			strcat(plgName, ".plg");
		}

		int ErrRet = mm_makePlugin(argv[1], firmwareName, plgName, 0x80002000, 0x80002000);

		switch (ErrRet)
		{
		case -1:
			fprintf(stderr,"can't open: %s\n", argv[1]);
			break;
		case -2:
			fprintf(stderr,"can't open: %s\ncheck if it on the same directory as makeplug program\n", firmwareName);
			break;
		//case -5:
		//	fprintf(stderr,"can't creat temporary file, dose the disk have enough space?\n");
		//	break;
		//case -4:
		//	fprintf(stderr,"Strange? a temporary file had created, but can't open now...\n");
		//	break;
		case -3:
			fprintf(stderr,"can't creat output file: %s\n", plgName);
			break;
		default:
			fprintf(stderr,"generate output file: %s success\n", plgName);
			break;
		}
	}
	else
	{
		fprintf(stderr,"argument error...\n");
		fprintf(stderr,"correct format is: ds2makeplug source [ds2_firmware.dat] output.plg\n");
		fprintf(stderr,"\"output\" is your wanted name\n");
		fprintf(stderr,"and make sure \"ds2_firmware.dat\" on the same directory as ds2makeplug\n");
		fprintf(stderr,"if ds2makeplug is searched from $PATH (you don't use slash),\n");
		fprintf(stderr,"\"ds2_firmware.dat\" on current directory will be used.\n");
	}

	return 0;
}




