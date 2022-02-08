#!/bin/sh

#not for production; uSTL doesn't allow -fno-rtti. libstdc++ allows it. That's all.

#CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ AR=i686-w64-mingw32-ar RANLIB=i686-w64-mingw32-ranlib configure --with-static --without-shared --without-libstdc++ --without-bounds --without-fastcopy --without-mmx

#make clean
#echo "Self"
#ARCH="-march=native -flto" LIBS="-ldl" make
make clean

echo "Win32 uSTL"
WIN32=1 USTL=1 ARCH=-flto INCLUDES=ustl LIBS=../ustl/libustl_win32.a PREFIX=i686-w64-mingw32- SUFF=.exe make
make clean

echo "Win64 uSTL"
WIN32=1 USTL=1 ARCH="-mtune=core2 -flto" INCLUDES=ustl LIBS=../ustl/libustl_win64.a PREFIX=x86_64-w64-mingw32- SUFF=_amd64.exe make
make clean

upx --best --lzma xenobox.exe
#upx --best --lzma xenobox xenobox.exe
