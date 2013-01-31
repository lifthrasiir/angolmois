Angolmois Internals
===================

This document is intended for serious BMS writers and other software
developers.


BMS Support Status
------------------

BMS has lots of extensions (quite comprehensive document can be found
[here][bmscmds]) and Angolmois supports lots of them. As most extensions are
not specified or specified vaguely, this section tries to cover most concerns
useful for BMS writers and other software developers.

[bmscmds]: http://hitkey.nekokan.dyndns.info/cmds.htm

This section is not intended to be complete. In doubt, always consult
the source code. The source code of Angolmois is in public after all.

### Basic Rules

* Lines can end with `\r` (Mac OS classic), `\n` (Unix) or `\r\n` (Windows).
* Too long lines (currently, about 4000 bytes) may be cut.
* Everything is parsed case-insensitively if not specified.
* Every 1296 two-letter alphanumeric key is supported. That is, you can always
  use `00` to `ZZ`. They are case-insensitive as well.
* Both one or more tabs and spaces count as whitespace.
* Preceding whitespace is ignored.
* Invalid key or number results in the command ignored entirely, but if the
  prefix of the line parses to the correct command then the remainder is
  ignored.[^1] As an exception, invalid keys in the data section does not
  affect other valid keys.
* The duplicate command overrides the prior command unless the command itself
  allows for duplicates.
* In the absence of duplicates and possible ambiguities, all commands and data
  are processed independently of their relative positions.

[^1]: Thus `#PLAYLEVEL 12.3` is valid even while `#PLAYLEVEL` requires
      an integer; it is equivalent to `#PLAYLEVEL 12`.

### Metadata

In the following commands, `<string>` is parsed case-sensitively.

* `#TITLE <string>`
* `#GENRE <string>`
* `#ARTIST <string>`
* `#PLAYLEVEL <integer>` (defaults to `#PLAYLEVEL 0`)

They are displayed before the actual game play unless `--no-info` option is
given. Angolmois does not try to detect or assume the character encoding;
bytes outside the ASCII region will displayed broken.

### Game Settings

* `#PLAYER 1` (default)
* `#PLAYER 2`
* `#PLAYER 3`

Corresponds to the single play (SP), couple play and double play (DP) mode
respectively. Angolmois does not support battle play (`#PLAYER 4`) and has
very limited support for couple play (`#PLAYER 2`).

Angolmois always puts the notes on the left hand side and BGA on the right
hand side. `#PLAYER 2` is implemented as split notes on both sides, and
otherwise same as `#PLAYER 3` (they share the same gauge and grading).

* `#RANK <integer>` (defaults to `#RANK 2`)

The current system will give BAD, GOOD, GREAT and COOL grade if the note is
approximately[^2] within `±{144, 84, 48, 14.4} / (1.5 - #RANK * 0.25)`
milliseconds respectively. Unpressing the key at the end of LN has the same
grading area as BAD. The following table summarizes the hard values:

	#RANK:  5       4       3       2       1       0       -1      ...

	BAD:    576ms   288ms   192ms   144ms   115ms   96ms    82ms    ...
	GOOD:   336ms   168ms   112ms   84ms    67ms    56ms    48ms    ...
	GREAT:  192ms   96ms    64ms    48ms    38ms    32ms    27ms    ...
	COOL:   58ms    29ms    19ms    14ms    12ms    10ms    8ms     ...

This results in considerably narrower COOL area and wider GREAT area, and also
support for every integer `#RANK` up to 5. Angolmois `#RANK` system is
certainly ill-designed however, so it may change without a notice. 

[^2]: Due to the current implementation strategy, the actual grading area may
      vary when the measure is rescaled or BPM is changed.

### Resources

In the following commands, `<path>` refers to the file path relative to the
BMS/BME/BML file (or `#PATH_WAV` if any). The file path may contain
directory separators `/` or `\` which are automatically normalized. The file
name matching is done case-insensitively even in non-Windows platforms.

If the original path was given a file extension, Angolmois also tries to use
alternative file extensions for given kind of resources. This may result in
multiple candidates for a single path; in this case Angolmois choose one
arbitrarily. If the candidate does not exist, Angolmois will issue an warning.

* `#WAVxx <path>`
* Channel `01`

Angolmois supports every audio file format that SDL\_mixer supports, including
WAV, OGG (Vorbis), MP3 (MPEG-1/2 Audio Layer 3). Angolmois also tries to use
common audio file extensions: `.wav`, `.ogg` and `.mp3`. `#WAV00` is currently
unused, but loaded anyway if given.

Due to the limitation of SDL's WAV parser, WAVs with sampling rates other than
11025, 22050 or 44100 Hz may sound incorrectly.

* `#BMPxx <path>`
* Channels `04` (bottom layer), `06` (POOR BGA) and `07` (top layer)

Angolmois supports every image file format that SDL\_image supports, including
BMP (including RLE-compressed ones), PNG, JPEG, GIF and so on. Angolmois also
tries to use common image file extensions: `.bmp`, `.png`, `.jpg`/`.jpeg` and
`.gif`. `#BMP00` is used as an initial POOR BGA if given. If the image has an
alpha channel it will retain its transparency; otherwise the color black
(`#000000`) will be used as a transparent color.

