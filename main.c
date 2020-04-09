#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//--Typedefs--------------------------------------------------------------------
typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long dword;

// -----------------------------------------------------------------------------

void syx2wav (dword file_position, char const load_file1[50], char const save_file[50])
{
	return;
}

int main(int argc, char *argv[]) {
	printf("Roland S-10 .syx to .wav conversion\n");

	if (argc < 3) {
		printf("\nError: Too few arguments.\nSyntax should be: s10-syx2wav input.syx output.wav\n");
		return 1;
	}

	syx2wav(0x0000, argv[1], argv[2]);

	return 0;
}
