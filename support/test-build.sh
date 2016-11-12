#! /bin/sh -e
autoreconf -fvi
rm -rf *.tar.gz
./configure
make dist
rm -rf _build
mkdir _build
cd _build
tar zxvf ../*.tar.gz
cd recoverjpeg*
mkdir _build
cd _build
../configure
make
make check
