#!/bin/bash

# remove previous build
rm -f ./build/s10-syx2wav

# compile program for linux
gcc main.c -o ./build/s10-syx2wav

# example for compiling for windows (32 and 64 bit compatible)
# i686-w64-mingw32-gcc main.c -o ./build/s10-syx2wav.exe

# run program with example file
find "./example-files/" -type f -name "*.syx" | while read fname; do
	echo "processing: $fname"
	./build/s10-syx2wav "$fname"
done
