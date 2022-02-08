/*
	libmshlsplash: MakeSplash re-implementation

	u32 flags; //m3sakura has 0x496e614d(ManI)
	u32 frames;

	//mshl1 / iSakuReal
	typedef struct{
		u32 VSyncCount;
		u32 FileOffset;
		u32 FileSize;   //comp
		u32 DecompSize; //decomp
	} splash1;
	//mshl1: gbalzss data continues
	//iSakuReal:
		4bytes palette length
		2*(length)bytes palette
		then lzss data continues. fastlzss16decpalasm_decode, see iSakuRealDecode()

	//mshl2
	typedef struct{
		u32 VSyncCount;
		u32 FileOffset;
		u32 ImageSize;   //decomp
		u32 b15BufCount; //comp
	} splash2;
	//1st frame: zlib data
	//then difference data continues

	//m3sakura
	u32 FileOffset
	* ImageSize===0x18000
	* FPS===15
*/

#if defined(FEOS) || !(defined(ARM9) || defined(ARM7))
/*
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
#include <stdbool.h>
*/
#include "../xenobox.h"
#endif

#if !defined(LIBPRISM_MEMSTREAM)
#include "memstream.h"
#endif

//mSPL
#define libmshlsplash_signature 0x4c50536d

typedef struct{
	u32 vsync;
	u32 offset;
	u32 size1;
	u32 size2;
} splash_frame;

typedef struct{
	u32 sig;
	FILE *f;
	int version;
	u32 flags;
	u32 frame;
	u32 fps;
	splash_frame *head;
	u16 *compbuf;
	u16 *decompbuf;
	u16 *firstbuf;
} splash;

#define SPLASH_MOONSHELL1 0x010
#define SPLASH_MOONSHELL2 0x020

//unsupported
#define SPLASH_ISAKUREAL  0x110
#define SPLASH_M3SAKURA   0x120

void gbalzssEncode(memstream *infile, memstream *outfile, bool VRAMSafe);
void gbalzssDecode(memstream *infile, memstream *outfile);

void gbalzssEncodeFile(FILE *infile, FILE *outfile, bool VRAMSafe);
void gbalzssDecodeFile(FILE *infile, FILE *outfile);

int iSakuRealDecode(memstream *comp,memstream *decomp,memstream *decompbuf,u32 palcnt,u16 *pal);

int zlib_decompress(const void *compbuf, const u32 compsize, void *decompbuf, const u32 decompsize);
int zlib_compress(const void *decompbuf, const u32 decompsize, void *compbuf, const u32 compsize);

//extract: ramdom access is allowed
splash* opensplash(const char *path);
int decompressimage(splash* handle, int idx);
int closesplash(splash* handle);
u32 getimagesize(splash* p, int idx); //pass it to fwrite. To use with u16*, you have to divide by 2.

//create: random access isn't allowed
int createsplash(const char *path, int version);
