Angolmois -- the simple BMS player
=========

**Angolmois** is a [BM98][bm98]-like music video game which supports the [BMS
format][bms] for playing. On a plethora of BMS players, Angolmois has a unique
distinction of natively supporting multiple platforms and relatively recent
BMS extensions.

Please refer the official website at <http://mearie.org/projects/angolmois/>.
Angolmois is in active development, and you can report any bug or suggestion
to <https://github.com/lifthrasiir/angolmois/issues>. If you don't know about
BMS, read the introductory section at the end of this document first.

[bms]: http://en.wikipedia.org/wiki/Be-Music_Source
[bm98]: http://bm98.yaneu.com/bm98/


Features
--------

* Single executable.
  Every required assets including fonts (!) are generated from the source
  code. (As a result non-ASCII characters will be broken, but this does not
  affect the game play.) Maybe you can fit it to your rescue disk ;)

* Supports almost every platforms.
  I have tested at least three platforms (Windows, Linux, Mac OS X). SDL
  supports more platforms, so Angolmois may run on them. To my best knowledge,
  there is no other BMS player that supports two or more different platforms.

* Everything except the game play is done from the command line.
  This can be negative or positive. In the negative side, it (intentionally)
  lacks the song select interface. In the positive side, Angolmois can be
  combined with the shell script for music player or other game play mode.
  The music player is possible with the headless autoplay mode ("exclusive
  mode").

* Supports many recent BMS extensions.
  Basically, BME, BML (`#LNTYPE 1`, `#LNTYPE 2`, `#LNOBJ`), PMS, foot pedals,
  variable BPMs, support for various image/sound formats, multiple movie
  support, advanced control flow and so on. See the "BMS support status"
  section below.

* Rudimentary game play.
  While it is a big laggy and strange, Angolmois at least supports basic game
  elements like grading, score, combo, gauge and clear criterion. You can even
  enable modifiers like mirror and random in the command line.

* Free and open source software.
  The source code of Angolmois is available in the terms of [GNU General
  Public License version 2 or later][gnugpl]. This is also unusual for BMS
  players.

[gnugpl]: http://www.gnu.org/licenses/old-licenses/gpl-2.0.html


How To Compile (Unix-like)
--------------

You need a C compiler with full C99 support. The author typically uses GCC 4.6
or later, but earlier versions or Clang may work. Have a path to the compiler
in your `$PATH`.

You also need the following dependencies:

* [pkg-config][pkgconfig]
* [SDL][sdl] version 1.2.15 or later (but not 2.0)
* [SDL\_image][sdlimage] version 1.2 or later
* [SDL\_mixer][sdlmixer] version 1.2 or later
* [smpeg][smpeg] version 0.4 or later

[pkgconfig]: http://www.freedesktop.org/wiki/Software/pkg-config
[sdl]: http://www.libsdl.org/
[sdlimage]: http://www.libsdl.org/projects/SDL_image/
[sdlmixer]: http://www.libsdl.org/projects/SDL_mixer/
[smpeg]: http://icculus.org/smpeg/

In Mac OS X, you'd better have [Homebrew][homebrew] and issue the following:

	$ brew install sdl sdl_image sdl_mixer smpeg

[homebrew]: http://mxcl.github.com/homebrew/

In other platforms, you probably have a decent package manager so go for it.
If your package manager has two versions of the library, use the "development"
version (e.g. with the name ending with `-dev`).

Then the following command will do the work:

	$ make

If not, you should check the result of the following commands first:

* `pkg-config --cflags --libs sdl SDL_image SDL_mixer`
* `smpeg-config --cflags --libs`


How To Compile (Windows)
--------------

Again, you need a C compiler with full C99 support. Unfortunately Visual C++
compiler is not suitable for this task (it is a C++ compiler after all); so
you need to install [MinGW][mingw] first. You will need C and C++ compiler,
and MSYS environment. (C++ compiler is required because smpeg requires C++
runtime, sigh.)

[mingw]: http://mingw.org/

In theory, the above instruction should also apply to MSYS environment. In
reality, MinGW does not come with a good package manager so it is hard to
follow. So in this section I'll give an instruction which is barely enough to
compile Angolmois and nothing else.

Download the following files:

* [SDL][sdl]: `SDL-devel-1.2.*-mingw32.tar.gz`
* [SDL\_image][sdlimage]: `SDL_image-devel-1.2.*-VC.zip` (yes, this is fine)
* [SDL\_mixer][sdlmixer]: `SDL_mixer-devel-1.2.*-VC.zip`
* [smpeg][smpeg]: Actually, `smpeg.dll` is included in SDL\_mixer (for MP3
  support). You however need two smpeg header files: [smpeg.h][smpeg.h] and
  [MPEGfilter.h][mpegfilter.h].

[smpeg.h]: http://svn.icculus.org/*checkout*/smpeg/tags/release_0_4_5/smpeg.h
[mpegfilter.h]: http://svn.icculus.org/*checkout*/smpeg/tags/release_0_4_5/MPEGfilter.h

Now extract all the files to your MinGW directory. For SDL, you can safely
extract it to the MinGW root. For SDL\_image and SDL\_mixer, extract `include`
directory as is and `lib\x64` or `lib\x86` (depending on your Windows
settings) directory to `lib` directory. Then copy all DLL files to
the directory where `angolmois.c` lives.

Angolmois' `Makefile` assumes that pkg-config is available, so it won't work.
Try the following command instead if `make` does not work:

	$ cc -Os -Wunused -Wall -W -std=c99 angolmois.c \
	  `sdl-config --prefix=/mingw --cflags --libs` \
	  -lSDL_mixer -lSDL_image -lsmpeg -o angolmois.exe

Then try launch it from MSYS:

	$ ./angolmois.exe

It should launch a file dialog, and if you cancel it it will create
`stderr.txt` file in the same directory as `angolmois.exe`. (This is a default
behavior of `SDLmain` library in Windows.) The file should contain the usage.

If it works fine in MSYS, also try launch it from outside. If it does not
launch with an error message that demands some DLL files, you should copy them
from your MinGW directory (typically `libstdc++-*.dll` and `libgcc_*.dll`).

You are greatly welcomed to suggest easier build instructions in Windows. :(


How To Play
-----------

In the normal game play mode (see the next section for other modes), you will
have the screen like this:

	SCORE 0098304        |
	--------------------'
	    |  |  |  |  |  |
	    |  |  |  |  |  |      +----------------+
	____|__|__|__|__|__|      |                |
	    |  |  |  |  |  |      |                |
	    |==|GREAT|  |  |      |                |
	====|  7 COMBO  |  |      |                |
	    |  |  |  |  |  |      |                |
	    |  |  |==|  |==|      |                |
	    |  |  |  |  |  |      |                |
	--------------------.     +----------------+
	 3.0x     @  23.5604 |
	          BPM  99.03 |
	 _______v___________ |
	         0:41 / 2:03 |
	#############::::::::::::::

The game screen consists of the actual chart area (middle left),
the information area (top/bottom left) and the BGA area (center). The BGA may
not appear if the BMS file does not support it or you've disabled it.

The goal of the game is as other BMS players: to press the button (or spin
the scratch etc.) according to the chart displayed as accurate as possible.
The chart continuously flows from the top to the bottom (its rate may change
however), and in each lane the note will move downwards. As the note touches
the bottom of the area you should press the corresponding button. The accuracy
is reported in the middle of the chart area: "COOL", "GREAT", "GOOD", "BAD"
and "MISS". The last means that you have completely missed the note, and
others are given according to the accuracy. If you have received a row of
"GREAT" or better grades, the "combo" number will increase and your health
gauge (the bottom of the screen) will also increase. "GOOD" does not affect
the "combo" number, and "BAD" resets it and decreases the gauge. You have
cleared the song when the gauge is above the threshold at the end of the song,
and Angolmois will print the statistics like this to the console:

	*** CLEARED! ***
	COOL   697    GREAT  701    GOOD   149    
	BAD     39    MISS    46    MAX COMBO 168
	SCORE 0404681 (max 0732540)

If you didn't manage it (Angolmois does not support the notion of immediate
death on depleted gauge), Angolmois will simply print:

	YOU FAILED!

You can see the play speed (i.e. how fast the chart flows) at the bottom left
of the screen (in this example, "3.0x"), and increase/decrease it by pressing
F3 and F4. (See `--speed` and `ANGOLMOIS_SPEED_KEYS` for customization.)

The information area also displays the current score, the current measure
position (useful for determining the parts of the song), the current BPM (i.e.
how fast the song itself is) and the current song position along with the play
speed.

The following facts will be useful for more serious players:

* There are horizontal bars on the chart. This is called a measure bar, and
  does not affect the game play but identifies the parts of the song. Combined
  with the measure position, this will help you determine the speed of the
  song and chart. (Basically, if you don't see the measure bar at all it is
  very fast.)

* Some songs have long notes, which are vertically expanded notes. You should
  press the button at the start of long notes and stop pressing it at the end.
  If you stop pressing too early or too lately you will get a "MISS" grade.

* Many songs have multiple patterns, normally labelled "normal", "hyper" and
  "another" (comes from _Beatmania IIDX_ rules). If you feel the chart too
  easy or too hard, search for other patterns in the same directory. If you
  are hardcore, you may even find a hidden pattern (disguised as other files)!


Command Line Options
--------------------

Angolmois uses a command line for controlling its behavior. The following
options are available:

### `--help`, `-h`

Shows a brief usage.

### `--version`, `-V`

Shows a version of Angolmois.

### `--speed <number>`, `-a <number>`

Sets the initial play speed to `<number>`. The default speed is 1.0x
(equivalent to `-a 1.0`).

The play speed can range from `0.1` to `99.0`. Other than that, you can set it
as you want, for example to 3.1415x. The play speed can also be changed with
F3/F4 keys (or keys specified in `ANGOLMOIS_SPEED_KEYS`) during the game play,
but this is limited to predefined speeds only.

The play speed does not affect the grading, and is only provided for
convenience.

### `-1` to `-9`

Same as `--speed 1.0` to `--speed 9.0`.

### `--autoplay`, `-v`

Enables the AUTO PLAY (viewer) mode. In this mode, you can only adjust
the play speed and the simulated play will be displayed.

### `--exclusive`, `-x`

Enables the *exclusive mode*. In this mode, the screen is reduced to 256 by
256 pixels and the game play screen is entirely missing. The play position and
information is displayed in the console. The ESC key still works when
the screen is on focus, but other keys won't work.

`--fullscreen` is ignored in the exclusive mode; the screen will always launch
in the new window.

If `--no-bga` (`-B`) option is also present, the screen does not launch at all
and it becomes the sound only mode. In this mode you can only stop
the playback with Ctrl-C (or equivalent signals like SIGINT).

### `--sound-only`, `-X`

Enables the sound only mode. Same as `--exclusive --no-bga`.

### `--fullscreen`

Enables the fullscreen mode. Your graphics driver should support 800x600
screen resolution (otherwise it will exit immediately).

This is default, so there is actually no point to use this option.

### `--no-fullscreen`, `-w`

Enables the windowed mode. The window is measured 800x600 or 256x256 depending
on the mode. You need to focus the screen for key input (so beware of your
meta or Windows key!).

### `--info`

Shows a brief information about the song. In the normal mode, this will be
overlayed on the loading screen; in the other modes it will print to
the console.

This is default, so there is actually no point to use this option.

### `--no-info`, `-q`

Does not show an information about the song. In the normal mode, this will
only affect the loading screen. In the other modes, this will disable all
output to the console (song information and playback status). This does not
disable any warning or error message however.

### `--mirror`, `-m`

Enables a mirror modifier, if this is the last specified modifier. (This is
common to other modifiers; modifiers do not overlap each other.)

The mirror modifier flips the entire chart horizontally, except for
the scratch and foot pedal (if any). You should input the keys accordingly;
if a note at key 1 is moved to key 5 then you should press key 5. The modifier
does not affect the timing (in fact, no modifier does so).

This is useful for patterns with many notes crowded in only one side.

### `--shuffle`, `-s`

Enables a shuffle modifier.

The shuffle modifier swaps keys randomly. If, however, you have several notes
common to one key, then those keys will move altogether. The modifier does not
affect the scratch and foot pedal (if any).

This is useful for patterns with notes continuously flowing one side to
another side (so called "floors"), as this makes a single hand handle too many
notes. The modifier may break the chain, splitting the load to both hands.

### `--shuffle-ex`, `-S`

Enables a shuffle modifier, but also affects the scratch and foot pedal
(if any). Otherwise same as `--shuffle`. There is no difference between
`--shuffle` and this modifier for PMS.

### `--random`, `-r`

Enables a random modifier.

The random modifier swaps *notes* randomly. This means that you never know
where the notes comes from, even if you fully memoized the chart. This also
tends to make patterns hard to handle. You have been warned. ;)

