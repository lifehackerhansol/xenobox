#!/bin/sh
make clean
echo "Self (clang)"
#cannot use -flto?
CLANG=1 ARCH="-march=native" LIBS="-ldl" SUFF=_clang make
make clean

upx-lzma --best --lzma xenobox_clang
