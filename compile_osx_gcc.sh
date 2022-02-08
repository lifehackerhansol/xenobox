#!/bin/sh
#You are highly encouraged to use compile_osx_clang.sh on Lion or later.

make clean
echo "i686"
ARCH="-arch i686 -mtune=core2" SUFF=_osx_i686 make
make clean

#removed. anyway xenobox isn't intended to big endian machines.
#echo "ppc"
#ARCH="-arch ppc" SUFF=_osx_ppc make
#make clean

echo "x86_64"
ARCH="-arch x86_64 -mtune=core2" SUFF=_osx_x86_64 make
make clean

upx --best --lzma xenobox_osx_i686 xenobox_osx_x86_64
lipo xenobox_osx_i686 xenobox_osx_x86_64 -create -output xenobox_osx
rm xenobox_osx_i686 xenobox_osx_x86_64
