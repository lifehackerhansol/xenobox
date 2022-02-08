#!/bin/sh
make clean
echo "Self (Open64)"
OPEN64=1 ARCH="-mmmx -msse" LIBS="-ldl -nolibopen64rt" SUFF=_open64 make
make clean

upx-lzma --best --lzma xenobox_open64
