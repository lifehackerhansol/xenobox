// pspio.h : psp -> PC port helper

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifndef FEOS
#include <fcntl.h>
#else
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  00100
#define O_TRUNC  01000
#endif
#include <stdarg.h>
#include <dirent.h>

#ifndef XENOBOX
typedef unsigned char u8;
typedef unsigned char byte;
typedef unsigned short u16;
typedef unsigned int u32;

typedef char s8;
typedef short s16;
typedef int s32;

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#define OPEN_BINARY O_BINARY
#else
#define OPEN_BINARY 0
#endif
#endif

typedef size_t SceSize;
typedef int SceUID;
// typedef DIR*        SceUID;
typedef int SceMode;
typedef off_t SceOff;
// typedef time_t      ScePspDateTime;
typedef struct stat SceIoStat;

#define pspSdkSetK1(k1)     (0)
#define scePowerTick(tick)  (0)
#define sceKernelExitGame() exit(0)
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#include <windows.h>
#define sceKernelDelayThread(delay) Sleep(delay / 1000)
#else
// currently unused...
#include <unistd.h>
#define sceKernelDelayThread(delay) usleep(delay)
#endif
#define pspDebugScreenClear()
#define pspDebugScreenSetXY(x, y)

#define PSP_O_RDONLY  (O_RDONLY | OPEN_BINARY)
#define PSP_O_WRONLY  (O_WRONLY | OPEN_BINARY)
#define PSP_O_RDWR    (O_RDWR | OPEN_BINARY)
#define PSP_O_CREAT   O_CREAT
#define PSP_O_TRUNC   O_TRUNC
#define PSP_O_APPEND  O_APPEND
#define PSP_O_DIROPEN O_DIRECTORY
#define PSP_O_EXCL    O_EXCL

#define PSP_SEEK_SET SEEK_SET
#define PSP_SEEK_CUR SEEK_CUR
#define PSP_SEEK_END SEEK_END

#define FIO_S_IFMT  S_IFMT
#define FIO_S_IFLNK S_IFLNK
#define FIO_S_IFDIR S_IFDIR
#define FIO_S_IFREG S_IFREG

#define FIO_S_ISUID S_ISUID
#define FIO_S_ISGID S_ISGID
#define FIO_S_ISVTX S_ISVTX

#define FIO_S_IRWXU S_IRWXU
#define FIO_S_IRUSR S_IRUSR
#define FIO_S_IWUSR S_IWUSR
#define FIO_S_IXUSR S_IXUSR

#define FIO_S_IRWXG S_IRWXG
#define FIO_S_IRGRP S_IRGRP
#define FIO_S_IWGRP S_IWGRP
#define FIO_S_IXGRP S_IXGRP

#define FIO_S_IRWXO S_IRWXO
#define FIO_S_IROTH S_IROTH
#define FIO_S_IWOTH S_IWOTH
#define FIO_S_IXOTH S_IXOTH

#define sceIoOpen(name, flag, mode) open(name, flag, mode)
#define sceIoClose                  close
#define sceIoRead                   read
#define sceIoWrite                  write
#define sceIoLseek                  lseek
#define sceIoLseek32                lseek
#define sceIoGetstat                stat
#define sceIoRemove                 remove

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#define sceIoMkdir(path, mode) mkdir(path)
#else
#define sceIoMkdir mkdir
#endif
#ifdef FEOS
#define sceIoRmdir remove
#else
#define sceIoRmdir rmdir
#endif
#define sceIoChdir  chdir
#define sceIoRename rename

// very sorry but these are currently unsupported:
// sceIoDopen(opendir) / sceIoDread(readdir) / sceIoDclose(closedir) / sceIoChstat(ftruncate/chmod)

#define Kprintf printf