This option handles long notes correctly, so there won't be a note
accidentally swapped into the longnote.

### `--random-ex`, `-R`

Enables a random modifier, but also affects the scratch and foot pedal
(if any). Otherwise same as `--random`. There is no difference between
`--random` and this modifier for PMS.

### `--preset <string>`, `-k <string>`

Sets the key preset. The key preset is a predefined specification of how lanes
are ordered and displayed. The following preset names are available:

* `5` and `5/fp`: One scratch, 5 keys, one foot pedal (only for `5/fp`)
* `7` and `7/fp`: One scratch, 7 keys, one foot pedal (only for `7/fp`)
* `10` and `10/fp`: Two scratches, 10 keys, two foot pedals (only for `10/fp`)
* `14` and `14/fp`: Two scratches, 14 keys, two foot pedals (only for `14/fp`)
* `9`: 9 buttons with channels `11`--`15` and `22`--`25`
* `9-bme`: 9 buttons with channels `11`--`19`
* `bms`, `bme` or `bml`: Selects one of `{5,7,10,14}[/fp]` automatically
* `pms`: Selects one of `9` and `9-bme` automatically

All names are case-insensitive. If not specified, Angolmois automatically uses
the preset `pms` for files which name ends with `.pms` and `bms` for others.

### `--key-spec <string> <string>`, `-K <string> <string>`

