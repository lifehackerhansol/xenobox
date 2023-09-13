#include "../xenobox.h"

static unsigned int search(unsigned char* mem, unsigned int size, unsigned int base, unsigned char* check,
                           unsigned int lcheck)
{
    if (size < 100)
    {
        fprintf(stderr, "too small\n");
        return 0;
    }

    unsigned int i = 0, count = 0;
    for (; i < size - 100; i += 4)
    {
        if (!memcmp(mem + i, check, lcheck))
        {
            // x=((mem[i+1]&0x0f)<<8)|mem[i];
            // p=i+x+8;
            // fprintf(stderr,"%08x %08x -> %08x\n",i,p,read32(mem+p));
            fprintf(stderr, "Offset=%08x Virtual=%08x\n", i, i + base);
            count++;
        }
    }

    if (count == 0)
    {
        fprintf(stderr, "no hits (search error)\n");
        return 0;
#if 0
	}else if(count>1){
		fprintf(stderr,"multiple hits (search error)\n");return 0;
	}else if(!isatty(fileno(stdout))){
		//printf("%08x",read32(mem+p));
#endif
    }
    return count; //...
}

int mmcprotectsearch(const int argc, const char** argv)
{
    unsigned char* mem;
    unsigned int size;
    FILE* f;

    if (argc < 3)
    { // && isatty(fileno(stdin))){
        fprintf(stderr, "mmc_protect_inf searcher\n"
                        "mmcprotectsearch kernel base_address search_value...\n"
                        "base_address is usually CONFIG_PAGE_OFFSET+0x8000\n");
        return 1;
    }

#if 0
	if(!isatty(fileno(stdin))){
		fprintf(stderr,"stdin: ");
		size=0x1000000; //16MB
		mem=(unsigned char*)malloc(size);
		if(!mem){fprintf(stderr,"cannot alloc memory\n");goto stdin_end;}
		size=fread(mem,1,size,stdin);
		fprintf(stderr,"size=%u ",size);
		search(mem,size);

		free(mem);
stdin_end:;
	}
#endif

    unsigned int base = strtoul(argv[2], NULL, 0), lcheck = 0;
    memset(buf, 0, BUFLEN);
    int i = 3;
    for (; i < argc; i++)
        buf[4 * (i - 3)] = strtol(argv[i], NULL, 0), lcheck += 4;

    // for(;c<argc;c++){
    //	fprintf(stderr,"%s: ",argv[c]);
    f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "cannot open kernel\n");
        return 1;
    } // continue;}
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size > 0x2000000)
    {
        fprintf(stderr, "too big\n");
        fclose(f);
        return 1;
    } // continue;} //32MB
    mem = (unsigned char*)malloc(size);
    if (!mem)
    {
        fprintf(stderr, "cannot alloc memory\n");
        fclose(f);
        return 1;
    } // continue;}
    fread(mem, 1, size, f);

    search(mem, size, base, buf, lcheck);

    free(mem);
    // break;
    //}
    return 0;
}
