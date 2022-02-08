#ifndef XENOBOX
#define XENOBOX

/*
 * XenoBox multi-call binary (under CC0)
 * The swiss army knife for manipulating files
 * 0904xx yspatch
 * 100207 dldipatch
 * 100309 kauralngasm/dis, m3dec
 * 100324 akaiodec
 * 100405 xenoips
 * 100411 fatpatch, guidpatch
 * 100510 m3make, dsbize
 * 100625 create, truncate [fileapplet's basis]
 * 100705 libfatreduce
 * 100707 h4xms2, msleep
 * 100723 binreplace
 * 100726 akextract
 * 100831 r4crypt
 * 100908 png2b15
 * 100910 breaksplash
 * 100911 dldirename
 * 100913 gbalzssrawstdio, cmdini
 * 100923 modifybanner
 * 101129 16bitbmp, dstwo2ismm, guidbreak
 * 101223 akextract_wood
 * 101225 checkcheatsize
 * 110107 updatecheat
 * 110118 extinfo2binary, openpatch
 * 110123 createff [xenobox's basis]
 * 110124 xenondstrim
 * 110213 ezskinfix
 * 110315 savconv
 * 110327 xenocrypt, xenobootfw
 * 110422 unlzop
 * 110425 openpatch_single
 * 110507 sucnv2ips, nsc, saten/decode
 * 110604 overall optimization, binreplace fixed. OSX / Win64 binary.
 * 110615 dldipatch nds as template fix.
 * 110627 file++
 * 110708 bin2cstdio, bin2sstdio, gunprx, xenogzip
 * 110710 gzprx, xenopbp, fcutdown
 * 110811 ds2decryptplug, ds2makeplug, ds2splitplug, fcutdown fixed.
 * 110823 xenopbp fixed, getebootid
 * 110902 sdatexpand, getdiscid, getebootid now supports to get TITLE
 * 110909 sdatexpand fix, xenofunzip
 * 110915 sdatexpand fix
 * 110917 decievo
 * 110918 dsapfilt, ilshield
 * 110926 unyomecolle
 * 111002 mergecheat, dlsymtest
 * 111003 mergecwc
 * 111018 nidcalc, gdd2011android, xenocrc32, xeno*sum, yspasscode, belon
 * 111120 ovrcextract
 * 111124 sprotectsearch, xenounubinize, wdayloader, fextend fix
 * 111126 xenoexcelbase, xenozeller
 * 111129 ndsarmsizefilter
 * 111205 checkumdsize, getdiscid_lite, zlibrawstdio2. Now 100 applets!
 * 111206 getdiscid supports dax(no NC) and jso, fixed fatal bug on Windows.
 * 111208 xenoadler32, xenocrc16, fixed memory leak of getdiscid.
 * 111209 many hashing libraries. xenobase64, xenocat, ilshield_3des/aes, gencrctable.
 * 111210 xenohash can read sum file from stdin.
 * 111215 keyconvertnds/psp.
 * 111230 tobinary, getjavacert. getdiscid can dump PARAM.SFO to stdout.
 * 120202 xenobase32, google2fa.
 * 120209 unsnowflake.
 * 120217 mmcprotectsearch, splitbootimg. OSX final.
 * 120302 fixed xenoips overall. xenoips can read ips from streamed stdin. xenoups (only patching).
 * 120303 xenociso, xenodaxcr, xenojiso, xenoups (creation), xenoppf. OSX update.
 * 120304 modified linking scheme on windows. xenobf.
 * 120324 unandrobook
 * 120330 kallsymslookupsearch
 * 120402 improved google2fa (from keys.txt)
 * 120420 xenouuencode, xenoxxencode, xenobase91. xenobase64 is now much faster.
 * 120421 updated toolchains...
 * 120514 xenobase85, xenobase85rfc.
 * 120524 now file++ works on Android Donut. fixed fatal bug in sdatexpand.
 * 120525 Initial port to NDS FeOS.
 * 120526 xenoalloc, xenodice, unxorhtml. createsplash supports FeOS.
 * 120531 improved extinfo2binary (still buggy though). fixed fatal bug of xenobase16.
 * 120613 fixed ciso/jiso handling on 64bit target.
 * 120614 updatecheat is launchable on uSTL.
 * 120623 xenowips. fcutdown accepts stdin.
 * 120630 mergecheat and mergecwc's target can be the same as source.
 * 120708 r4ilscrypt, r4brute.
 * 120715 kawen/decode.
 * 120822 reduced warnings. Open64 support.
 * 120914 fixed md5 on 64bit target.
 * 120927 akextract_wood2
 * 120930 getyomecolleuid
 * 121007 xenodate, xenofrob (nsc+), xenohead
 * 121015 xenofrob buffering, xenocat fix
 * 121127 m4c2m4a
 * 121219 androidlogo, xenocal
 * 130303 xenociso uses 7z.so and zopfli. fixed fatal bug of base.c.
 * 130314 xenotee
 */

