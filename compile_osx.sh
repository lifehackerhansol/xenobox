#!/bin/sh
#compile using clang++. You need OSX Lion.
#Actually compiling xenobox using gcc isn't recommended on Lion or later. 

make clean
echo "i686"
LIBS="-Wl,-macosx_version_min,10.6 -lcrt1.10.5.o" CLANG=1 ARCH="-arch i686 -mtune=core2 -flto" SUFF=_osx_i686 make
make clean

#removed. anyway xenobox isn't intended to big endian machines.
#echo "ppc"
#ARCH="-arch ppc" SUFF=_osx_ppc make
#make clean

echo "x86_64"
LIBS="-Wl,-macosx_version_min,10.6 -lcrt1.10.5.o" CLANG=1 ARCH="-arch x86_64 -mtune=core2 -flto" SUFF=_osx_x86_64 make
make clean

./xenobox_osx_i686
./xenobox_osx_x86_64

upx --best --lzma xenobox_osx_i686 xenobox_osx_x86_64
lipo xenobox_osx_i686 xenobox_osx_x86_64 -create -output xenobox_osx
rm xenobox_osx_i686 xenobox_osx_x86_64
