#!/bin/sh

#spsgen advanced
#pros: can recognize subfolder
#cons: you need find and basename. lol on Windows.

export NDSDIR="nds/"
export SPSDIR="data/NDS Music Player/"

for file in `find "$NDSDIR" -name "*.nds"`;do
	echo $file
	name=`basename "$file"`
	sps=${name%%.nds}.sps
	xenobox sdatexpand s "$file" "$SPSDIR$sps"
done
