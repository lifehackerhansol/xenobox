#!/bin/sh

make clean
echo "NDS FeOS"

#need to configure too many flags... let FeOS SDK configure it.
#ARCH="-mthumb-interwork -mword-relocations -save-temps -fvisibility=hidden -g -funwind-tables -DFEOS -DUSTL -Disatty\(x\)=0 -nostdinc -I${FEOSSDK}/include -I${FEOSSDK}/include/cxx -I${FEOSSDK}/userlib/ustl/include" PREFIX=arm-eabi- \
#LIBS="-nostartfiles -nostdlib -T ${FEOSSDK}/bin/fxe2.ld -Wl,-d,-q,-Map,xenobox.map -L${FEOSSDK}/lib -L${FEOSSDK}/userlib/ustl/lib -lustl -lfeos -lfeoscxx" SUFF=.elf make

make -f Makefile.FeOS install
#make -f Makefile.FeOS package #well...
#"make -f Makefile.FeOS clean" will remove fx2 too...
make clean
rm -f xb.elf xb.elf.dbg
