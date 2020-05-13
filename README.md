# Roland S-10 .syx to .wav conversion

Takes a sysex-file intended for the Roland S-10 (12 bit) sampler, analyzes the sysex data and writes one or more wav-files.

`$ s10-syx2wav input.syx`

The wav-filename will be based on 1) the input file, 2) the sampling structure used, 3) what wave parameter and 4) the tone name. For example, converting the file `input.syx` may generate `input - A-B (A) ToneName.wav`.

The resulting wav-file(s) will be either 15 or 30 kHz 16 bit mono and includes the loop information.

## Batch converting

The program only converts one sysex-file at the time. In Linux or Windows you can create the following file and put it together with the compiled executable in order to convert a directory of .syx-files (plus all its subdirectories) to .wav.

### Linux

**batch-convert.sh**
```bash
#!/bin/bash
find "./" -type f -name "*.syx" | while read fname; do
  ./s10-syx2wav "$fname"
done
```

### Windows

**batch-convert.bat**
```bat
for /r %%i in (*.syx) do s10-syx2wav.exe "%%i"
```