Sets the custom key specification. This overrides prior `--preset` options.
This option can be used for emulating various BMS extensions not supported by
Angolmois itself.

The key specification composed of two strings, one for the left panel and one
for the right panel. The distinction between two panels is only useful for
couple play (`#PLAYER 2`), and they will be merged to one panel otherwise.

Each string contains three-letter lane specification optionally separated by
whitespace (the string can be empty if the right panel is not used). First two
letters are the note channel used for given lane (`11`, `26` etc.), and the
third lowercased letter specifies the kind of given lane:

	Primarly used for BMS/BME/BML:
	a       White key
	y       White key (displayed yellow)
	b       Black key (displayed blue)
	s       Scratch (displayed red)
	p       Foot pedal (displayed green)

	Primarly used for PMS:
	q       White button
	w       Yellow button
	e       Green button
	r       Navy button
	t       Red button

One key cannot have multiple lanes. The maximum possible number of lanes is
therefore 72 (`10` to `1Z`, `20` to `2Z`), but it won't display properly so
about 15 to 20 lanes are the sensible maximum. Likewise, too small number of
lanes (less than 5) may cause a problem.

In order to illustrate the point, the option `--preset 10/fp` is actually
a shorthand for this equivalent key specification:

	--key-spec '16s 11a 12b 13a 14b 15a 17p' '27p 21a 22b 23a 24b 25a 26s'

