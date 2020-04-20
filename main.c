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
	char verbose = 0;

	if (verbose) printf("*** Roland S-10 .syx to .wav conversion ***\n");

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
	dword fsize = ftell(syxfile);
	rewind(syxfile);

	byte *syxbuf = malloc(fsize);			// read syxfile into syxbuf
	byte *wave_chunk_data = malloc(8+fsize);	// make buffer of atleast equal size
	
	static dword s10_memory_max = 256*1024;
	byte *s10_memory = malloc(s10_memory_max);

	static byte wave_header[12];
	static byte wave_chunk_fmt[24];
	static byte wave_chunk_smpl[68];

	fread(syxbuf, 1, fsize, syxfile);
	fclose(syxfile);

	dword SamplePosition;
	int LoHiToggle;
	int Sample16bit;

	byte char_temp;
	int SysexCounter = 0;
	byte SysexActive = 0;
	byte CommandId;
	byte ParameterId;
	size_t x;

	// Sampler parameters
	byte LoopMode = 0, ScanMode = 0;
	dword Address = 0, StartAddress = 0, ManualLoopLength = 0, ManualEndAddress = 0, AutoLoopLength = 0, AutoEndAddress = 0;
	dword SampleRate = 30000;

	for (x = 0; x < fsize; x++) {
		char_temp = syxbuf[x];
		//printf("%02hhX ", char_temp);

		if (char_temp == 0xf0) { // system exclusive start
			if (verbose) printf("\nSystem Exclusive start.\n");
			SysexCounter = 0;
			SysexActive = 1;
			continue;
		}

		if (char_temp == 0xf7) { // system exclusive stop
			if (verbose) printf("System Exclusive stop. SysexCounter (minus header and stop) at: %d\n", SysexCounter-8);
			SysexActive = 0;
			continue;
		}

		if (SysexActive) {
			if (SysexCounter == 0) {
				if (char_temp != 0x41) { // Roland ID ?
					if (verbose) printf("Wrong manufacturer ID.\n");
					SysexActive = 0;
				} else {
					if (verbose) printf("Roland ID found.\n");
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 1) {
				if (char_temp > 0x0f) { // Device ID
					if (verbose) printf("Wrong MIDI basic channel.\n");
					SysexActive = 0;
				} else {
					if (verbose) printf("MIDI basic channel: %d\n", char_temp+1);
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 2) {
				if (char_temp != 0x10) { // S-10 ?
					if (verbose) printf("Wrong Model-ID.\n");
					SysexActive = 0;
				} else {
					if (verbose) printf("S-10 found.\n");
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 3) {
				CommandId = char_temp;

				if (CommandId == 0x11) { // RQ1?
					if (verbose) printf("Command-ID: Request (one way).\n");
				}
				if (CommandId == 0x12) { // DT1?
					if (verbose) printf("Command-ID: Data set (One way).\n");
				}
				if (CommandId == 0x40) { // WSD?
					if (verbose) printf("Command-ID: Want to send data.\n");
				}
				if (CommandId == 0x41) { // RQD?
					if (verbose) printf("Command-ID: Request data.\n");
				}
				if (CommandId == 0x42) { // DAT?
					if (verbose) printf("Command-ID: Data set.\n");
				}
				if (CommandId == 0x43) { // ACK?
					if (verbose) printf("Command-ID: Acknowledge.\n");
				}
				if (CommandId == 0x45) { // EOD?
					if (verbose) printf("Command-ID: End of data.\n");
				}
				if (CommandId == 0x4e) { // ERR?
					if (verbose) printf("Command-ID: Communication error.\n");
				}
				if (CommandId == 0x4f) { // RJC?
					if (verbose) printf("Command-ID: Rejection.\n");
				}

				SysexCounter++;
				continue;
			}

			if (SysexCounter == 4) { // Address
				if (CommandId == 0x12) { // DT1?
					if (verbose) printf("Address: %02hhX %02hhX %02hhX ", syxbuf[x], syxbuf[x+1] ,syxbuf[x+2]);
					Address = (syxbuf[x] << 16) + (syxbuf[x+1] << 8) + syxbuf[x+2];
					ParameterId = 0;
					LoHiToggle = 0;

					if (Address >= 0x00010000 && Address <= 0x00010048) {
						ParameterId = 1;
						if (verbose) printf("Wave parameter of block-1.");
					}
					if (Address >= 0x00010049 && Address <= 0x00010111) {
						ParameterId = 1;
						if (verbose) printf("Wave parameter of block-2.");
					}
					if (Address >= 0x00010112 && Address <= 0x0001015a) {
						ParameterId = 1;
						if (verbose) printf("Wave parameter of block-3.");
					}
					if (Address >= 0x0001015b && Address <= 0x00010224) {
						ParameterId = 1;
						if (verbose) printf("Wave parameter of block-4.");
					}

					if ((syxbuf[x] == 0x01) && (syxbuf[x+1] == 0x08)) {
						ParameterId = 2;
						if (verbose) printf("Performance parameter.");
					}

					/* 0x020000 - 0x117f7f

					0x02-02x11 = 16 *
					0x00-0x7f = 128 *
					0x00-0x7f = 128 = 262144

					*/

					if ((syxbuf[x] >= 0x02) && (syxbuf[x] <= 0x11)) {
						ParameterId = 3;
						SamplePosition =
							((syxbuf[x] - 0x02) << 14) +
							(syxbuf[x+1] << 7) +
							syxbuf[x+2];

						if (SamplePosition > s10_memory_max) {
							if (verbose) printf("SamplePosition outside S-10 memory boundary.\n");
							break;
						}
					}

					if ((syxbuf[x] >= 0x02) && (syxbuf[x] <= 0x05)) {
						if (verbose) printf("Wave data of bank-1.");
					}
					if ((syxbuf[x] >= 0x06) && (syxbuf[x] <= 0x09)) {
						if (verbose) printf("Wave data of bank-2.");
					}
					if ((syxbuf[x] >= 0x0a) && (syxbuf[x] <= 0x0d)) {
						if (verbose) printf("Wave data of bank-3.");
					}
					if ((syxbuf[x] >= 0x0e) && (syxbuf[x] <= 0x11)) {
						if (verbose) printf("Wave data of bank-4.");
					}
					if (verbose) printf("\n");
				}
			}

			if (SysexCounter >= 7) {
				if (ParameterId == 1) { // Wave parameter
					if ((SysexCounter >= 7) && (SysexCounter <= 7+0x08)) { // Tone Name
						if (verbose) {
							if (SysexCounter == 7) printf("Tone Name: ");
							printf("%c", syxbuf[x]);
							if (SysexCounter == 7+8) printf("\n");
						}
					}

					if (SysexCounter == 7+0x09) { // Sampling structure
						if (verbose) printf("Sampling structure: %d\n", syxbuf[x]);
					}

					if (SysexCounter == 7+0x0a) { // Destination bank
						if (verbose) printf("Destination bank: %d\n", syxbuf[x]);
					}

					if (SysexCounter == 7+0x0b) { // Sampling rate
						if (syxbuf[x] & 0x01) {
							SampleRate = 15000;
							if (verbose) printf("Sampling rate: 15 kHz\n");
						} else {
							if (verbose) printf("Sampling rate: 30 kHz\n");
						}
					}

					if (SysexCounter == 7+0x0c) { // Loop mode / Scan Mode
						if ((syxbuf[x] & 0x0c) == 0x00) {
							if (verbose) printf("Loop mode: 1 shot\n");
						}
						if ((syxbuf[x] & 0x0c) == 0x04) {
							LoopMode = 1;
							if (verbose) printf("Loop mode: Manual\n");
						}
						if ((syxbuf[x] & 0x0c) == 0x08) {
							LoopMode = 2;
							if (verbose) printf("Loop mode: Auto\n");
						}

						if ((syxbuf[x] & 0x03) == 0x00) {
							if (verbose) printf("Scan mode: Forward\n");
						}
						if ((syxbuf[x] & 0x03) == 0x01) {
							ScanMode = 1;	// Alternating loop (forward/backward, also known as Ping Pong)
							if (verbose) printf("Scan mode: Alternate\n");
						}
						if ((syxbuf[x] & 0x03) == 0x02) {
							ScanMode = 2;	// Loop backward (reverse)
							if (verbose) printf("Scan mode: Backward\n");
						}
					}

					if (SysexCounter == 7+0x0d) { // Rec key number
						if (verbose) printf("Rec key number: %d\n", (syxbuf[x] & 0x0f) + ((syxbuf[x+1] & 0x0f) << 4));
					}

					if (SysexCounter == 7+0x11) { // Start address, Manual and auto loop length and end address

						StartAddress =	// (StartAddress-65536) / 32768 seems to be the same as destination bank
							((syxbuf[x] & 0x0f) << 8) +
							((syxbuf[x+1] & 0x0f) << 12) +
							((syxbuf[x+2] & 0x0f)) +
							((syxbuf[x+3] & 0x0f) << 4) +
							((syxbuf[x+21] & 0x0c) << 14);

						ManualLoopLength =
							((syxbuf[x+4] & 0x0f) << 8) +
							((syxbuf[x+5] & 0x0f) << 12) +
							((syxbuf[x+6] & 0x0f)) +
							((syxbuf[x+7] & 0x0f) << 4) +
							((syxbuf[x+20] & 0x0c) << 14);

						ManualEndAddress =
							((syxbuf[x+8] & 0x0f) << 8) +
							((syxbuf[x+9] & 0x0f) << 12) +
							((syxbuf[x+10] & 0x0f)) +
							((syxbuf[x+11] & 0x0f) << 4) +
							((syxbuf[x+20] & 0x03) << 16);

						AutoLoopLength =
							((syxbuf[x+12] & 0x0f) << 8) +
							((syxbuf[x+13] & 0x0f) << 12) +
							((syxbuf[x+14] & 0x0f)) +
							((syxbuf[x+15] & 0x0f) << 4) +
							((syxbuf[x+23] & 0x0c) << 14);

						AutoEndAddress =
							((syxbuf[x+16] & 0x0f) << 8) +
							((syxbuf[x+17] & 0x0f) << 12) +
							((syxbuf[x+18] & 0x0f)) +
							((syxbuf[x+19] & 0x0f) << 4) +
							((syxbuf[x+23] & 0x03) << 16);

						if (verbose) printf("Start address: %ld\n\n", StartAddress);
						if (verbose) printf("Manual Loop Length: %ld\n", ManualLoopLength);
						if (verbose) printf("Manual End Address: %ld\n\n", ManualEndAddress);
						if (verbose) printf("Auto Loop Length: %ld\n", AutoLoopLength);
						if (verbose) printf("Auto End Address: %ld\n\n", AutoEndAddress);
					}
				}

				if (ParameterId == 3) { // Wave data
					if (LoHiToggle %2 != 0) { // odd

						Sample16bit = ((syxbuf[x-1] & 0x7f) << 9) + ((syxbuf[x] & 0x7c) << 2);
						wave_chunk_data[8+SamplePosition] = 0xff & Sample16bit;
						wave_chunk_data[8+SamplePosition+1] = 0xff & (Sample16bit >> 8);	// Significant Bits Per Sample (16)

						SamplePosition+=2;
						if (SamplePosition > s10_memory_max) {
							if (verbose) printf("SamplePosition outside S-10 memory boundary.\n");
							break;
						}
					}
					LoHiToggle++;
				}
			}

			SysexCounter++;
		}
	}

	if (verbose) printf("SamplePosition: %ld\n", SamplePosition);

	byte NumChannels = 1;
	byte BitsPerSample = 16;
	dword NumSamples = (SamplePosition >> 1);

	// description of WAV-format: http://soundfile.sapp.org/doc/WaveFormat/

	dword total_chunk_size = sizeof(wave_header) + sizeof(wave_chunk_fmt) + (NumSamples * NumChannels * BitsPerSample / 8) + sizeof(wave_chunk_smpl);

	// header

	wave_header[0] = 0x52;	// "RIFF"
	wave_header[1] = 0x49;
	wave_header[2] = 0x46;
	wave_header[3] = 0x46;

	wave_header[4] = 255 & ((total_chunk_size - 8));				// Total Chunk Size
	wave_header[5] = 255 & ((total_chunk_size - 8) >> 8);
	wave_header[6] = 255 & ((total_chunk_size - 8) >> 16);
	wave_header[7] = 255 & ((total_chunk_size - 8) >> 24);

	wave_header[8] = 0x57;	// "WAVE"
	wave_header[9] = 0x41;
	wave_header[10] = 0x56;
	wave_header[11] = 0x45;

	// fmt

	wave_chunk_fmt[0] = 0x66;	// SubChunk 1 ID ("fmt ")
	wave_chunk_fmt[1] = 0x6d;
	wave_chunk_fmt[2] = 0x74;
	wave_chunk_fmt[3] = 0x20;

	wave_chunk_fmt[4] = 255 & ((sizeof(wave_chunk_fmt) - 8));			// Chunk Data Size, 16 for PCM
	wave_chunk_fmt[5] = 255 & ((sizeof(wave_chunk_fmt) - 8) >> 8);
	wave_chunk_fmt[6] = 255 & ((sizeof(wave_chunk_fmt) - 8) >> 16);
	wave_chunk_fmt[7] = 255 & ((sizeof(wave_chunk_fmt) - 8) >> 24);

	wave_chunk_fmt[8] = 1;	// Compression code, 1 = PCM / Linear quantization

	wave_chunk_fmt[10] = 255 & NumChannels; // Mono = 1, Stereo = 2, etc
	wave_chunk_fmt[11] = 255 & (NumChannels >> 8);

	wave_chunk_fmt[12] = 255 & SampleRate;			// SampleRate
	wave_chunk_fmt[13] = 255 & (SampleRate >> 8);
	wave_chunk_fmt[14] = 255 & (SampleRate >> 16);
	wave_chunk_fmt[15] = 255 & (SampleRate >> 24);

	wave_chunk_fmt[16] = 255 & (SampleRate * NumChannels * BitsPerSample / 8);				// ByteRate
	wave_chunk_fmt[17] = 255 & ((SampleRate * NumChannels * BitsPerSample / 8) >> 8);
	wave_chunk_fmt[18] = 255 & ((SampleRate * NumChannels * BitsPerSample / 8) >> 16);
	wave_chunk_fmt[19] = 255 & ((SampleRate * NumChannels * BitsPerSample / 8) >> 24);

	wave_chunk_fmt[20] = 255 & (NumChannels * BitsPerSample / 8);				// BlockAlign
	wave_chunk_fmt[21] = 255 & ((NumChannels * BitsPerSample / 8) >> 8);

	wave_chunk_fmt[22] = 255 & BitsPerSample; // Significant Bits Per Sample (16)
	wave_chunk_fmt[23] = 255 & (BitsPerSample >> 8);

	// data

	wave_chunk_data[0] = 0x64;	// SubChunk 2 ID ("data")
	wave_chunk_data[1] = 0x61;
	wave_chunk_data[2] = 0x74;
	wave_chunk_data[3] = 0x61;

	wave_chunk_data[4] = 255 & (NumSamples * NumChannels * BitsPerSample / 8);				// Chunk Data Size
	wave_chunk_data[5] = 255 & ((NumSamples * NumChannels * BitsPerSample / 8) >> 8);
	wave_chunk_data[6] = 255 & ((NumSamples * NumChannels * BitsPerSample / 8) >> 16);
	wave_chunk_data[7] = 255 & ((NumSamples * NumChannels * BitsPerSample / 8) >> 24);

	// smpl (https://sites.google.com/site/musicgapi/technical-documents/wav-file-format#smpl)

	wave_chunk_smpl[0] = 0x73;	// "smpl"
	wave_chunk_smpl[1] = 0x6d;
	wave_chunk_smpl[2] = 0x70;
	wave_chunk_smpl[3] = 0x6c;

	wave_chunk_smpl[4] = 255 & ((sizeof(wave_chunk_smpl) - 8));				// Chunk Data Size
	wave_chunk_smpl[5] = 255 & ((sizeof(wave_chunk_smpl) - 8) >> 8);
	wave_chunk_smpl[6] = 255 & ((sizeof(wave_chunk_smpl) - 8) >> 16);
	wave_chunk_smpl[7] = 255 & ((sizeof(wave_chunk_smpl) - 8) >> 24);

	wave_chunk_smpl[8] = 0;	// Manufacturer ID

	wave_chunk_smpl[12] = 0;	// Product ID

	wave_chunk_smpl[16] = 0;	// Sample period

	wave_chunk_smpl[20] = 60; 	// MIDI Unity Note

	wave_chunk_smpl[24] = 0; 	// MIDI Pitch Fraction

	wave_chunk_smpl[28] = 0;	// SMPTE Format

	wave_chunk_smpl[32] = 0;	// SMPTE Offset

	wave_chunk_smpl[36] = 1;	// Sample Loops

	wave_chunk_smpl[40] = 0;	// Sampler Data size

	// sample loop 0

	wave_chunk_smpl[44] = 0;	// Cue Point ID

	wave_chunk_smpl[48] = ScanMode;	// Type - loop forward

	if (LoopMode == 1) {
		wave_chunk_smpl[52] = 255 & ((ManualEndAddress - ManualLoopLength));				// Loop Start
		wave_chunk_smpl[53] = 255 & ((ManualEndAddress - ManualLoopLength) >> 8);
		wave_chunk_smpl[54] = 255 & ((ManualEndAddress - ManualLoopLength) >> 16);
		wave_chunk_smpl[55] = 255 & ((ManualEndAddress - ManualLoopLength) >> 24);

		wave_chunk_smpl[56] = 255 & (ManualEndAddress);					// Loop End
		wave_chunk_smpl[57] = 255 & (ManualEndAddress >> 8);
		wave_chunk_smpl[58] = 255 & (ManualEndAddress >> 16);
		wave_chunk_smpl[59] = 255 & (ManualEndAddress >> 24);
	}

	else if (LoopMode == 2) {
		wave_chunk_smpl[52] = 255 & ((AutoEndAddress - AutoLoopLength));				// Loop Start
		wave_chunk_smpl[53] = 255 & ((AutoEndAddress - AutoLoopLength) >> 8);
		wave_chunk_smpl[54] = 255 & ((AutoEndAddress - AutoLoopLength) >> 16);
		wave_chunk_smpl[55] = 255 & ((AutoEndAddress - AutoLoopLength) >> 24);

		wave_chunk_smpl[56] = 255 & (AutoEndAddress);					// Loop End
		wave_chunk_smpl[57] = 255 & (AutoEndAddress >> 8);
		wave_chunk_smpl[58] = 255 & (AutoEndAddress >> 16);
		wave_chunk_smpl[59] = 255 & (AutoEndAddress >> 24);
	}

	wave_chunk_smpl[60] = 0;		// Fraction

	wave_chunk_smpl[64] = 0;		// Play Count

	// 68 bytes in total

	if (!(wavfile = fopen(argv[2], "wb"))) {
	    printf("Error opening file: %s\n", strerror(errno));
	    return(1); // returning non-zero exits the program as failed
	}

	fwrite(wave_header, 1, sizeof(wave_header), wavfile);
	fwrite(wave_chunk_fmt, 1, sizeof(wave_chunk_fmt), wavfile);
	fwrite(wave_chunk_data, 1, 8+SamplePosition, wavfile);
	fwrite(wave_chunk_smpl, 1, sizeof(wave_chunk_smpl), wavfile);

	fclose(wavfile);

	syx2wav(0x0000, argv[1], argv[2]);

	return 0;
}
