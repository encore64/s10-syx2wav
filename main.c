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

	byte *syxbuf = malloc(fsize);			// read syxfile into syxbuf
	byte *wavbuf = malloc(fsize);			// make wavbuffer of atleast equal size
	fread(syxbuf, 1, fsize, syxfile);
	fclose(syxfile);

	int SampleCounter = 0;
	int LoHiToggle;
	int Sample16bit;

	byte char_temp;
	int SysexCounter = 0;
	int SysexActive = 0;
	byte CommandId;
	byte ParameterId;
	size_t x;

	for (x = 0; x < fsize; x++) {
		char_temp = syxbuf[x];
		//printf("%02hhX ", char_temp);

		if (char_temp == 0xf0) { // system exclusive start
			printf("\nSystem Exclusive start.\n");
			SysexCounter = 0;
			SysexActive = 1;
			continue;
		}
		if (SysexActive) {
			if (SysexCounter == 0) {
				if (char_temp != 0x41) { // Roland ID ?
					printf("Wrong manufacturer ID.\n");
					SysexActive = 0;
				} else {
					printf("Roland ID found.\n");
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 1) {
				if (char_temp > 0x0f) { // Device ID
					printf("Wrong MIDI basic channel.\n");
					SysexActive = 0;
				} else {
					printf("MIDI basic channel: %d\n", char_temp+1);
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 2) {
				if (char_temp != 0x10) { // S-10 ?
					printf("Wrong Model-ID.\n");
					SysexActive = 0;
				} else {
					printf("S-10 found.\n");
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 3) {
				CommandId = char_temp;

				if (CommandId == 0x11) { // RQ1?
					printf("Command-ID: Request (one way).\n");
				}
				if (CommandId == 0x12) { // DT1?
					printf("Command-ID: Data set (One way).\n");
				}
				if (CommandId == 0x40) { // WSD?
					printf("Command-ID: Want to send data.\n");
				}
				if (CommandId == 0x41) { // RQD?
					printf("Command-ID: Request data.\n");
				}
				if (CommandId == 0x42) { // DAT?
					printf("Command-ID: Data set.\n");
				}
				if (CommandId == 0x43) { // ACK?
					printf("Command-ID: Acknowledge.\n");
				}
				if (CommandId == 0x45) { // EOD?
					printf("Command-ID: End of data.\n");
				}
				if (CommandId == 0x4e) { // ERR?
					printf("Command-ID: Communication error.\n");
				}
				if (CommandId == 0x4f) { // RJC?
					printf("Command-ID: Rejection.\n");
				}

				SysexCounter++;
				continue;
			}

			if (SysexCounter == 4) { // Address
				if (CommandId == 0x12) { // DT1?
					printf("Address: %02hhX %02hhX %02hhX ", syxbuf[x], syxbuf[x+1] ,syxbuf[x+2]);
					ParameterId = 0;
					LoHiToggle = 0;

					if ((syxbuf[x] == 0x01) && (((syxbuf[x+1] == 0x00) && (syxbuf[x+2] >= 0x00)) || ((syxbuf[x+1] == 0x00) && (syxbuf[x+2] >= 0x48)))) {
						ParameterId = 1;
						printf("Wave parameter of block-1.");
					}
					if ((syxbuf[x] == 0x01) && (((syxbuf[x+1] == 0x00) && (syxbuf[x+2] >= 0x49)) || ((syxbuf[x+1] == 0x01) && (syxbuf[x+2] >= 0x11)))) {
						ParameterId = 1;
						printf("Wave parameter of block-2.");
					}
					if ((syxbuf[x] == 0x01) && (((syxbuf[x+1] == 0x01) && (syxbuf[x+2] >= 0x12)) || ((syxbuf[x+1] == 0x01) && (syxbuf[x+2] >= 0x5a)))) {
						ParameterId = 1;
						printf("Wave parameter of block-3.");
					}
					if ((syxbuf[x] == 0x01) && (((syxbuf[x+1] == 0x01) && (syxbuf[x+2] >= 0x5b)) || ((syxbuf[x+1] == 0x02) && (syxbuf[x+2] >= 0x24)))) {
						ParameterId = 1;
						printf("Wave parameter of block-4.");
					}

					if ((syxbuf[x] == 0x01) && (syxbuf[x+1] == 0x08)) {
						ParameterId = 2;
						printf("Performance parameter.");
					}

					if ((syxbuf[x] >= 0x02) && (syxbuf[x] <= 0x05)) {
						ParameterId = 3;
						printf("Wave data of bank-1.");
					}
					if ((syxbuf[x] >= 0x06) && (syxbuf[x] <= 0x09)) {
						ParameterId = 3;
						printf("Wave data of bank-2.");
					}
					if ((syxbuf[x] >= 0x0a) && (syxbuf[x] <= 0x0d)) {
						ParameterId = 3;
						printf("Wave data of bank-3.");
					}
					if ((syxbuf[x] >= 0x0e) && (syxbuf[x] <= 0x12)) {
						ParameterId = 3;
						printf("Wave data of bank-4.");
					}
					printf("\n");
				}
			}

			if ((SysexCounter >= 7) && (ParameterId == 3) && (char_temp != 0xf7)) { // Wave data and no sysex stop
				if (LoHiToggle %2 != 0) { // odd

					Sample16bit = ((syxbuf[x-1] & 0x7f) << 9) + ((syxbuf[x] & 0x7c) << 2);
					wavbuf[SampleCounter] = 0xff & Sample16bit;
					wavbuf[SampleCounter+1] = 0xff & (Sample16bit >> 8);

					SampleCounter+=2;
				}
				LoHiToggle++;
			}

			SysexCounter++;
		}

		if (char_temp == 0xf7) { // system exclusive stop
			printf("System Exclusive stop. SysexCounter at: %d\n", SysexCounter);
			SysexActive = 0;
			continue;
		}
	}

	printf("SampleCounter: %d\n", SampleCounter);

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
	fwrite(wavbuf, 1, SampleCounter, wavfile);

	fclose(wavfile);

	syx2wav(0x0000, argv[1], argv[2]);

	return 0;
}