...and the [OCT/FP][octfp] extension which Angolmois does not support natively
can be emulated as follows:

	--key-spec '21p 16s 11a 12b 13a 14b 15a 18b 19a 22b 23a 24b 25a 28b 29a 26s' ''

[octfp]: http://wayback.archive.org/web/20071103011620/http://www.diana.dti.ne.jp/~idee/octave.html

### `--bga`

Loads and shows the BGA.

This is default, so there is actually no point to use this option.

### `--no-bga`, `-B`

Do not load and show the BGA. This is useful when you don't like distracted by
the BGA, or you can't wait for the BGA loaded.

Combined with `--exclusive`, this option disables the screen. See
`--exclusive` for more information.

### `--no-movie`, `-M`

Do not load and show the BGA movie. This is useful when your system is slow so
the BGA lags the entire game.

### `--joystick <index>`, `-j <index>`

Enables the joystick. The `<index>` is normally 0, but if you have multiple
joystick-compatible devices it can be higher. Ultimately you test for
the correct value.

Angolmois' joystick implementation is targeted to the consumer _Beatmania_
controller and its clones, so other devices may not work. If your device
does not work with Angolmois, you may try joystick-to-keyboard utilities like
[Joy2Key][joy2key].

Joystick input slightly differs from normal keyboard input. Most importantly,
spinning the scratch forward and backward becomes different inputs
(e.g. breaking the long note input).

