#!/bin/sh

# 1. put droid-ndk-gcc to $PATH then link to droid-ndk-{g++,ld,strip}
# 2. put specs to /opt/android-ndk/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/lib/gcc/arm-linux-androideabi/4.4.3/

make clean
echo "Android"
#PREFIX=arm-elf-linux-androideabi- LIBS="-ldl -static-libgcc" SUFF=_android make
ARCH=-mthumb PREFIX=droid-ndk- LIBS="-ldl /opt/android-ndk/sources/cxx-stl/gnu-libstdc++/libs/armeabi/libstdc++.a" SUFF=_android make
make clean

#upx-lzma --best --lzma xenobox_android
