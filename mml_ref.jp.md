MML reference for ctrmml (2020-12-10)
========================

## メタコマンド
\#や@で始まる行はメタコマンドとして扱われます。
`#`で始まるメタコマンドは文字列が,`@`で始まるメタコマンドはスペース区切り、またはカンマ区切りのパラメータが入ります。
文字列はダブルクオーテーションで囲うことができます。

-	`#title`, `#composer`, `#author`, `#date`, `#comment` - 曲のメタデータ
-	`#platform` - どのプラットフォームのMMLかの選択。
	- **Note**: 今は `megadrive` または `mdsdrv`しか選択できません。
-	`@<num>` - 音色の定義。プラットフォームに依存する。
-	`@E<num>` - エンベロープの定義。
-	`@M<num>` - ピッチエンベロープの定義。
-	`@P<num>` - パンエンベロープの定義。

## トラックの定義
行の先頭に文字を配置することでトラックを選択できます。
行の先頭にスペースを配置することはできません。

トラックの選択はどの順番でもできます。
これは条件分岐`{/}`に影響を与えます。

### Macros
`*<num>` can be used to specify a track by its number. Since tracks 32 and
above are unlikely to be used by sound channels, you can use them as macros
with the `*` command.

This is not a substitution macro but rather more like a subroutine.
Each track has its own octave and length counter.

### Example:
-	`A` - use track A
-	`ABC` - use tracks A, B, C
-	`ACB` - use tracks in a different order
-	`*0` - use track number 0.

## Adding notes and commands
After specifying channels and adding spaces, you can enter MML commands. It is
also possible to write a new line without specifying tracks, in that case
the previously specified channels will be used.

### Example:
	A       cdefgab
	        bagfedc ; both use track A

## Command reference

### Basic commands
-	`o<0..7>` - Set octave.
-	`>`, `<` - Octave up and down, respectively.
-	`cdefgabh` - Insert notes. Optionally set duration after each note. If no
	duration is set, the duration set by the `l` command is used.
	- Durations are calculated by dividing the value with the length of a
	measure. Optionally a dot `.` can be specified to extend the duration by
	half. Specify duration in number of frames with the `:` prefix.
	- `h` has the same value as `b` in normal mode.
	- `+`, `-`, `=` specify accidentals. `+` adds a sharp, `-` adds a flat,
	`=` adds a natural. Using accidentals will override the key signature.
-	`^` - Tie. Extends duration of previous note.
-	`&` - Slur. Used to connect two notes (legato).
-	`r` - Rest. Optionally set duration after the rest.
-	`Q<1..8>` - Quantize. Used to set articulation. Note length is param/8.
-	`q<1..8>` - Set early release. Used to set articulation. Early release
	cannot exceed note length, in that case it will be note length - 1.
-	`l<duration>` - Set default duration, used if not specified by notes,
	rests, `R` or `~` commands.