Angolmois also supports an MPEG-1 video file playback if the file name ends
with `.mpg` (case insensitive). There may be multiple video files loaded and
played. It plays while it is "in display", that is, not replaced by other
image or video (but may have been obscured by other layers or hidden due to
POOR BGAs). If one video is shared among multiple layers, the playback status
is shared among them and may restart unexpectedly. A video does not support
a transparent color.

If the image or video is larger than 256 by 256 pixels, only the upper left
region will be used. Conversely, smaller one will be padded to the transparent
color (or black).

* `#BGAxx yy <integer> <integer> <integer> <integer> <integer> <integer>`

The image composition using `#BGAxx` is done after completely loading images,
and executed in the order of appearance. The target slot (`xx`) should be
empty, i.e. not used by `#BMPxx` or other `#BGAxx`, and should not be a video.

The top/left coordinates are clipped to 0 if negative; the width and height
calculated from both coordinates are clipped to 256 if larger than 256. Other
clippings do not occur.

* `#STAGEFILE <path>`

The image is displayed before the actual game play, along with other metadata.
Otherwise its behavior is same as `#BMPxx` without a video support. The image
is automatically rescaled to 800 by 600 pixels.

* `#PATH_WAV <path>` (defaults to the directory containing the BMS file)

This command overrides the base directory used for resolving file paths. It
may be located anywhere in the BMS file; the resolution is done after parsing.

### Playback Settings

* `#BPM <number>` (defaults to `#BPM 130`)
* `#BPMxx <number>`
* Channels `03` and `08`

Variable BPM is fully supported, with a caveat that very large or very small
BPM may not work as intended (related to floating point calculations). You can
mix a hexadecimal BPM (channel `03`) and extended BPM format (channel `08`);
extended BPM is preferred if they are in the same position.

Angolmois has a limited support for negative BPM. A negative BPM is assumed
specially to indicate the end of the song, so that no more grading is
performed and you can only watch the chart scrolling backwards until it hits
the bottom (measure -1). If not all objects have been graded (like many
variations of "Sofa $15 -> $1"), it is not considered "cleared" and the result
message is ignored.

Angolmois does not support zero BPM. Do not try this at home.

* `#STOPxx <integer>`
* `#STP<integer>.<integer> <integer>`
* Channel `09`

Again, stopping scroll for certain amount of time (hereafter "STOP") is
supported with a similar caveat as variable BPM. You can specify a STOP time
as a multiple of 192nd of measure (`#STOPxx` and channel `09`) or milliseconds
(`#STP`). Angolmois will choose one of them arbitrarily if they are in
the same position.

Negative STOP time will cause a crash. Do not try this at home.

### Data Section

In general, data section does not allow whitespace between keys, but does
strip preceding and trailing whitespace.

* Channel `02` (defaults to 1.0)

Technically speaking, the size of measure affects the interval of horizontal
measure bars and nothing else. Angolmois does not support a measure size less
than 0.001.

* Channel `1x` and `2x`

...

* `#LNOBJ xx`

...

* Channel `3x` and `4x`

...

* `#LNTYPE 1` (default)
* Channel `5x` and `6x`

...

* `#LNTYPE 2`

...

### Control Flow

These commands may occur at any position in the file.

* `#RANDOM <integer>`

Upon the `#RANDOM` command, Angolmois generates one random number between one
and given integer (inclusive) with an equal probability. If the integer was
zero or negative the random number is still "generated", but put in the
indeterminate state so that the number is not equal to any integer. Similarly
the initial random number is in the indeterminate state.

* `#SETRANDOM <integer>`

Functionally similar to `#RANDOM`, but the random number is fixed to given
integer. The number is put in the indeterminate state if the integer is zero
or negative.

* `#ENDRANDOM`

`#ENDRANDOM`, which closes the innermost `#RANDOM` block, is recommended for
reducing memory consumption but not required. It does nothing if there is no
open `#RANDOM` block.

* `#IF <integer>`
* `#ELSEIF <integer>`
* `#ELSE`
* `#ENDIF` (actually, `#END`)

These commands affects a flag whether the following commands will be processed
or not. The integer in `#IF` and `#ELSEIF` should be positive, otherwise the
following commands are unconditionally ignored.

`#IF` blocks cannot be nested unless other `#RANDOM` or `#SETRANDOM`
blocks wrap them; the nested `#IF` overrides the preceding `#IF`. In the
absence of an open `#IF` block, `#ELSEIF` is same as `#IF` and `#ELSE` always
toggles a processing flag while implicitly opening `#IF` block. `#ENDIF`
closes an open `#IF` block; over the course it also implicitly closes
innermost `#RANDOM` blocks without no open `#IF` block.

Angolmois assumes every line starting with `#END` as the end of `#IF` block,
except for `#ENDRANDOM` and `#ENDSW` (unsupported but disambiguated). This is
intended to handle occasional mistakes like `#END IF`.

