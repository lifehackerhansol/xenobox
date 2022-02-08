#!/bin/sh
make clean
echo "iPhone (JailBroken)"
ARCH=-mthumb PREFIX=arm-apple-darwin9- SUFF=_iphone make
make clean

#upx-lzma --best --lzma xenobox_iphone
ldid -S xenobox_iphone