[joy2key]: http://www.electracode.com/4/joy2key/JoyToKey%20English%20Version.htm

### `--`

Ends the option processing. You need this option before the file name if
the file name starts with `-`.

### File name

An argument not starting with `-` is considered a file name. You need
the exact path to BMS/BME/BML/PMS file (the extension does not really matter
except for PMS, though), and other image and sound files are resolved in
the directory where the BMS/BME/BML/PMS file is (unless `#PATH_WAV` is
in effect; see the "BMS support status" section).

You may have two or more file names, but only the first is used. Multiple file
names are reserved for later extension.

If the file name is missing, and if you are using Windows version of
Angolmois, the file dialog will ask for the BMS/BME/BML/PMS file. This makes
a batch file using Angolmois relatively easier. In the other platforms, you
may use [dialog][dialog] or [zenity][zenity] for similar functionality.

[dialog]: http://invisible-island.net/dialog/dialog.html
[zenity]: http://library.gnome.org/users/zenity/stable/


Environment Variables
---------------------

The keys for Angolmois can be controlled with the environment variables
(except for ESC, which is always used as an exit key):

### `ANGOLMOIS_1P_KEYS`

Sets the keys for 1P. This environment variable is only used for BMS, BME or
BML files (see `ANGOLMOIS_XXy_KEY` for actual details). The environment
variable should follow the following format:

	<scratch>|<key1>|<key2>|<key3>|<key4>|<key5>|<key6>|<key7>|<pedal>

...where `<key1>` etc. are actual keys for key 1. The actual keys are
specified as a SDL virtual key name, such as `a`, `right shift`, or `f1`.
(See the list in the [SDL wiki][sdlkeys].) The name is case-insensitive.
You can leave the name as an empty string if you are not interested in that
key (e.g. no 7KEY); in that case that key cannot be pressed in the game.

[sdlkeys]: http://www.libsdl.org/cgi/docwiki.cgi/SDLKey

Multiple keys can be set, by separating each key name with `%`. (It looks
obscure, but SDL uses lots of punctuations as a key name so we have no other
choices.) Angolmois considers that the key is being pressed when the first
actual key mapped to it is being pressed, and the key is being unpressed when
the last actual key mapped to it is being unpressed. Unpressing one key while
other mapped keys are pressed is ignored.

When the joystick is available, special key names `button <index>` and `axis
<index>` can be used. The `<index>` should be a number from 0 to the number of
buttons/axes minus 1. Note that the joystick axis is considered as an input
when the axis is out of the origin by more than 10% of its range, and moving
the axis from one end to another end is considered as unpressing then pressing
the key (this is how the consumer _Beatmania_ controller works). This may not
suitable for other devices.

The default value is as follows:

* `<scratch>`: `left shift`, `axis 3`
* `<key1>`: `z`, `button 3`
* `<key2>`: `s`, `button 6`
* `<key3>`: `x`, `button 2`
* `<key4>`: `d`, `button 7`
* `<key5>`: `c`, `button 1`
* `<key6>`: `f`, `button 4`
* `<key7>`: `v`, `axis 2`
* `<pedal>`: `left alt`

