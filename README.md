# Roland S-10 .syx to .wav conversion

Takes a sysex-file intended for the Roland S-10 (12 bit) sampler, analyzes the sysex data and writes one or more wav-files.

`$ s10-syx2wav input.syx`

The wav-filename will be based on 1) the input file, 2) the sampling structure used, 3) what wave parameter and 4) the tone name. For example, converting the file `input.syx` may generate `input - A-B (A) ToneName.wav`.

The resulting wav-file(s) will be either 15 or 30 kHz 16 bit mono and includes the loop information.

It only converts one sysex-file at the time. The `compile_and_run.sh` bash-script demonstrates how to convert a directory (plus all its subdirectories) in Linux.
