cd zlib
./configure --static
cd ..
make -C zlib
mkdir -p libs/include
cp zlib/libz.a libs/libz.a
cp zlib/*.h libs/include/
make