### `ANGOLMOIS_2P_KEYS`

Sets the keys for 2P. This environment variable is only used for BMS, BME or
BML files. The format is as follows:

	<pedal>|<key1>|<key2>|<key3>|<key4>|<key5>|<key6>|<key7>|<scratch>

Otherwise same as `ANGOLMOIS_1P_KEYS`. The default value is as follows:

* `<pedal>`: `right alt`
* `<key1>`: `m`
* `<key2>`: `k`
* `<key3>`: `,`
* `<key4>`: `l`
* `<key5>`: `.`
* `<key6>`: `;`
* `<key7>`: `/`
* `<scratch>`: `right shift`

### `ANGOLMOIS_PMS_KEYS`

Sets the keys for 9KEY. This environment variable is only used for PMS files
(see `ANGOLMOIS_XXy_KEY` for actual details). The format is as follows:

	<key1>|<key2>|<key3>|<key4>|<key5>|<key6>|<key7>|<key8>|<key9>

Otherwise same as above. The default value is as follows:

* `<key1>`: `z`
* `<key2>`: `s`
* `<key3>`: `x`
* `<key4>`: `d`
* `<key5>`: `c`
* `<key6>`: `f`
* `<key7>`: `v`
* `<key8>`: `g`
* `<key9>`: `b`

### `ANGOLMOIS_SPEED_KEYS`

Sets the keys for play speed change. The format is as follows:

	<speed down>|<speed up>

Otherwise same as above. The default value is as follows:

* `<speed down>`: `f3`
* `<speed up>`: `f4`

### `ANGOLMOIS_XXy_KEY`

Sets the keys for given channel. The format is same as above, but `|` cannot
appear. This kind of variables are processed after other environment
variables, so they can override keys for the specific channel. A key itself
can be used for only one channel however.

These are intended as a general way to set keys when `--key-spec` is used.
Like `--key-spec`, `XX` should be an uppercased alphanumeric key for the note
channel (`11`, `26` etc.) and `y` should be one lowercased letter which
identifies the kind of that channel. See `--key-spec` for details.

If multiple environment variables for same channel and different kind are
available, Angolmois uses at most one variable with the matching channel and
kind (or none if there is no match). This distinction is also used for
`ANGOLMOIS_{1P,2P,PMS}_KEYS` environment variables: they assign the specified
keys only when the channel and predefined kind agrees to each other.


BMS Support Status
------------------

In brief, Angolmois supports the following commands and channels:

* `#ARTIST <string>`
* `#BGAxx yy <integer> <integer> <integer> <integer> <integer> <integer>`
* `#BMPxx <path>`
* `#BPM <number>`
* `#BPMxx <number>`
* `#ELSE`
* `#ELSEIF <integer>`
* `#ENDIF` (actually, `#END`)
* `#ENDRANDOM`
* `#GENRE <string>`
* `#IF <integer>`
* `#LNOBJ xx`
* `#LNTYPE 1`, `#LNTYPE 2`
* `#PATH_WAV <path>`
* `#PLAYER 1`, `#PLAYER 2` (limited), `#PLAYER 3`
* `#PLAYLEVEL <integer>`
* `#RANDOM <integer>`
* `#RANK <integer>`
* `#SETRANDOM <integer>`
* `#STAGEFILE <path>`
* `#STOPxx <integer>`
* `#STP<integer>.<integer> <integer>`
* `#TITLE <string>`
* `#WAVxx <path>`
* Channel  `01` for BGM
* Channel  `02` for measure rescaling
* Channels `03` and `08` for variable BPM
* Channels `04`, `06`, `07` and `0A` for BGA and POOR BGA
* Channel  `09` for STOP
* Channels `1x` and `2x` for visible objects
* Channels `3x` and `4x` for invisible objects
* Channels `5x` and `6x` for long-note objects
* Channels `Dx` and `Ex` for bomb objects

Due to the sheer amount of contents, the remainder of this section has been
moved to `INTERNALS.md` file.


Appendix: What is BMS?
----------------------

