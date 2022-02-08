#!/bin/sh
export NDSDIR="nds/"
export SPSDIR="data/NDS Music Player/"

for file in $NDSDIR*.nds;do
	echo $file
	sps=${file%%.nds}.sps
	xenobox sdatexpand s "$file" "$SPSDIR${sps#$NDSDIR}"
done
