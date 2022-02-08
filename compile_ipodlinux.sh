#!/bin/sh

make clean
echo "iPodLinux"
ARCH=-DNODLOPEN PREFIX=arm-uclinux-elf- LIBS="-static" SUFF=_ipodlinux make
make clean

#upx-lzma --best --lzma xenobox_ipodlinux