-	`R<duration>` - Reverse rest. This subtracts the value from the previous
	note or rest.
	- This can be used to bring back tracks in sync after a delayed echo
	track.
	- If unable (such as if the previous note was at the end of a
	loop, a warning is issued.
-	`~<note><duration>` - Grace note. This subtracts from the length of the
	previous note like the `R` command, then adds a note with the duration of
	the borrow.
-	`[/]<0..255>` - Loop block. The section after the `/` is skipped at the
	last iteration.
-	`L` - Set loop point (segno). If this is present, playback resumes at this
	point when the end of the track is reached.
-	`*<0..255>` - Play commands from another track before resuming at the
	current position.
-	`{/}` - Conditional block. Using this block you can specify commands to
	run on each	individual track, when multiple tracks are specified. Example:
	`ABC {c/d+/g} {d/f/a}`
-	`;` - Comment. The rest of the line is skipped.
-	`|` - Divider. This has no effect and can be used to divide measures or
	bars, for aesthetic reasons.

### Platform commands
These commands are defined for all platforms, however how they are handled
depends on the platform. They may be ignored or the accepted range may differ.

-	`@<0..65535>` - Set instrument.
-	`v<0..15>` - Set volume.
-	`)`, `(` - Volume up and down, respectively. You may specify the increment
	or decrement, if you want.
-	`V<0..255>` - Set volume (fine).
-	`V<-128..+127>` - Modify volume (fine).
-	`p<-128..127>` - Set panning.
-	`_<-128..127>` - Set transpose.
-	`__<-128..127>` - Set relative transpose.
-	`_{<data>}` - Set key signature.
	For details, see [Using key signatures](#using-key-signatures)
-	`k<-128..127>` - Set transpose. The default behavior is the same as the
	`_` command, but in the future there will be an option to change the
	behavior of this command.
-	`K<-128..127>` - Set detune.
-	`E<0..255>` - Set envelope. 0 to disable.
-	`M<0..255>` - Set pitch envelope. 0 to disable.
-	`P<0..255>` - Set pan envelope. 0 to disable.
-	`G<0..255>` - Set portamento. 0 to disable.
-	`D<0..255>` - Drum mode. If set to a non-zero value, enable drum mode and
	set the drum mode index to the parameter value.
	- This changes the behavior of notes. `abcdefgh` represent 0-7 and is
	added to the drum mode index. The macro track with this number is then
	executed up to and including the first note encountered.
	- The duration specified in the macro track is ignored and the calling
	track is used instead.
-	`t<0..255>` - Set tempo in BPM.
-	`T<0..255>` - Set tempo using the platform's native timer values.
-	``'<key> <value> ...'`` - Platform-specific commands.

#### Using key signatures
The `_{}` command can be used to set or modify a key signature. It's possible
to specify a scale directly, or a list of notes to be made sharp, flat or
natural.

The default key signature is always C major or A minor (no sharp or flat notes)

After a key signature has been set, when a note is inserted, if no accidental
is also specified, the sharps/flats as defined in the key signature will be
used.

##### Specifying a key signature
These scales are recognized. Upper case indicates a major scale, and lower case
indicates a minor key. When a scale is specified, the existing key signature
is cancelled.

-	Sharp key signatures: `C`, `a`, `G`, `e`, `D`, `b`, `A`, `f+`, `E`, `c+`,
	`B`, `g+`, `F+`, `d+`, `C+`, `a+`
-	Flat key signatures: `C`, `a`, `F`, `d`, `B-`, `g`, `E-`, `c`, `A-`, `f`,
	`D-`, `b-` `G-`, `e-`, `C-`, `a-`

Example:
-	 `_{c}`: Set C minor scale.
	-	`cdefgab` automatically becomes `cde-fga-b-`

##### Modifying a key signature
If the key signature specification begins with `+`, `-` or `=`, the following
notes will be sharpened, flattened or become natural.

Examples:
-	`_{+cfg}`: `c`, `f`, `g` automatically becomes sharp.
	- This does not cancel the previously set key signature.

-	`_{D} _{=f}`: Set D major key signature, then make `f` natural.
	- End result: `cdefgab` automatically becomes `c+defgab`.

### Comparison with other MML formats (incomplete)
#### PMD
-	`[:]` (loops) Use `/` to break loops.
-	`l-<duration>` (borrow), use the `R<duration>` command instead.
-	`&` (tie), use the `^` command.
-	`&&` (slur), use the `&` command.
-	`%` (direct length) use `:` to specify note length in frames.

## Platforms
### MDSDRV (Mega Drive)
The `#platform` tag controls the PCM mixing mode when playing back files in
`mmlgui` or when converting to VGM.

Setting `#platform` to `megadrive` will use VGM datablocks and DAC stream
commands to play back samples. These files will have a smaller filesize, and
are suitable for conversion using other sound drivers like XGM. However, only
sample mixing and volume is not supported.

Setting `#platform` to `mdsdrv` will simulate MDSDRV's PCM driver. 2-3
PCM channels can be mixed, and 16 levels of volume control is possible.
The sample rate is however fixed to ~2khz increments up to 17.5 kHz, to match
the real MDSDRV code. By default, two channel mixing is enabled and the
output sampling rate is 17.5 Khz.

#### Channel mapping
-	`ABCDEF` FM channels 1-6.
-	`GHI` PSG tone channels 1-3. Channel `I` may be used for FM3 special mode.
-	`J` PSG noise channel.
-	`KL` PCM channels 2-3.
-   `MNOP` Dummy channels. May be used for FM3 special mode.

Channels `F`, `K` and `L` can play PCM instruments. PCM always takes priority
over the FM at channel 6 (`F`). If `#platform` is set to `mdsdrv`, software
mixing and volume control will also be for these PCM channels.

#### Platform-exclusive commands
-	`fm3 <mask>` - Enables FM3 special mode. Mask defines the operators that
	are affected by this channel. Example: `fm3 0011` to use operators 1 and 2.
	Set the mask to `1111` to disable the special mode. You can use this on PSG
	channel 3 (`I`) or the dummy channels (`KLMNOP`) to temporarily use this
	MML track for FM3.
-	`lfo <0..3> <0..7>` - Set hardware LFO parameters for a channel. The first
	parameter sets the AM sensitivity (tremolo depth) and the second parameter
	sets the PM sensitivity (vibrato depth).
-	`lforate <0..9>` - Sets the hardware LFO rate. 0 disables LFO, while `1..9`
	gradually increases the LFO speed. The last two settings are much faster.
-	`mode <0..1>` - For the PSG noise channel (`J`), this will enable the use
	of the third tone channel (`I`) as the noise frequency source. There will
	be a conflict if you try to control the frequency from both channels while
	this is active.
-	`pcmmode <2..3>` - Sets the PCM mixing mode. (`#platform mdsdrv` only).
	`pcmmode 2` supports 2 channel PCM mixing at up to 17.5 kHz, while
	`pcmmode 3` supports 3 channel PCM mixing at up to 13 kHz.
-	`pcmrate <1..8>` - Change the PCM pitch. The sample rate can be set in
	~2.2 kHz steps. This value is temporary and lasts until the next instrument
	change.
-	`write <register> <data>` - Write bytes directly to FM registers. Instead of
	specifying the register directly, the following aliases can also be used:
	`dtml*`, `ksar*`, `amdr*`, `sr*`, `slrr*`, `ssg*`, `fbal`, where `*`
	is replaced with the operator number. The effect of this command is
	temporary and lasts until the next instrument change.
-	`tl<1..4> <value>` - Change the base operator volume (total level). If a
	sign (`+` or `-`) is specified, the value is added or subtracted from the
	current level.

#### Limitations
Pan envelopes not supported.

Panning using the `p` command is only allowed for FM channels and the accepted
range is 0-3. Bit 1 enables the right channel, bit 2 enables the left channel.

#### FM instruments
FM instruments are defined as below: (Commas between values are optional)

	@1 fm ; finger bass
	;	ALG  FB
		  3   0
	;	 AR  DR  SR  RR  SL  TL  KS  ML  DT SSG
		 31   0  19   5   0  23   0   0   0   0 ; OP1 (M1)
		 31   6   0   4   3  19   0   0   0   0 ; OP2 (C1)
		 31  15   0   5   4  38   0   4   0   0 ; OP3 (M2)
		 31  27   0  11   1   0   0   1   0   0 ; OP4 (C2)
		  0 ; TRS (optional)

To enable AM for an operator, add 100 to the SSG-EG value.

##### 2op chord macro
Instrument type `2op` is used to duplicate FM instruments, modifying the
operators' multiply ratios and setting a transpose (useful if the base note
uses a multiplier which is not a power of 2). Disabled carrier operators are
also enabled, which can be useful if you use the same patch for both
monophonic and polyphonic sounds.

This feature is similar to the "Tone Doubler" in NRTDRV.

	@2 fm ;2op lead
	;	ALG  FB
		  4   0
	;	 AR  DR  SR  RR  SL  TL  KS  ML  DT SSG
		 20   5   0   1   1   9   0   4   7   0 ; OP1
		 31   8   4   7   2   0   0   4   0   0 ; OP2
		 20   5   0   1   1   9   0   1   7   0 ; OP3
		 31   8   4   7   2 127   0   1   0   0 ; OP4
	;         @ ML1 ML2 ML3 ML4 TRS
	@24 2op   2   5   5   4   4   0 ; n+4
	@25 2op   2   4   4   3   3   5 ; n+5
	@26 2op   2   7   7   5   5  -4 ; n+6
	@27 2op   2   6   6   4   4   0 ; n+7
	@28 2op   2   8   8   5   5  -4 ; n+8

#### PSG instruments
PSG instruments (envelopes) are defined as a sequence of values. 15 is max
volume and 0 is silence.

	@10 psg
		1 4 6 8 10 12 13 14 15

Use `>` to specify sliding from one value to another (important: no space
around the `>`)

	@11 psg
		15>10

Use `:` set the length of each value (in frames).

	@12 psg
		15:10     ; vol 15 for 10 frames
		15>0:100  ; from 15 to 0 in 100 frames

Set the sustain position with `/` or the loop position with `|`

	@13 psg
		15 14 / 13>0:7
	@14 psg
		0>14:7 | 15 10 5 0 5 10

When specifying envelopes, there must be no space between the
parameters in a node, as the space itself separates nodes.

#### PCM samples
PCM samples are defined as instruments. The first (and currently only)
parameter is the path to the sample (relative to that of the MML file).
The sample rate specified in the WAV file is used, and if the WAV file
has more than one channel, the first (left) channel is read.

	@30 pcm "path/to/sample.wav"

You can use a PCM sample from channels `F`, `K` and `L`. However, the
panning settings from FM channel 6 (`F`) are used and the FM output
from that channel is muted while PCM samples are playing.

It is possible to override the sample rate by adding the `rate` parameter:

	@30 pcm "path/to/sample.wav" rate=8000

(Note that it might pick the closest possible sample rate in `mdsdrv` mode)

It is possible to adjust the start position with the `offset` parameter:

	@30 pcm "path/to/sample.wav" offset=4000

#### Pitch envelopes
Pitch envelopes are specified with `@M<number>` and are used in MML
sequences with the `M<number>` command.

Pitch envelopes consist of up to 256 envelope nodes. Each node has
an initial position, a length parameter and a delta parameter.
The delta parameter is calculated from a "target" parameter and the
length parameter.

The parameters in the node are specified like this. As with PSG
envelopes, there must be no space between parameters in one envelope
node.

##### Commands
The initial and target parameters are defined in semitones. You can use
decimals, although they will be truncated to 8.8 bits fixed point.

-	`value>target:length` - Inserts a node with initial value,
	slide target and length.
-	`value>target` - Inserts a node with initial value and slide
	target. The length depends on the depth of the slide. **TODO**: describe
-	`value:length` - Inserts a node with length.
-	`value` - Inserts a node with a single frame. This can be placed at
	the end of the envelope to stop an infinite slide.
-	`|` Set loop position
-	`Vvalue:depth:rate` - Vibrato macro.
	- `value` is the pitch bias,
	- `depth` sets the depth of the vibrato in semitones.
	- `Rate` defines the duration of 1/4 of the vibrato waveform.
	Lower value = faster.

##### Examples
Simple arpeggio:

	@M1 | 0 3 5

Pitch slide from 2 semitones over 20 frames

	@M1 -2>0:20 0 ;last '0' position needed to stop the slide

Vibrato with 20 frame delay at the beginning

	@M1 0:20 | 0>0.5:5 0.5>-0.5:10 -0.5>0:5

Using the `V` macro to do the same thing as above

	@M1 0:20 | V0:1:5
