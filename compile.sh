#!/bin/sh
make clean
echo "Self"
ARCH="-march=native -flto" LIBS="-ldl" make
make clean

#echo "Win32"
#WIN32=1 ARCH=-flto PREFIX=i686-w64-mingw32- SUFF=.exe make
#make clean

#echo "Win64"
#WIN32=1 ARCH="-mtune=core2 -flto" PREFIX=x86_64-w64-mingw32- SUFF=_amd64.exe make
#make clean

#On Debian, use 3.08-2 or later.
upx --best --lzma xenobox
# xenobox.exe