This section is a brief history and introduction to BMS. You can skip this
section if you already know BMS and are ready to play.

BMS is originated from Konami's successful music video game, _Beatmania_.
First released in December 1997, _Beatmania_ was so influential in the music
video game genre that lots of clones (and especially PC clones) have been
produced. One of such clone, BM98, was developed as a simulator for practicing
_Beatmania_ charts. Since the game lacked the actual game data, and it is no
way to legitimately get it, BM98 devised its own data format so anyone can
make the charts for BM98. This formed the basis of the modern BMS[^1] format.

[^1]: It is obvious that the "BM" of BM98 alludes *Beatmania*, but it has
      explicitly backronymed to "Be-Music" to avoid the trademark problem.
      BMS thus stands for "Be-Music Script" or "Be-Music Source".

While BMS originally targeted the reproduction of commercial music game, it
has much more used as a medium of amateur or free music accompanied by music
game element. It has survived the initial demise of BM98 (Urao Yare had to
stop the development due to the [legal and personal concerns][whystopbm98]),
and its plain text nature allowed other programs (now called "BMS players")
can use and extend it at their own needs. The entire BMS subculture has
emerged (this is comparable to the modern [demoscene][demoscene]), and it was
a _de facto_ gateway for amateur musicians until mid-2000s when the websites
like _Nico Nico Douga_ have taken this role. Nevertheless, the BMS scene is
very much alive to this day, and thousands of original BMSes are produced
every year.

[whystopbm98]: http://bm98.yaneu.com/bm98/gamelab9904.txt
[demoscene]: http://en.wikipedia.org/wiki/Demoscene

As a music video game, BMS is a reproduction of _Beatmania_ which used five
keys and one turntable ("scratch"). There are now dozens of other modes of
BMS, of which the following are widely used:

* "BME", which uses seven keys and one turntable. This is a reproduction of
  _Beatmania IIDX_ which is a sequel series to _Beatmania_.
* "BML", which introduces a long note which forces the player to input the key
  (or spin the turntable) for given duration. This is a reproduction of
  _EZ2DJ_, the South Korean music video game which has similar elements to
  _Beatmania_ but turned out to have different gaming experience at the end.
  _Beatmania IIDX_ has introduced this very element later in 2009.
* Double play (DP), which uses the both (1P/2P) sides of controllers at once.
  Often called as 10 keys and 14 keys modes, it is now an integral part of
  the BMS format. Normal play mode is now called Single Play (SP).
* "PMS", which uses nine colorful keys. This is a reproduction of _Pop'n
  Music_, and technically same as 5 keys DP but requires wildly different UI.

The maturity of BMS scene made easy to get quality BMSes. The followings are
common methods:

* If you are new to BMS, I recommend you to search for "BMS starter pack"
  which contains several BMSes and default player software. (This does not
  mean that the player is required for them, Angolmois will normally play them
  flawlessly.)
* Every year about a dozen BMS events are open, where the creators submit
  their BMSes according to the event rules. In Japan the [Digital Emergency
  Exit 2][dee2events] has provided a room for many BMS events, and there are
  similar events in other countries (e.g. [KOREA BMS PARTY][kbp]).
* For more advanced players, there are plenty of "delta" patterns for
  existing songs that have been created separately.

[dee2events]: http://manbow.nothing.sh/event/event.cgi
[kbp]: http://k-bms.com/


Appendix: Acknowledgements
--------------------------

I'd like to thank to the following persons for inspiration and advice:

* Choi Kaya for the namesake; he is the creator of the [Project
  Angolmois](http://angolmois.net/), which inspired me to create a source code
  shaped as a recognizable image.
* Park Joon-Kyu, Park Jiin, Hye-Shik Chang, Park Jaesong and numerous others
  for helping the initial debugging phase of Angolmois 1.0.
* [Hitkey](http://hitkey.nekokan.dyndns.info/) for the thorough analysis of
  Angolmois and other BMS players.
* [Nekokan](http://nekokan.dyndns.info/) for the Japanese translation of this
  `README.md` and `INTERNALS.md`. :)

