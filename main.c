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
	printf("*** Roland S-10 .syx to .wav conversion ***\n");

	if (argc < 3) {
		printf("\nError: Too few arguments.\nSyntax should be: s10-syx2wav input.syx output.wav\n");
		return 1;
	}

	FILE *syxfile, *wavfile;

	if (!(syxfile = fopen(argv[1], "rb"))) {
	    printf("Error opening file: %s\n", strerror(errno));
	    return(1); // returning non-zero exits the program as failed
	}

	fseek(syxfile, 0, SEEK_END);			// check length of file
	long fsize = ftell(syxfile);
	rewind(syxfile);

	byte *syxbuf = malloc(fsize+1);			// read syxfile into syxbuf
	fread(syxbuf, 1, fsize, syxfile);
	fclose(syxfile);
	syxbuf[fsize] = 0;

	byte char_temp;
	int sysex_segments = 0;
	size_t x;

	/*
	for (x = 0; x < fsize; x++) {
		char_temp = syxbuf[x];
		//printf("%02hhX ", char_temp);

		if (char_temp == 0xf0) {
			sysex_segments++;
		}
	}

	printf("\nNumber of sysex segments: %d\n", sysex_segments);
	*/

	long SampleRate = 30000;
	byte NumChannels = 1;
	byte BitsPerSample = 16;
	long NumSamples = (fsize >> 2);

	static unsigned char wave_header[44];

	// description of WAV-format: http://soundfile.sapp.org/doc/WaveFormat/

	wave_header[0] = 0x52;	// RIFF
	wave_header[1] = 0x49;
	wave_header[2] = 0x46;
	wave_header[3] = 0x46;

	wave_header[4] = 255 & ((SampleRate * NumChannels * BitsPerSample / 8) + 36);				// ChunkSize
	wave_header[5] = 255 & (((SampleRate * NumChannels * BitsPerSample / 8) + 36) >> 8);
	wave_header[6] = 255 & (((SampleRate * NumChannels * BitsPerSample / 8) + 36) >> 16);
	wave_header[7] = 255 & (((SampleRate * NumChannels * BitsPerSample / 8) + 36) >> 24);

	wave_header[8] = 0x57;	// WAVE
	wave_header[9] = 0x41;
	wave_header[10] = 0x56;
	wave_header[11] = 0x45;

	wave_header[12] = 0x66;	// SubChunk1ID ("fmt ")
	wave_header[13] = 0x6d;
	wave_header[14] = 0x74;
	wave_header[15] = 0x20;

	wave_header[16] = 0x10; // Subchunk1Size, 16 for PCM

	wave_header[20] = 0x01; // 1 = PCM / Linear quantization

	wave_header[22] = 255 & NumChannels; // Mono = 1, Stereo = 2, etc

	wave_header[24] = 255 & SampleRate;			// SampleRate
	wave_header[25] = 255 & (SampleRate >> 8);
	wave_header[26] = 255 & (SampleRate >> 16);
	wave_header[27] = 255 & (SampleRate >> 24);

	wave_header[28] = 255 & (SampleRate * NumChannels * BitsPerSample / 8);				// ByteRate
	wave_header[29] = 255 & ((SampleRate * NumChannels * BitsPerSample / 8) >> 8);
	wave_header[30] = 255 & ((SampleRate * NumChannels * BitsPerSample / 8) >> 16);
	wave_header[31] = 255 & ((SampleRate * NumChannels * BitsPerSample / 8) >> 24);

	wave_header[32] = 255 & (NumChannels * BitsPerSample / 8);				// BlockAlign
	wave_header[33] = 255 & ((NumChannels * BitsPerSample / 8) >> 8);

	wave_header[34] = BitsPerSample; // BitsPerSample (16)

	wave_header[36] = 0x64;	// SubChunk2ID ("data")
	wave_header[37] = 0x61;
	wave_header[38] = 0x74;
	wave_header[39] = 0x61;

	wave_header[40] = 255 & (NumSamples * NumChannels * BitsPerSample / 8);				// Subchunk2Size
	wave_header[41] = 255 & ((NumSamples * NumChannels * BitsPerSample / 8) >> 8);
	wave_header[42] = 255 & ((NumSamples * NumChannels * BitsPerSample / 8) >> 16);
	wave_header[43] = 255 & ((NumSamples * NumChannels * BitsPerSample / 8) >> 24);

	if (!(wavfile = fopen(argv[2], "wb"))) {
	    printf("Error opening file: %s\n", strerror(errno));
	    return(1); // returning non-zero exits the program as failed
	}

	fwrite(wave_header, 1, sizeof(wave_header), wavfile);

	for (x = 0; x < fsize; x++) {
		char_temp = (syxbuf[x]);
		fwrite(&char_temp, 1, sizeof(char_temp), wavfile);
	}

	fclose(wavfile);

	syx2wav(0x0000, argv[1], argv[2]);

	return 0;
}
