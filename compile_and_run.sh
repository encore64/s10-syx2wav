#!/bin/bash

# remove previous build
rm -f ./build/s10-syx2wav

# compile program
gcc main.c -o ./build/s10-syx2wav

# run program with example file

# ./build/s10-syx2wav "./example-files/L-100 001 B Drum Set (SD).syx" "./example-files/L-100 001 B Drum Set (SD).wav"
find "./example-files/" -type f -name "*.syx" | while read fname; do
	echo "processing: $fname"
	./build/s10-syx2wav "$fname" "$fname.wav"
done

