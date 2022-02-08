unlzop - LZOP decoder under public domain

This is yet another LZOP decoder under public domain.
So you can use this even in proprietary softwares.

History
V.01.090106 initial version
Ignores checksum/mode/time.
Supports lzo/zlib method.
As for file, only stdio is supported.
V.02.090107
lzo.c is separated as liblzod.so to clarify license.
#Possibly we should not link liblzod.so statically, but maybe OK, because lzopd is a test application.
V.03.090108
I should allocate more 1 byte memory for decompression...
V.04.090108
Fix: Could not decode zlib correctly.
Now Adler32/CRC32 is processed correctly.
V.05.090203
Finally original lzo1x/y decompression implementation. Public domain.

110421: rewritten lzo1xy_decompress() using memstream API.
