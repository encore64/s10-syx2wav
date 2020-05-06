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
	char verbose = 1;

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
	
	static dword s10_memory_max = 256*1024;
	byte *s10_memory = malloc(s10_memory_max);

	static byte wave_header[12];
	static byte wave_chunk_fmt[24];
	static byte wave_chunk_smpl[68];
	static byte wave_chunk_data[8];

	fread(syxbuf, 1, fsize, syxfile);
	fclose(syxfile);

	dword SamplePosition;
	byte LoHiToggle;
	int Sample16bit;

	byte char_temp;
	dword SysexCounter = 0;
	byte SysexActive = 0;
	byte CommandId;
	byte ParameterId;
	int WPOffs;	// Wave Parameter Offset
	byte WPBlock; // Wave Parameter Block
	size_t x;

	static byte SamplingStructureMax = 10;
	byte
		SSBankOffset = 0,
		SSLength = 0,
		SSLoops = 0;
	dword Address;

	static char *SamplingStructureLUT[] = {
		"A",
		"B",
		"C",
		"D",
		"AB",
		"CD",
		"ABCD",
		"A/B",
		"C/D",
		"AB/CD",
		"A/B/C/D"
	};

	// Sampling structure array - bank offset, length, loops

	static char SSBankOffsetLengthLoops[][3] = {
		{0, 1, 1},
		{1, 1, 1},
		{2, 1, 1},
		{3, 1, 1},
		{0, 2, 1},
		{2, 2, 1},
		{0, 4, 1},
		{0, 1, 2},
		{2, 1, 2},
		{0, 2, 2},
		{0, 1, 4}
	};

	// Sampler parameters
	char ToneName[4][10]; // 9 characters + null
	byte
		SamplingStructure[4] = {0, 0, 0, 0},
		LoopMode[4] = {0, 0, 0, 0},
		ScanMode[4] = {0, 0, 0, 0};
	dword
		StartAddress[4] = {0, 0, 0, 0},
		ManualLoopLength[4] = {0, 0, 0, 0},
		ManualEndAddress[4] = {0, 0, 0, 0},
		AutoLoopLength[4] = {0, 0, 0, 0},
		AutoEndAddress[4] = {0, 0, 0, 0},
		SampleRate[4] = {30000, 30000, 30000, 30000};

	for (x = 0; x < fsize; x++) {
		char_temp = syxbuf[x];
		//printf("%02hhX ", char_temp);

		if (char_temp == 0xf0) { // system exclusive start
			if (verbose > 1) printf("\nSystem Exclusive start.\n");
			SysexCounter = 0;
			SysexActive = 1;
			continue;
		}

		if (char_temp == 0xf7) { // system exclusive stop
			if (verbose > 1) printf("System Exclusive stop. SysexCounter (minus header and stop) at: %ld\n", SysexCounter-8);
			SysexActive = 0;
			continue;
		}

		if (SysexActive) {
			if (SysexCounter == 0) {
				if (char_temp != 0x41) { // Roland ID ?
					if (verbose > 1) printf("Wrong manufacturer ID.\n");
					SysexActive = 0;
				} else {
					if (verbose > 1) printf("Roland ID found.\n");
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 1) {
				if (char_temp > 0x0f) { // Device ID
					if (verbose > 1) printf("Wrong MIDI basic channel.\n");
					SysexActive = 0;
				} else {
					if (verbose > 1) printf("MIDI basic channel: %d\n", char_temp+1);
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 2) {
				if (char_temp != 0x10) { // S-10 ?
					if (verbose > 1) printf("Wrong Model-ID.\n");
					SysexActive = 0;
				} else {
					if (verbose > 1) printf("S-10 found.\n");
				}
				SysexCounter++;
				continue;
			}

			if (SysexCounter == 3) {
				CommandId = char_temp;

				if (CommandId == 0x11) { // RQ1?
					if (verbose > 1) printf("Command-ID: Request (one way).\n");
				}
				if (CommandId == 0x12) { // DT1?
					if (verbose > 1) printf("Command-ID: Data set (One way).\n");
				}
				if (CommandId == 0x40) { // WSD?
					if (verbose > 1) printf("Command-ID: Want to send data.\n");
				}
				if (CommandId == 0x41) { // RQD?
					if (verbose > 1) printf("Command-ID: Request data.\n");
				}
				if (CommandId == 0x42) { // DAT?
					if (verbose > 1) printf("Command-ID: Data set.\n");
				}
				if (CommandId == 0x43) { // ACK?
					if (verbose > 1) printf("Command-ID: Acknowledge.\n");
				}
				if (CommandId == 0x45) { // EOD?
					if (verbose > 1) printf("Command-ID: End of data.\n");
				}
				if (CommandId == 0x4e) { // ERR?
					if (verbose > 1) printf("Command-ID: Communication error.\n");
				}
				if (CommandId == 0x4f) { // RJC?
					if (verbose > 1) printf("Command-ID: Rejection.\n");
				}

				SysexCounter++;
				continue;
			}

			if (SysexCounter == 4) { // Address
				if (CommandId == 0x12) { // DT1?
					if (verbose > 1) printf("Address: %02hhX %02hhX %02hhX ", syxbuf[x], syxbuf[x+1] ,syxbuf[x+2]);
					Address = (syxbuf[x] << 16) + (syxbuf[x+1] << 8) + syxbuf[x+2];
					ParameterId = 0;
					LoHiToggle = 0;
					WPOffs = 0x00;	// reset Wave Parameter Offset

					if (Address >= 0x00010000 && Address <= 0x00010048) {
						ParameterId = 1; WPBlock = 0; if (verbose) printf("Wave parameter of block-1.\n");
					}
					if (Address >= 0x00010049 && Address <= 0x00010111) {
						ParameterId = 1; WPBlock = 1; if (verbose) printf("Wave parameter of block-2.\n");
					}
					if (Address >= 0x00010112 && Address <= 0x0001015a) {
						ParameterId = 1; WPBlock = 2; if (verbose) printf("Wave parameter of block-3.\n");
					}
					if (Address >= 0x0001015b && Address <= 0x00010224) {
						ParameterId = 1; WPBlock = 3; if (verbose) printf("Wave parameter of block-4.\n");
					}
					if ((syxbuf[x] == 0x01) && (syxbuf[x+1] == 0x08)) {
						ParameterId = 2; if (verbose) printf("Performance parameter.\n");
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
							if (verbose > 1) printf("SamplePosition outside S-10 memory boundary.\n");
							break;
						}
					}

					if ((syxbuf[x] >= 0x02) && (syxbuf[x] <= 0x05)) {
						if (verbose > 1) printf("Wave data of bank-1.\n");
					}
					if ((syxbuf[x] >= 0x06) && (syxbuf[x] <= 0x09)) {
						if (verbose > 1) printf("Wave data of bank-2.\n");
					}
					if ((syxbuf[x] >= 0x0a) && (syxbuf[x] <= 0x0d)) {
						if (verbose > 1) printf("Wave data of bank-3.\n");
					}
					if ((syxbuf[x] >= 0x0e) && (syxbuf[x] <= 0x11)) {
						if (verbose > 1) printf("Wave data of bank-4.\n");
					}
				}
			}

			if (SysexCounter >= 7) {

				// Wave parameter

				if (ParameterId == 1) {

					if (SysexCounter == 7+0x49) {	// When a second wave parameter block is in the same sysex chunk
						WPOffs = 0x49;

						if (verbose > 1) printf("WPOffs is: %d\n", WPOffs);
					}

					if (SysexCounter == 7+WPOffs) {
						WPBlock = syxbuf[x + 0x0a]; // Destination bank (we set this early / first as we rely on it instead of memory address)
						if (verbose) printf("Destination bank: %d\n", WPBlock+1);
					}

					if ((SysexCounter >= 7+WPOffs) && (SysexCounter <= 7+WPOffs+0x08)) { // Tone Name
						ToneName[WPBlock][SysexCounter-7-WPOffs] = syxbuf[x];

						if (SysexCounter == 7+WPOffs+8) {
							ToneName[WPBlock][9] = 0x00; // null

							if (verbose) printf("Tone Name: '%s'\n", ToneName[WPBlock]);
						}
					}

					if (SysexCounter == 7+WPOffs+0x09) { // Sampling structure
						if (syxbuf[x] <= SamplingStructureMax) {
							SamplingStructure[WPBlock] = syxbuf[x];
							SSBankOffset = SSBankOffsetLengthLoops[SamplingStructure[WPBlock]][0];
							SSLength = SSBankOffsetLengthLoops[SamplingStructure[WPBlock]][1];
							SSLoops = SSBankOffsetLengthLoops[SamplingStructure[WPBlock]][2];

							if (verbose) {
								printf("Sampling structure: %d - %s\n", SamplingStructure[WPBlock], SamplingStructureLUT[SamplingStructure[WPBlock]]);
							}
						}
					}

					// (SysexCounter == 7+WPOffs+0x0a) { // Destination bank

					if (SysexCounter == 7+WPOffs+0x0b) { // Sampling rate
						if (syxbuf[x] & 0x01) {
							SampleRate[WPBlock] = 15000;
							if (verbose) printf("Sampling rate: 15 kHz\n");
						} else {
							if (verbose) printf("Sampling rate: 30 kHz\n");
						}
					}

					if (SysexCounter == 7+WPOffs+0x0c) { // Loop mode / Scan Mode
						if ((syxbuf[x] & 0x0c) == 0x00) {
							if (verbose) printf("Loop mode: 1 shot\n");
						}
						if ((syxbuf[x] & 0x0c) == 0x04) {
							LoopMode[WPBlock] = 1;
							if (verbose) printf("Loop mode: Manual\n");
						}
						if ((syxbuf[x] & 0x0c) == 0x08) {
							LoopMode[WPBlock] = 2;
							if (verbose) printf("Loop mode: Auto\n");
						}

						if ((syxbuf[x] & 0x03) == 0x00) {
							if (verbose) printf("Scan mode: Forward\n");
						}
						if ((syxbuf[x] & 0x03) == 0x01) {
							ScanMode[WPBlock] = 1;	// Alternating loop (forward/backward, also known as Ping Pong)
							if (verbose) printf("Scan mode: Alternate\n");
						}
						if ((syxbuf[x] & 0x03) == 0x02) {
							ScanMode[WPBlock] = 2;	// Loop backward (reverse)
							if (verbose) printf("Scan mode: Backward\n");
						}
					}

					if (SysexCounter == 7+WPOffs+0x0d) { // Rec key number
						if (verbose) printf("Rec key number: %d\n", (syxbuf[x] & 0x0f) + ((syxbuf[x+1] & 0x0f) << 4));
					}

					if (SysexCounter == 7+WPOffs+0x11) { // Start address, Manual and auto loop length and end address

						StartAddress[WPBlock] =	// (StartAddress-65536) / 32768 seems to be the same as destination bank
							((syxbuf[x] & 0x0f) << 8) +
							((syxbuf[x+1] & 0x0f) << 12) +
							((syxbuf[x+2] & 0x0f)) +
							((syxbuf[x+3] & 0x0f) << 4) +
							((syxbuf[x+21] & 0x0c) << 14);

						if (StartAddress[WPBlock] > 65535) { // probably always true
							StartAddress[WPBlock] -=65536;
						}

						ManualLoopLength[WPBlock] =
							((syxbuf[x+4] & 0x0f) << 8) +
							((syxbuf[x+5] & 0x0f) << 12) +
							((syxbuf[x+6] & 0x0f)) +
							((syxbuf[x+7] & 0x0f) << 4) +
							((syxbuf[x+20] & 0x0c) << 14);

						ManualLoopLength[WPBlock]--;

						ManualEndAddress[WPBlock] =
							((syxbuf[x+8] & 0x0f) << 8) +
							((syxbuf[x+9] & 0x0f) << 12) +
							((syxbuf[x+10] & 0x0f)) +
							((syxbuf[x+11] & 0x0f) << 4) +
							((syxbuf[x+20] & 0x03) << 16);
						if (ManualEndAddress[WPBlock] > 65535) { // probably always true
							ManualEndAddress[WPBlock] -=65536;
						}

						ManualEndAddress[WPBlock] -= StartAddress[WPBlock];

						AutoLoopLength[WPBlock] =
							((syxbuf[x+12] & 0x0f) << 8) +
							((syxbuf[x+13] & 0x0f) << 12) +
							((syxbuf[x+14] & 0x0f)) +
							((syxbuf[x+15] & 0x0f) << 4) +
							((syxbuf[x+23] & 0x0c) << 14);

						AutoLoopLength[WPBlock]--;

						AutoEndAddress[WPBlock] =
							((syxbuf[x+16] & 0x0f) << 8) +
							((syxbuf[x+17] & 0x0f) << 12) +
							((syxbuf[x+18] & 0x0f)) +
							((syxbuf[x+19] & 0x0f) << 4) +
							((syxbuf[x+23] & 0x03) << 16);
						if (AutoEndAddress[WPBlock] > 65535) { // probably always true
							AutoEndAddress[WPBlock] -=65536;
						}

						AutoEndAddress[WPBlock] -= StartAddress[WPBlock];

						if (verbose) printf("Start Address: %ld\n\n", StartAddress[WPBlock]);
						if (verbose) printf("Manual Loop Length: %ld\n", ManualLoopLength[WPBlock]);
						if (verbose) printf("Manual End Address (minus Start Address): %ld\n\n", ManualEndAddress[WPBlock]);
						if (verbose) printf("Auto Loop Length: %ld\n", AutoLoopLength[WPBlock]);
						if (verbose) printf("Auto End Address (minus Start Address): %ld\n\n", AutoEndAddress[WPBlock]);
					}
				}

				// Wave data

				if (ParameterId == 3) {
					if (LoHiToggle %2 != 0) { // odd

						Sample16bit = ((syxbuf[x-1] & 0x7f) << 9) + ((syxbuf[x] & 0x7c) << 2);
						s10_memory[SamplePosition] = 0xff & Sample16bit;
						s10_memory[SamplePosition+1] = 0xff & (Sample16bit >> 8);	// Significant Bits Per Sample (16)

						SamplePosition+=2;
						if (SamplePosition > s10_memory_max) {
							if (verbose > 1) printf("SamplePosition outside S-10 memory boundary.\n");
							break;
						}
					}
					LoHiToggle++;
				}
			}

			SysexCounter++;
		}
	}

	if (verbose) printf("\nFinal SamplePosition: %ld\n", SamplePosition);

	byte NumChannels = 1;
	byte BitsPerSample = 16;
	dword
		NumSamples,
		total_chunk_size;

	for (x = SSBankOffset; x < (SSLoops+SSBankOffset); x++) {
		NumSamples = (SSLength * (s10_memory_max >> 3));
		printf("Loop: %ld offset: %ld length: %d, %ld\n", x, (SSBankOffset+x), SSLength, NumSamples);

		// description of WAV-format: http://soundfile.sapp.org/doc/WaveFormat/

		total_chunk_size = sizeof(wave_header) + sizeof(wave_chunk_fmt) + (NumSamples * NumChannels * BitsPerSample / 8) + sizeof(wave_chunk_smpl);

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

		wave_chunk_fmt[12] = 255 & SampleRate[x];			// SampleRate
		wave_chunk_fmt[13] = 255 & (SampleRate[x] >> 8);
		wave_chunk_fmt[14] = 255 & (SampleRate[x] >> 16);
		wave_chunk_fmt[15] = 255 & (SampleRate[x] >> 24);

		wave_chunk_fmt[16] = 255 & (SampleRate[x] * NumChannels * BitsPerSample / 8);				// ByteRate
		wave_chunk_fmt[17] = 255 & ((SampleRate[x] * NumChannels * BitsPerSample / 8) >> 8);
		wave_chunk_fmt[18] = 255 & ((SampleRate[x] * NumChannels * BitsPerSample / 8) >> 16);
		wave_chunk_fmt[19] = 255 & ((SampleRate[x] * NumChannels * BitsPerSample / 8) >> 24);

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

		wave_chunk_smpl[48] = ScanMode[x];	// Type - loop forward

		if (LoopMode[x] == 1) {
			wave_chunk_smpl[52] = 255 & ((ManualEndAddress[x] - ManualLoopLength[x]));				// Loop Start
			wave_chunk_smpl[53] = 255 & ((ManualEndAddress[x] - ManualLoopLength[x]) >> 8);
			wave_chunk_smpl[54] = 255 & ((ManualEndAddress[x] - ManualLoopLength[x]) >> 16);
			wave_chunk_smpl[55] = 255 & ((ManualEndAddress[x] - ManualLoopLength[x]) >> 24);

			wave_chunk_smpl[56] = 255 & (ManualEndAddress[x]);					// Loop End
			wave_chunk_smpl[57] = 255 & (ManualEndAddress[x] >> 8);
			wave_chunk_smpl[58] = 255 & (ManualEndAddress[x] >> 16);
			wave_chunk_smpl[59] = 255 & (ManualEndAddress[x] >> 24);
		}

		else if (LoopMode[x] == 2) {
			wave_chunk_smpl[52] = 255 & ((AutoEndAddress[x] - AutoLoopLength[x]));				// Loop Start
			wave_chunk_smpl[53] = 255 & ((AutoEndAddress[x] - AutoLoopLength[x]) >> 8);
			wave_chunk_smpl[54] = 255 & ((AutoEndAddress[x] - AutoLoopLength[x]) >> 16);
			wave_chunk_smpl[55] = 255 & ((AutoEndAddress[x] - AutoLoopLength[x]) >> 24);

			wave_chunk_smpl[56] = 255 & (AutoEndAddress[x]);					// Loop End
			wave_chunk_smpl[57] = 255 & (AutoEndAddress[x] >> 8);
			wave_chunk_smpl[58] = 255 & (AutoEndAddress[x] >> 16);
			wave_chunk_smpl[59] = 255 & (AutoEndAddress[x] >> 24);
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
		fwrite(wave_chunk_data, 1, 8, wavfile);

		fwrite(s10_memory+(StartAddress[x] << 1), 1, (NumSamples << 1), wavfile);
		fwrite(wave_chunk_smpl, 1, sizeof(wave_chunk_smpl), wavfile);

		fclose(wavfile);
	}

	syx2wav(0x0000, argv[1], argv[2]);

	return 0;
}
