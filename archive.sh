rm -f xenobox.7z xenobox_DSEM.7z
7z a -r0 -mx=9 -xr0!*/.svn/* -xr0!*/.svn xenobox.7z *
7z a -mx=9 xenobox_DSEM.7z xenobox.exe GameList.txt