#define XENOBOX_REVISION 130314

// You should use strcasecmp/ftruncate. On Windows, MinGW automatically translates it into stricmp/chsize.
// Never use _lrotl/r. use lrotl/r.

// VisualC++ / C++Builder won't be supported.

#ifdef __cplusplus
extern "C"{
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>

#include "lib/zlib/zlib.h"

typedef unsigned char byte;
#ifndef FEOS
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long int u64;

typedef char  s8;
typedef short s16;
typedef int   s32;
typedef long long int s64;
#endif

#if !defined(__cplusplus)
//typedef enum { false, true } bool; // <nds/ndstypes.h>
#include <stdbool.h>
#endif

#if !defined(__cplusplus) && !defined(min)
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define between(a,x,b) ((a)<=(x)&&(x)<=(b))

//I hope (sizeof(val)*CHAR_BIT-(rot)) will be precalculated in compilation.
#define lrotr(val,rot) (( (val)<<(sizeof(val)*CHAR_BIT-(rot)) )|( (val)>>(rot) ))
#define lrotl(val,rot) (( (val)<<(rot) )|( (val)>>(sizeof(val)*CHAR_BIT-(rot)) ))

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	#include <windows.h>
	#include <fcntl.h>
	#define sleep(t) Sleep(1000*(t))
	#define initstdio() setmode(fileno(stdin),O_BINARY),setmode(fileno(stdout),O_BINARY),setmode(fileno(stderr),O_BINARY);
	#define OPEN_BINARY O_BINARY
	//because of nasty msvcrt
	#define LLU "%I64u"
	#if defined(__GNUC__) || defined(__clang__) //fixme
		#include <unistd.h>
	#endif
#else
	#include <unistd.h>
	#include <sys/stat.h>
	int filelength(int fd);
	#define initstdio()
	#define OPEN_BINARY 0
	#define LLU "%llu"

	#ifdef FEOS //low level IO
		#define O_RDONLY           0
		#define O_WRONLY           1
		#define O_RDWR             2
		#define O_CREAT        00100
		#define O_TRUNC        01000
		#define O_APPEND       02000

		//fixme...
		//#define isatty(f) 0
		int open(const char *name, int flags);
		int close(int fd);
		int read(int fd, void *buf, size_t count);
		int write(int fd, void *buf, size_t count);
		int lseek(int fd, size_t off, int whence);
		int access(const char *name, int x);
		int strcasecmp(const char *s1, const char *s2);
		int strncasecmp(const char *s1, const char *s2, size_t n);
		unsigned long long strtoull(const char *s, char **endp, int base);
	#else
		#include <fcntl.h>
	#endif		

	#ifndef NODLOPEN //dynamic load
	#ifdef FEOS
		#include <feos.h>
		#undef ARM9
		#define LoadLibraryA(filename) FeOS_LoadModule(filename) 
		#define GetProcAddress FeOS_FindSymbol
		#define FreeLibrary FeOS_FreeModule
	#else
		#include <dlfcn.h>
		#define LoadLibraryA(filename) dlopen(filename,RTLD_NOW)
		#define GetProcAddress dlsym
		#define FreeLibrary dlclose
	#endif
	#endif
#endif

int sfilelength(const char *path);
int filemode(int fd);
int sfilemode(const char *path);

//p should be 2,4,8,...
#define align2p(p,i) (((i)+((p)-1))&~((p)-1))

#define align2(i) align2p(2,i)
#define align4(i) align2p(4,i)
#define align8(i) align2p(8,i)
#define align256(i) align2p(256,i)
#define align512(i) align2p(512,i)
#define swiCRC16 crc16

#define BIT(n) (1<<(n))

#define isRedirected(file) (!isatty(fileno(file)))

//you should care about BUFLEN if you use these functions in portable devices.
#ifdef FEOS
//512KB
#define BUFLEN (1<<19)
#else
//currently 4MB. some game cheat entry is more than 3MB...
#define BUFLEN (1<<22)
#endif
extern unsigned char buf[BUFLEN];
#define cbuf ((char*)buf)

//64KB
#define DECOMPBUFLEN (1<<16)
#define COMPBUFLEN   (DECOMPBUFLEN|(DECOMPBUFLEN>>1))
extern unsigned char __compbuf[COMPBUFLEN],__decompbuf[DECOMPBUFLEN];

//util
unsigned long long int read64(const void *p);
unsigned int read32(const void *p);
unsigned int read24(const void *p);
unsigned short read16(const void *p);
void write64(void *p, const unsigned long long int n);
void write32(void *p, const unsigned int n);
void write24(void *p, const unsigned int n);
void write16(void *p, const unsigned short n);

unsigned int read32be(const void *p);
unsigned int read24be(const void *p);
unsigned short read16be(const void *p);
void write32be(void *p, const unsigned int n);
void write24be(void *p, const unsigned int n);
void write16be(void *p, const unsigned short n);

char* myfgets(char *buf,int n,FILE *fp);
void msleep(int msec);

unsigned short crc16(unsigned short crc, const unsigned char *p, unsigned int size);
unsigned int crc32_left(unsigned int crc, const unsigned char *p, unsigned int size);

typedef void (*type_u32p)(u32*);

int strchrindex(const char *s, const int c, const int idx);
size_t _FAT_directory_mbstoucs2(unsigned short* dst, const unsigned char* src, size_t len);
u32 mbstoucs2(unsigned short* dst, const unsigned char* src);
size_t _FAT_directory_ucs2tombs(unsigned char* dst, const unsigned short* src, size_t len);
u32 ucs2tombs(unsigned char* dst, const unsigned short* src);
void NullMemory(void* buf, unsigned int n);

int memcmp_fast(const void *x,const void *y,unsigned int len);

//xorshift
unsigned int xor_rand();
void xor_srand(unsigned int seed);

//applet
#define F(name) int name(const int argc, const char **argv);
F(applets)
F(_link)
F(_install)
F(license)
F(create)
F(createff)
F(_truncate)
F(_extend)
F(_cutdown)
F(_msleep)

F(_16bitbmp)
F(a9_02000000)
F(akaio151)
F(akaiodec_v3a)
F(akaiodec_v4)
F(akextract)
F(akextract_ex4)
F(akextract_wood)
F(akextract_wood2)
F(androidlogo)
F(asn1derdump)
F(belon)
F(bin2cstdio)
F(bin2sstdio)
F(binreplace)
F(breaksplash)
F(checkcheatsize)
F(checkumdsize)
F(cmdini)
F(decievo)
F(dldipatch)
F(dldirename)
F(dlsymtest)
F(ds2decryptplug)
F(ds2makeplug)
F(ds2splitplug)
F(dsapfilt)
F(dsbize)
F(dstwo2ismm)
F(extinfo2binary)
F(ezskinfix)
F(fatpatch)
F(filepp)
F(fixheader)
F(fixloaderstartup)
F(gbalzssrawstdio)
F(gdd2011android)
F(gencrctable)
F(getdiscid)
F(getdiscid_lite)
F(getebootid)
F(getjavacert)
F(getyomecolleuid)
F(google2fa)
F(guidbreak)
F(guidpatch)
F(gunprx)
F(gzprx)
F(h4xms2)
F(ilshield_aes)
F(ilshield_des)
F(ilshield_3des)
F(kallsymslookupsearch)
F(kauralngasm)
F(kauralngdis)
F(kawdecode)
F(kawencode)
F(keyconvertnds)
F(keyconvertpsp)
F(libfatreduce)
F(m3dec)
F(m3make)
F(m3patch)
F(m4c2m4a)
F(makesplash)
F(mergecheat) //C++(STL)
F(mergecwc) //C++(STL)
F(mmcprotectsearch)
F(modifybanner)
F(moo)
F(mselink)
F(ndsarmsizefilter)
F(ndslink)
F(nidcalc)
F(nsc)
F(openpatch) //C++(STL)
F(openpatch_single)
F(ovrcextract)
F(r4brute)
F(r4crypt)
F(r4ilscrypt)
F(r4isdhc)
F(replaceloader)
F(satdecode)
F(satencode)
F(savconv)
F(sdatexpand)
F(splitbootimg)
F(sprotectsearch)
F(sucnv2ips)
//F(tobinary)
F(unandrobook)
F(unlzop)
F(unsnowflake)
F(unxorhtml)
F(unyomecolle)
F(updatecheat) //C++(STL)
F(wdayloader)
F(xenoadler32)
F(xenoalloc)
F(xenobase16)
F(xenobase32)
F(xenobase32hex)
F(xenobase64)
F(xenobase85)
F(xenobase85rfc)
F(xenobase91)
F(xenobf)
F(xenobootfw)
F(xenobsdsum)
F(xenobz2crc32)
F(xenocal)
F(xenocat)
F(xenociso)
F(xenocksum)
F(xenocrc16)
F(xenocrc32)
F(xenocrypt)
F(xenodate)
F(xenodaxcr)
F(xenodice)
F(xenoelf32)
F(xenoexcelbase)
F(xenofrob)
F(xenofunzip)
F(xenogbatrim)
F(xenogzip)
F(xenohash)
F(xenohead)
F(xenoips)
F(xenojiso)
F(xenomd5sum)
F(xenomoonguid)
F(xenondstrim)
F(xenopbp)
F(xenoppf)
F(xenosha1sum)
F(xenosha256sum)
F(xenosize)
F(xenosum32)
F(xenosysvsum)
F(xenotee)
F(xenounubinize)
F(xenoups)
F(xenouuencode)
F(xenowips)
F(xenoxxencode)
F(xenozeller)
F(yspatch)
F(zlibrawstdio)
F(zlibrawstdio2)
F(yzpasscode)
#undef F

typedef struct{
	char *name;
	int  (*func)(const int, const char**);
}applet;

#ifdef __cplusplus
}
#endif

#endif

