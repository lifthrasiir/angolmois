/*
 * Angolmois -- the simple BMS player
 * Copyright (c) 2005, 2007, 2009, 2012, 2013, Kang Seonghoon.
 * Project Angolmois is copyright (c) 2003-2007, Choi Kaya (CHKY).
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <smpeg.h>

static const char VERSION[] = "Angolmois 2.0.0 alpha 2";
static const char *argv0 = "angolmois";

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define R(x,y,w,h) &(SDL_Rect){x,y,w,h}

#define INFO_INTERVAL 47 /* try not to refresh screen or console too fast (tops at 21fps) */
#define MEASURE_TO_MSEC(measure,bpm) ((measure) * 24e4 / (bpm))
#define MSEC_TO_MEASURE(msec,bpm) ((msec) * (bpm) / 24e4)

/******************************************************************************/
/* utility declarations */

static void die(const char *msg, ...);
static void warn(const char *msg, ...);
#define SHOULD(x) ((x) ? (void) 0 : die("assertion failure: %s", #x))

struct xv_base { ptrdiff_t xv__size, xv__alloc; };
#define XV(...) struct { struct xv_base xv__base; __VA_ARGS__ *xv__ptr; }
#define XV_BASE(xv) ((xv).xv__base)
#define XV_SIZE(xv) (XV_BASE(xv).xv__size)
#define XV_ALLOC(xv) (XV_BASE(xv).xv__alloc)
#define XV_PTR(xv) ((xv).xv__ptr)
#define XV_ITEMSIZE(xv) (ptrdiff_t) sizeof(*XV_PTR(xv))
#define XV_EMPTY {.xv__base = {.xv__size = 0, .xv__alloc = 0}, .xv__ptr = NULL}
#define XV_INIT(xv) (XV_SIZE(xv) = XV_ALLOC(xv) = 0, XV_PTR(xv) = NULL)
#define XV_INVARIANT(xv) \
	(XV_SIZE(xv) >= 0 && XV_ALLOC(xv) >= 0 && XV_SIZE(xv) <= XV_ALLOC(xv) && \
	 (XV_ALLOC(xv) > 0) == (XV_PTR(xv) != NULL))
#define XV_CHECK(xv,i) ((ptrdiff_t) (i) >= 0 && (ptrdiff_t) (i) < XV_SIZE(xv))
#define XV_RESERVE(xv,n) \
	((ptrdiff_t) (n) > XV_ALLOC(xv) && \
	 (XV_PTR(xv) = xv_do_resize(&XV_BASE(xv), XV_PTR(xv), (n), XV_ITEMSIZE(xv)), 1))
#define XV_RESIZE(xv,n) (XV_SIZE(xv) = (ptrdiff_t) (n), XV_RESERVE(xv, XV_SIZE(xv)))
#define XV_RESIZEBY(xv,n) XV_RESIZE(xv, XV_SIZE(xv) + (ptrdiff_t) (n))
#define XV_AT(xv,i) (XV_PTR(xv)[(ptrdiff_t) (i)])
#define XV_ATEND(xv,i) (XV_PTR(xv)[XV_SIZE(xv) - (ptrdiff_t) (i) - 1])
#define XV_LOOP(xv,itype,i,before,after) \
	for (itype (i) = 0; (ptrdiff_t) (i) < XV_SIZE(xv) && ((void) (before), 1); \
	     (void) (after), ++(i))
#define XV_IEACH(i,v,xv) XV_LOOP(xv, ptrdiff_t, i, (v) = XV_AT(xv,i), 0)
#define XV_IEACHPTR(i,p,xv) XV_LOOP(xv, ptrdiff_t, i, (p) = &XV_AT(xv,i), 0)
#define XV_EACH(v,xv) XV_IEACH(xv__i_##__LINE__,v,xv)
#define XV_EACHPTR(p,xv) XV_IEACHPTR(xv__i_##__LINE__,p,xv)
#define XV_PUSH(xv,x) (XV_RESERVE(xv, XV_SIZE(xv)+1), XV_AT(xv, XV_SIZE(xv)++) = (x))
#define XV_POP(xv) (XV_PTR(xv)[--XV_SIZE(xv)])
#define XV_COPYTO(p,xv,i,n) memcpy((p), XV_PTR(xv)+i, (n)*XV_ITEMSIZE(xv))
#define XV_COPYFROM(xv,i,p,n) ((void) memcpy(XV_PTR(xv)+i, (p), (n)*XV_ITEMSIZE(xv)))
#define XV_COPYPUSH(xv,p,n) \
	(XV_RESERVE(xv, XV_SIZE(xv) + (ptrdiff_t) (n)), \
	 XV_COPYFROM(xv, XV_SIZE(xv), p, n), (void) (XV_SIZE(xv) += (n)))
#define XV_ZEROIZE(xv,i,n) memset(XV_PTR(xv) + (i), 0, (n) * XV_ITEMSIZE(xv))
#define XV_FREE(xv) if (XV_ALLOC(xv) == 0) { } else free(XV_PTR(xv))

static void *xv_do_resize(struct xv_base *base, void *ptr, ptrdiff_t n, ptrdiff_t itemsize)
{
	static const ptrdiff_t GROWLIMIT = 0x7fff;
	if (n <= GROWLIMIT) {
		--n; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; ++n;
		if (n < 4) n = 4;
	} else {
		if (n > (PTRDIFF_MAX & ~GROWLIMIT)) die("memory error"); /* overflow */
		n = (n + GROWLIMIT) & ~GROWLIMIT;
	}
	ptr = realloc(ptr, n * itemsize);
	if (!ptr) die("memory error");
	base->xv__alloc = n;
	return ptr;
}

/******************************************************************************/
/* constants, variables */

static enum { PLAY_MODE, AUTOPLAY_MODE, EXCLUSIVE_MODE } opt_mode = PLAY_MODE;
static int opt_showinfo = 1;
static int opt_fullscreen = 1;
static enum { NO_MODF, MIRROR_MODF, RANDOM_MODF, RANDOMEX_MODF, SHUFFLE_MODF, SHUFFLEEX_MODF } opt_modf = NO_MODF;
static enum { BGA_AND_MOVIE, BGA_BUT_NO_MOVIE, NO_BGA } opt_bga = BGA_AND_MOVIE;
static int opt_joystick = -1;

static char *bmspath, respath[512];

enum { M_TITLE = 0, M_GENRE = 1, M_ARTIST = 2, M_STAGEFILE = 3 };
enum { V_PLAYER = 0, V_PLAYLEVEL = 1, V_RANK = 2, V_LNTYPE = 3, V_LNOBJ = 4 };

#define MAXMETADATA 1023
static char metadata[4][MAXMETADATA+1];
static double bpm = 130;
static int value[] = {[V_PLAYER]=1, [V_PLAYLEVEL]=0, [V_RANK]=2, [V_LNTYPE]=1, [V_LNOBJ]=0};

static char *sndpath[1296], *imgpath[1296];
static XV(struct blitcmd { int dst, src, x1, y1, x2, y2, dx, dy; }) blitcmd = XV_EMPTY;
static struct { Mix_Chunk *res; int ch; } sndres[1296];
static XV(int) sndchmap;
static struct { SDL_Surface *surface; SMPEG *movie; } imgres[1296];
static int stoptab[1296];
static double bpmtab[1296];

#define NOTE_CHANNEL(player, chan) ((player)*9+(chan)-1)
#define IS_NOTE_CHANNEL(c) ((c) < 18)
enum { BGM_CHANNEL = 18, BGA_CHANNEL = 19, BPM_CHANNEL = 20, STOP_CHANNEL = 21 };
enum { LNDONE = 0, LNSTART = 1, NOTE = 2, INVNOTE = 3 }; /* types for NOTE_CHANNEL */
enum { BGA_LAYER = 0, BGA2_LAYER = 1, POORBGA_LAYER = 2 }; /* types for BGA_CHANNEL */
enum { BPM_BY_VALUE = 0, BPM_BY_INDEX = 1 }; /* types for BPM_CHANNEL */
enum { STOP_BY_192ND_OF_MEASURE = 0, STOP_BY_MSEC = 1 }; /* types for STOP_CHANNEL */

typedef struct {
	double time; /* time */
	int type; /* type (see above). 0 if unspecified. <0 if removed somehow. */
	int index; /* associated resource or key value (if any) */
} bmsnote;

static bmsnote *channel[22];
static double _shorten[2005], *shorten = _shorten + 1;
static int nchannel[22];
static double length;
static int has7keys, haslongnote, haspedal, hasbpmchange;
static int nnotes, maxscore, duration;

/******************************************************************************/
/* system dependent functions */

#ifdef WIN32

#include <windows.h>

static const char sep = '\\';

static int filedialog(char *buf)
{
	OPENFILENAME ofn = {
		.lStructSize = sizeof ofn,
		.lpstrFilter =
			"All Be-Music Source File (*.bms;*.bme;*.bml)\0*.bms;*.bme;*.bml\0"
			"Be-Music Source File (*.bms)\0*.bms\0"
			"Extended Be-Music Source File (*.bme)\0*.bme\0"
			"Longnote Be-Music Source File (*.bml)\0*.bml\0"
			"All Files (*.*)\0*.*\0",
		.lpstrFile = buf,
		.nMaxFile = 512,
		.lpstrTitle = "Choose a file to play",
		.Flags = OFN_HIDEREADONLY};
	return GetOpenFileName(&ofn);
}

static void die(const char *msg, ...)
{
	va_list v;
	char b[512];
	va_start(v, msg);
	vsprintf(b, c, v);
	va_end(a);
	MessageBox(0, b, VERSION, 0);
	exit(1);
}

static void dirinit(void) {}
static void dirfinal(void) {}
static const char *adjust_path_case(char *file) { return file; }

#else /* WIN32 */

#include <dirent.h>

static const char sep = '/';
static char *flist[2592];
static int nfiles = 0;

static int filedialog(char *buf)
{
	(void) buf;
	return 0;
}

static void die(const char *msg, ...)
{
	va_list v;
	fprintf(stderr, "%s: ", argv0);
	va_start(v, msg);
	vfprintf(stderr, msg, v);
	va_end(v);
	fprintf(stderr, "\n");
	exit(1);
}

static int stricmp(const char *a, const char *b); /* DUMMY */
static char *strcopy(const char*); /* DUMMY */

static void dirinit(void)
{
	DIR *d;
	struct dirent *e;

	if ((d = opendir(bmspath))) {
		while ((e = readdir(d))) flist[nfiles++] = strcopy(e->d_name);
		closedir(d);
	}
}

static void dirfinal(void)
{
	while (nfiles--) free(flist[nfiles]);
}

static const char *adjust_path_case(char *file)
{
	int i;
	for (i = 0; i < nfiles; ++i) {
		if (stricmp(flist[i], file)) return flist[i];
	}
	return file; /* always nonexistent file */
}
#endif

/******************************************************************************/
/* general functions */

static void warn(const char *msg, ...)
{
	va_list v;
	fprintf(stderr, "*** Warning: ");
	va_start(v, msg);
	vfprintf(stderr, msg, v);
	va_end(v);
	fprintf(stderr, "\n");
}

static int stricmp(const char *a, const char *b)
{
	while (*a && *b && !((*a ^ *b) & ~32)) ++a, ++b;
	return *a == *b;
}

static char *adjust_path(char *path)
{
	int i;
	strcpy(respath, bmspath);
	strcat(respath, adjust_path_case(path)); /* XXX could be overflow */
	for (i = 0; respath[i]; ++i) {
		if (respath[i] == '\\' || respath[i] == '/') respath[i] = sep;
	}
	return respath;
}

static char *adjust_path_with_ext(char *path, char *ext)
{
	int len = strlen(bmspath), i;
	char *oldext;
	strcpy(respath, bmspath);
	strcpy(respath + len, path);
	oldext = strrchr(respath + len, '.');
	if (oldext) {
		strcpy(oldext, ext);
		strcpy(respath + len, adjust_path_case(respath + len));
	}
	for (i = 0; respath[i]; ++i) {
		if (respath[i] == '\\' || respath[i] == '/') respath[i] = sep;
	}
	return respath;
}

static char *strcopy(const char *src)
{
	char *dest = malloc(strlen(src)+1);
	return strcpy(dest, src);
}

/******************************************************************************/
/* bms parser */

static int getdigit(int n)
{
	n |= 32;
	if ('0' <= n && n <= '9') return n - '0';
	if ('a' <= n && n <= 'z') return (n - 'a') + 10;
	return -1296;
}

static int key2index(const char *a)
{
	return getdigit(*a) * 36 + getdigit(a[1]);
}

static int compare_bmsline(const void *a, const void *b)
{
	int i, j;
	for (i = 0; i < 6; ++i) {
		if ((j = (*(char**)a)[i] - (*(char**)b)[i])) return j;
	}
	return (char*)a - (char*)b;
}

static int compare_bmsnote(const void *a, const void *b)
{
	const bmsnote *A = a, *B = b;
	return (A->time > B->time ? 1 : A->time < B->time ? -1 : A->type - B->type);
}

static void add_note(int chan, double time, int type, int index)
{
	bmsnote temp = {time, type, index};
	channel[chan] = realloc(channel[chan], sizeof(bmsnote) * (nchannel[chan]+1));
	channel[chan][nchannel[chan]++] = temp;
}

static void remove_note(int chan, int index)
{
	if (IS_NOTE_CHANNEL(chan) && channel[chan][index].index) {
		add_note(BGM_CHANNEL, channel[chan][index].time, 0, channel[chan][index].index);
	}
	channel[chan][index].type = -1;
}

#define KEY_PATTERN "%2[0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz]"

static int parse_bms(void)
{
	static const char *bmsheader[] = {
		"title", "genre", "artist", "stagefile", "bpm", "player", "playlevel",
		"rank", "lntype", "lnobj", "wav", "bmp", "bga", "stop", "stp", "random",
		"if", "else", "endif", 0};

	FILE *fp;
	int i, j, k, a, b, c;
	int rnd = 1, ignore = 0;
	int measure, chan, prev[18] = {0}, lprev[18] = {0};
	double t;
	char linebuf[4096], buf1[4096], buf2[4096];
	char *line;
	struct blitcmd bc;
	XV(char*) bmsline = XV_EMPTY;

	fp = fopen(bmspath, "r");
	if (!fp) return 1;

	srand(time(0));
	while (fgets(line = linebuf, sizeof linebuf, fp)) {
		while (*line == ' ' || *line == '\t') ++line;
		if (*line++ != '#') continue;

		for (i = 0; bmsheader[i]; ++i) {
			for (j = 0; bmsheader[i][j]; ++j) {
				if ((bmsheader[i][j] ^ line[j]) & ~32) break;
			}
			if (!bmsheader[i][j]) {
				line += j;
				break;
			}
		}
		if (ignore) i = ~i;

		switch (i) {
		case 0: /* title */
		case 1: /* genre */
		case 2: /* artist */
		case 3: /* stagefile */
			sscanf(line, "%*[ ]%" STRINGIFY(MAXMETADATA) "[^\r\n]", metadata[i]);
			break;

		case 4: /* bpm */
			if (sscanf(line, "%*[ ]%lf", &bpm) >= 1) {
				/* do nothing, bpm is set */
			} else if (sscanf(line, KEY_PATTERN "%*[ ]%lf", buf1, &t) >= 2) {
				i = key2index(buf1);
				if (i >= 0) bpmtab[i] = t;
			}
			break;

		case 5: /* player */
		case 6: /* playlevel */
		case 7: /* rank */
		case 8: /* lntype */
			sscanf(line, "%*[ ]%d", &value[i-5]);
			break;

		case 9: /* lnobj */
			if (sscanf(line, "%*[ ]" KEY_PATTERN, buf1) >= 1) {
				value[V_LNOBJ] = key2index(buf1);
			}
			break;

		case 10: /* wav## */
		case 11: /* bmp## */
			if (sscanf(line, KEY_PATTERN "%*[ ]%[^\r\n]", buf1, buf2) >= 2) {
				j = key2index(buf1);
				if (j >= 0) {
					char **path = (i==10 ? sndpath : imgpath);
					free(path[j]);
					path[j] = strcopy(buf2);
				}
			}
			break;

		case 12: /* bga## */
			if (sscanf(line, KEY_PATTERN "%*[ ]" KEY_PATTERN "%*[ ]%d %d %d %d %d %d",
					buf1, buf2, &bc.x1, &bc.y1, &bc.x2, &bc.y2, &bc.dx, &bc.dy) >= 8) {
				bc.dst = key2index(buf1);
				bc.src = key2index(buf2);
				if (bc.dst >= 0 && bc.src >= 0) XV_PUSH(blitcmd, bc);
			}
			break;

		case 13: /* stop## */
			if (sscanf(line, KEY_PATTERN "%*[ ]%d", buf1, &j) >= 2) {
				i = key2index(buf1);
				if (i >= 0) stoptab[i] = j;
			}
			break;

		case 14: /* stp## */
			if (sscanf(line, "%d.%d %d", &i, &j, &k) >= 3) {
				add_note(STOP_CHANNEL, i+j/1e3, STOP_BY_MSEC, k);
			}
			break;

		case 15: case ~15: /* random */
			if (sscanf(line, "%*[ ]%d", &j) >= 1) {
				rnd = rand() % abs(j) + 1;
			}
			break;

		case 16: case ~16: /* if */
			if (sscanf(line, "%*[ ]%d", &j) >= 1) {
				ignore = (rnd != j);
			}
			break;

		case 17: case ~17: /* else */
			ignore = !ignore;
			break;

		case 18: case ~18: /* endif */
			ignore = 0;
			break;

		case 19: /* #####:... */
			/* only check validity, do not store them yet */
			if (sscanf(line, "%*5c:%c", buf1) >= 1) {
				XV_PUSH(bmsline, strcopy(line));
			}
		}
	}
	fclose(fp);

	qsort(XV_PTR(bmsline), XV_SIZE(bmsline), XV_ITEMSIZE(bmsline), compare_bmsline);
	XV_EACH(line, bmsline) {
		j = atoi(line);
		measure = j / 100;
		chan = j % 100;
		if (chan == 2) {
			shorten[measure] = atof(line+6);
		} else {
			j = 6 + strspn(line+6, " \t\r\n");
			a = strcspn(line+j, " \t\r\n") / 2;
			for (k = 0; k < a; ++k, j+=2) {
				b = key2index(line+j);
				t = measure + 1. * k / a;
				if (chan == 1) {
					if (b) add_note(BGM_CHANNEL, t, 0, b);
				} else if (chan == 3) {
					if (b/36<16 && b%36<16) add_note(BPM_CHANNEL, t, BPM_BY_VALUE, b/36*16+b%36);
				} else if (chan == 4) {
					if (b) add_note(BGA_CHANNEL, t, BGA_LAYER, b);
				} else if (chan == 6) {
					if (b) add_note(BGA_CHANNEL, t, POORBGA_LAYER, b);
				} else if (chan == 7) {
					if (b) add_note(BGA_CHANNEL, t, BGA2_LAYER, b);
				} else if (chan == 8) {
					if (b) add_note(BPM_CHANNEL, t, BPM_BY_INDEX, b);
				} else if (chan == 9) {
					if (b) add_note(STOP_CHANNEL, t, STOP_BY_192ND_OF_MEASURE, b);
				} else if (chan >= 10 && chan < 30 && chan % 10 != 0) {
					if (b) {
						c = NOTE_CHANNEL(chan>20, chan%10);
						if (value[V_LNOBJ] && b == value[V_LNOBJ]) {
							if (nchannel[c] && channel[c][nchannel[c]-1].type==NOTE) {
								channel[c][nchannel[c]-1].type = LNSTART;
								add_note(c, t, LNDONE, b);
							}
						} else {
							add_note(c, t, NOTE, b);
						}
					}
				} else if (chan >= 30 && chan < 50 && chan % 10 != 0) {
					if (b) add_note(NOTE_CHANNEL(chan>40, chan%10), t, INVNOTE, b);
				} else if (chan >= 50 && chan < 70 && chan % 10 != 0) {
					c = NOTE_CHANNEL(chan>60, chan%10);
					if (value[V_LNTYPE] == 1 && b) {
						if (prev[c]) {
							prev[c] = 0;
							add_note(c, t, LNDONE, 0);
						} else {
							prev[c] = b;
							add_note(c, t, LNSTART, b);
						}
					} else if (value[V_LNTYPE] == 2) {
						if (prev[c] || prev[c] != b) {
							if (prev[c]) {
								if (lprev[c] + 1 < measure) {
									add_note(c, lprev[c]+1, LNDONE, 0);
								} else if (prev[c] != b) {
									add_note(c, t, LNDONE, 0);
								}
							}
							if (b && (prev[c]!=b || lprev[c]+1<measure)) {
								add_note(c, t, LNSTART, b);
							}
							lprev[c] = measure;
							prev[c] = b;
						}
					}
				}
			}
		}
		free(line);
	}
	XV_FREE(bmsline);

	length = measure + 2;
	for (i = 0; i < 18; ++i) {
		if (prev[i]) {
			if (value[V_LNTYPE] == 2 && lprev[i] + 1 < measure) {
				add_note(i, lprev[i]+1, LNDONE, 0);
			} else {
				add_note(i, length - 1, LNDONE, 0);
			}
		}
	}

	return 0;
}

static int sanitize_bms(void)
{
	int i, j, k, b, c;

	for (i = 0; i < 22; ++i) {
		if (!channel[i]) continue;
		qsort(channel[i], nchannel[i], sizeof(bmsnote), compare_bmsnote);

		if (i != BGM_CHANNEL && i != STOP_CHANNEL) {
			b = 0;
			j = 0;
			while (j < nchannel[i]) {
				k = j;
				c = 0;
				for (k = j; k < nchannel[i] && channel[i][k].time <= channel[i][j].time; ++k) {
					if (c & 1 << channel[i][k].type) {
						remove_note(i, k);
					} else {
						c |= 1 << channel[i][k].type;
					}
				}

				if (b) {
					/* remove starting longnote if there's no ending longnote */
					c &= ~(c & 1 ? 0 : 2);
					/* remove visible note */
					c &= ~4;
					/* remove invisible note if there is any longnote */
					c &= ~(c & 3 ? 8 : 0);

					b = !(c & 1);
				} else {
					/* remove starting longnote if there's also ending longnote */
					c &= ~(c & 1 ? 2 : 0);
					/* remove ending longnote */
					c &= ~1;
					/* keep only one note, in the order of importance */
					c &= -c;

					b = (c & 2);
				}

				for (; j < k; ++j) {
					if (channel[i][j].type < 0) continue;

					if (IS_NOTE_CHANNEL(i) && !(c & 1 << channel[i][j].type)) {
						remove_note(i, j);
					}
				}
			}
			if (IS_NOTE_CHANNEL(i) && b) {
				/* remove last starting longnote which is unfinished */
				while (j >= 0 && channel[i][--j].type < 0);
				if (j >= 0 && channel[i][j].type == LNSTART) remove_note(i, j);
			}
		}
		k = 0;
		for (j = 0; j < nchannel[i]; ++j) {
			if (channel[i][j].type >= 0) {
				channel[i][j-k] = channel[i][j];
			} else {
				++k;
			}
		}
		nchannel[i] -= k;
	}

	for (i = 0; i < 2005; ++i) {
		if (_shorten[i] <= .001)
			_shorten[i] = 1;
	}

	return 0;
}

/* forward declarations to keep things apart */
static SDL_Surface *newsurface(int w, int h);
static void resource_loaded(const char *path);

static int load_resource(void)
{
	SDL_Surface *temp;
	struct blitcmd bc;
	int i;

	for (i = 0; i < 1296; ++i) {
		sndres[i].ch = -1;
		if (sndpath[i]) {
			resource_loaded(sndpath[i]);
			sndres[i].res = Mix_LoadWAV(adjust_path(sndpath[i]));
			if (!sndres[i].res) sndres[i].res = Mix_LoadWAV(adjust_path_with_ext(sndpath[i], ".mp3"));
			if (!sndres[i].res) sndres[i].res = Mix_LoadWAV(adjust_path_with_ext(sndpath[i], ".ogg"));
			if (!sndres[i].res) warn("failed to load sound #%d (%s)", i, sndpath[i]);
			free(sndpath[i]);
			sndpath[i] = 0;
		}
		if (imgpath[i]) {
			char *ext = strrchr(imgpath[i], '.');
			resource_loaded(imgpath[i]);
			if (ext && stricmp(ext, ".mpg") && opt_bga < BGA_BUT_NO_MOVIE) {
				SMPEG *movie = SMPEG_new(adjust_path(imgpath[i]), NULL, 0);
				if (!movie) {
					warn("failed to load image #%d (%s)", i, imgpath[i]);
				} else {
					imgres[i].surface = newsurface(256, 256);
					imgres[i].movie = movie;
					SMPEG_enablevideo(movie, 1);
					SMPEG_loop(movie, 1);
					SMPEG_setdisplay(movie, imgres[i].surface, NULL, NULL);
				}
			} else if (opt_bga < NO_BGA) {
				temp = IMG_Load(adjust_path(imgpath[i]));
				if (!temp) temp = IMG_Load(adjust_path_with_ext(imgpath[i], ".png"));
				if (!temp) temp = IMG_Load(adjust_path_with_ext(imgpath[i], ".jpg"));
				if (temp) {
					imgres[i].surface = SDL_DisplayFormat(temp);
					SDL_FreeSurface(temp);
					SDL_SetColorKey(imgres[i].surface, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
				} else {
					warn("failed to load image #%d (%s)", i, imgpath[i]);
				}
			}
			free(imgpath[i]);
			imgpath[i] = 0;
		}
	}

	XV_EACH(bc, blitcmd) {
		if (imgres[bc.dst].movie || imgres[bc.src].movie || !imgres[bc.src].surface) continue;
		temp = imgres[bc.dst].surface;
		if (!temp) {
			imgres[bc.dst].surface = temp = newsurface(256, 256);
			SDL_FillRect(temp, 0, SDL_MapRGB(temp->format, 0, 0, 0));
			SDL_SetColorKey(temp, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
		}
		if (bc.x1 < 0) bc.x1 = 0;
		if (bc.y1 < 0) bc.y1 = 0;
		if (bc.x2 > bc.x1 + 256) bc.x2 = bc.x1 + 256;
		if (bc.y2 > bc.y1 + 256) bc.y2 = bc.y1 + 256;
		SDL_BlitSurface(imgres[bc.src].surface, R(bc.x1, bc.y1, bc.x2-bc.x1, bc.y2-bc.y1),
			temp, R(bc.dx, bc.dy, 0, 0));
	}
	XV_FREE(blitcmd);

	return 0;
}

static double adjust_object_time(double base, double offset)
{
	int i = (int)(base+1)-1;
	if ((i + 1 - base) * shorten[i] > offset)
		return base + offset / shorten[i];
	offset -= (i + 1 - base) * shorten[i];
	while (shorten[++i] <= offset)
		offset -= shorten[i];
	return i + offset / shorten[i];
}

static double adjust_object_position(double base, double time)
{
	int i = (int)(base+1)-1, j = (int)(time+1)-1;
	base = (time - j) * shorten[j] - (base - i) * shorten[i];
	while (i < j) base += shorten[i++];
	return base;
}

static void get_bms_info(void)
{
	int i, j;

	has7keys = (nchannel[7] || nchannel[8] || nchannel[16] || nchannel[17]);
	haspedal = (nchannel[6] || nchannel[15]);
	hasbpmchange = nchannel[20];
	haslongnote = nnotes = 0;
	for (i = 0; i < 18; ++i) {
		for (j = 0; j < nchannel[i]; ++j) {
			if (channel[i][j].type == LNSTART) {
				haslongnote = 1;
				++nnotes;
			} else if (channel[i][j].type == NOTE) {
				++nnotes;
			}
		}
	}
	for (i = 0; i < nnotes; ++i) {
		maxscore += (int)(300 * (1 + 1. * i / nnotes));
	}
}

static int get_bms_duration(void)
{
	int cur[22] = {0}, i, j;
	double time, rtime, ttime;
	double pos, xbpm, tmp;

	xbpm = bpm;
	time = rtime = 0.0;
	pos = -1;
	while (1) {
		for (i = 0, j = -1; i < 22; ++i) {
			if (cur[i] < nchannel[i] && (j < 0 || channel[i][cur[i]].time < channel[j][cur[j]].time)) j = i;
		}
		if (j < 0) {
			time += MEASURE_TO_MSEC(adjust_object_position(pos, length), xbpm);
			break;
		}
		time += MEASURE_TO_MSEC(adjust_object_position(pos, channel[j][cur[j]].time), xbpm);

		i = channel[j][cur[j]].index;
		ttime = 0.0;
		if (IS_NOTE_CHANNEL(j) || j == BGM_CHANNEL) {
			if (i && sndres[i].res) ttime = sndres[i].res->alen / 176.4;
		} else if (j == BPM_CHANNEL) {
			tmp = (channel[j][cur[j]].type == BPM_BY_INDEX ? bpmtab[i] : i);
			if (tmp > 0) {
				xbpm = tmp;
			} else if (tmp < 0) {
				time += MEASURE_TO_MSEC(adjust_object_position(-1, pos), -tmp);
				break;
			}
		} else if (j == STOP_CHANNEL) {
			if (channel[j][cur[j]].type == STOP_BY_MSEC) {
				time += i;
			} else { /* STOP_BY_192ND_OF_MEASURE */
				time += MEASURE_TO_MSEC(stoptab[i] / 192.0, xbpm);
			}
		}
		if (rtime < time + ttime) rtime = time + ttime;
		pos = channel[j][cur[j]].time;
		++cur[j];
	}
	return (int) (time > rtime ? time : rtime);
}

static void shuffle_bms(int mode, int player)
{
	bmsnote *tempchan, *result[18] = {0};
	int nresult[18] = {0};
	int map[18], perm[18], nmap;
	int ptr[18] = {0,}, thru[18] = {0}, target[18];
	int thrumap[18], thrurmap[18];
	int i, j, flag = 1, temp;
	double t;

	srand(time(0));
	for (i = 0; i < 18; ++i) map[i] = perm[i] = i;
	if (!nchannel[7] && !nchannel[8] && !nchannel[16] && !nchannel[17]) {
		map[7] = map[8] = map[16] = map[17] = -1;
	}
	if (!nchannel[6] && !nchannel[15]) {
		map[6] = map[15] = -1;
	}
	if (mode != SHUFFLEEX_MODF && mode != RANDOMEX_MODF) {
		map[5] = map[6] = map[14] = map[15] = -1;
	}
	if (player) {
		for (i = 0; i < 9; ++i) map[player==1 ? i+9 : i] = -1;
	}
	for (i = j = 0; i < 18; ++i) {
		if (map[i] < 0) {
			++j;
		} else {
			map[i-j] = map[i];
		}
	}
	nmap = 18 - j;

#define SWAP(x,y,t) (t)=(x);(x)=(y);(y)=(t);

	if (mode <= MIRROR_MODF) { /* mirror */
		for (i = 0, j = nmap-1; i < j; ++i, --j) {
			SWAP(channel[map[i]], channel[map[j]], tempchan);
			SWAP(nchannel[map[i]], nchannel[map[j]], temp);
		}
	} else if (mode <= SHUFFLEEX_MODF) { /* shuffle */
		for (i = nmap-1; i > 0; --i) {
			j = rand() % i;
			SWAP(channel[map[i]], channel[map[j]], tempchan);
			SWAP(nchannel[map[i]], nchannel[map[j]], temp);
		}
	} else if (mode <= RANDOMEX_MODF) { /* random */
		while (flag) {
			for (i = nmap-1; i > 0; --i) {
				j = rand() % i;
				SWAP(perm[i], perm[j], temp);
			}
			t = 1e4;
			flag = 0;
			for (i = 0; i < nmap; ++i) {
				if (ptr[map[i]] < nchannel[map[i]]) {
					flag = 1;
					target[i] = 1;
					if (t > channel[map[i]][ptr[map[i]]].time)
						t = channel[map[i]][ptr[map[i]]].time - 1e-4;
				} else {
					target[i] = 0;
				}
			}
			t += 2e-4;
			for (i = 0; i < nmap; ++i) {
				if (target[i] && channel[map[i]][ptr[map[i]]].time > t)
					target[i] = 0;
			}
			for (i = 0; i < nmap; ++i) {
				if (!target[i]) continue;
				temp = channel[map[i]][ptr[map[i]]].type;
				if (temp == LNDONE) {
					j = thrumap[i];
					thru[j] = 2;
				} else {
					j = perm[i];
					while (thru[j]) j = perm[thrurmap[j]];
					if (temp == LNSTART) {
						thrumap[i] = j;
						thrurmap[j] = i;
						thru[j] = 1;
					}
				}
				result[j] = realloc(result[j], sizeof(bmsnote) * (nresult[j]+1));
				result[j][nresult[j]++] = channel[map[i]][ptr[map[i]]++];
			}
			for(i = 0; i < nmap; ++i) {
				if (thru[i] == 2) thru[i] = 0;
			}
		}
		for (i = 0; i < nmap; ++i) {
			free(channel[map[i]]);
			channel[map[i]] = result[i];
			nchannel[map[i]] = nresult[i];
		}
	}
}

/******************************************************************************/
/* general graphic functions */

static SDL_Surface *screen;
static SDL_Event event;

static int getpixel(SDL_Surface *s, int x, int y)
{
	Uint8 r, g, b;
	SDL_GetRGB(((Uint32*)s->pixels)[x+y*s->pitch/4], s->format, &r, &g, &b);
	return (int)(r << 16) | ((int)g << 8) | (int)b;
}

static Uint32 map(int c)
{
	return SDL_MapRGB(screen->format, c >> 16, (c >> 8) & 255, c & 255);
}

static int putpixel(SDL_Surface *s, int x, int y, int c)
{
	c = SDL_MapRGB(s->format, c >> 16, (c >> 8) & 255, c & 255);
	return ((Uint32*)s->pixels)[x+y*s->pitch/4] = c;
}

static int blend(int x, int y, int a, int b)
{
	int i = 0;
	for (; i < 24; i += 8)
		y += ((x>>i&255) - (y>>i&255))*a/b << i;
	return y;
}

static void putblendedpixel(SDL_Surface *s, int x, int y, int c, int o)
{
	putpixel(s, x, y, blend(getpixel(s,x,y), c, o, 255));
}

static SDL_Surface *newsurface(int w, int h)
{
	return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0xff0000, 0xff00, 0xff, 0);
}

static int bicubic_kernel(int x, int y) {
	if (x < 0) x = -x;
	if (x < y) {
		return ((y*y - 2*x*x + x*x/y*x) << 11) / y / y;
	} else if (x < y * 2) {
		return ((4*y*y - 8*x*y + 5*x*x - x*x/y*x) << 11) / y / y;
	} else {
		return 0;
	}
}

static int bicubic_interpolation(SDL_Surface *src, SDL_Surface *dest) {
	int x, dx, y, dy, ww, hh, w, h;
	int i, j, k, r, g, b, c, xx, yy, a[4][2], d;

	dx = x = 0;
	ww = src->w - 1;
	hh = src->h - 1;
	w = dest->w - 1;
	h = dest->h - 1;
	for (i = 0; i <= w; ++i) {
		dy = y = 0;
		for (j = 0; j <= h; ++j) {
			r = g = b = 0;
			for (k = 0; k < 4; ++k) {
				a[k][0] = bicubic_kernel((x+k-1)*w - i*ww, w);
				a[k][1] = bicubic_kernel((y+k-1)*h - j*hh, h);
			}
			for (k = 0; k < 16; ++k) {
				xx = x + k/4 - 1;
				yy = y + k%4 - 1;
				if (xx>=0 && xx<=ww && yy>=0 && yy<=hh) {
					c = getpixel(src, xx, yy);
					d = a[k/4][0] * a[k%4][1] >> 6;
					r += (c>>16) * d;
					g += (c>>8&255) * d;
					b += (c&255) * d;
				}
			}
			r = (r<0 ? 0 : r>>24 ? 255 : r>>16);
			g = (g<0 ? 0 : g>>24 ? 255 : g>>16);
			b = (b<0 ? 0 : b>>24 ? 255 : b>>16);
			putpixel(dest, i, j, (r<<16) | (g<<8) | b);

			dy += hh;
			if (dy > h) {
				++y;
				dy -= h;
			}
		}
		dx += ww;
		if (dx > w) {
			++x;
			dx -= w;
		}
	}

	return 0;
}

/******************************************************************************/
/* font functions */

static Uint16 fontdata[3072];
static Uint8 (*zoomfont[16])[96] = {NULL};

static void fontdecompress(void)
{
	/* delta coded words */
	static int words[] = {0, 0, 2, 6, 2, 5, 32, 96, 97, 15, 497, 15,
		1521, 15, 1537, 16, 48, 176, 1, 3, 1, 3, 7, 1, 4080, 4096, 3, 1, 8,
		3, 4097, 4080, 16, 16128, 240, 1, 2, 9, 3, 8177, 15, 16385, 240, 15,
		1, 47, 721, 143, 2673, 2, 6, 7, 1, 31, 17, 16, 63, 64, 33, 0, 1, 2,
		1, 8, 3};
	/* LZ77-compressed indices to words */
	static const char *indices =
		"!!7a/&w7a!!g'M*Qg(O&J!!&Je!!g2Qg-B!!u2cQ[b7b2[[Q7b2[bQ!c!i&d>UT2c&"
		"b>WT!b2eGW&!c!i2MbUbD!.8(M$UQCb-b!!l'U*2eTXb2Gb5b>9ZW!!k*e8(J$b!!u"
		"*8(M$Q!h#b'O*?!!k#Q'O*?b!h8(M$Q!!m&JTLcG_&J>]TLc&J!!o&Je7[&Je!!{&J"
		"dM$Q!!s7[!!}e&Je!!m0b3b.8(M$U!Cb-b!o>UQ2Ub]ba9Y[SbQCb2GW!!k*MbQbBb"
		"!kaQ!!k>UQ2Ub]bG8.M(U$[!Ca[!!k>UQ2d!IJQ!d2cGW!!k78bM6U2Cb2ea[2!c!l"
		"a[!2e>[Q!d2cGW!!k>UQ2c!b>[Q2gGW!!ka[Q!UbDb.8(J&h!!k>UQ2eIXQ2gGW!!k"
		">UQ2gaWQ!b2cGW!!m&Je!!e&Je!!m&Je!!e&JdM$Q!!k3b.8(M$U!H#W'O,?4!!t7["
		"!!c7[!!r:#W'O,?5!.8(M$U!Cb!k>UQ2cUbD!.8(J!!&Jc!!k>UJ)LKKhGb!)7W!!k"
		"'8,M=UWCQ2ca[Q2e!!k>[Q2eI[Q2gG[!!k>MSUQC!2gQ:RWGO!!k,[<2WbQhUbDb.["
		"!!ka[!2e7[!2ga[!!ka[!2e7[!2j!k>MSUQC!2c[bWbQ:RWGO!!kQ2ga[Q2i!!k7[&"
		"Jo7[!!kQm2eGW!!kU2Db.9([$b#b'b,@<2Wb!!l2qa[!!kU:^[adT2cQh!!kQ2d:R["
		"Vba@_2WbQd!!k,M=UWCQ2gU:EW.O!!k>[Q2gG[!2h!k>UQ2iVbabIW_!Wb!j>[Q2gI"
		"[Q2g!!k>UQ2c!b>WQ!d2cGW!!k7[&Jq!!kQ2qGW!!kQ2kU:EW.O(?!!kQ2gTfa[CW2"
		"Q!!kM+U:^[GW.O,M>U`[WCO-!!kM'U,D<.W(O&Ji!!ka[Q!Ub]bG8.M(U$[!Ca[!!k"
		"*Q!p*b!!n+b:#W'O,?4!1b!p*Qb!pQ!!g'8,M=UWCO-!!}oa[!!i#Q'O,?4!!}b>QQ"
		"!caUQ2eaW!!l2e>[Q2iG[!!o>UQ2c!dQdGW!!kQfaUQ2iaW!!o>UQ2ea[!2UbGW!!k"
		">8TJc&b7[&Ji!!oaUQ2gaWQ!b2cGW!!f2e>[Q2m!!k&Jc!!&Jm!!kQd!dQi2eGW!!h"
		"2cUbDb.9(['b,@<2Wb!!k*Jb?b!o!p;[abT2gQd!!o>[Q2m!!o>UQ2kGW!!o>[Q2kG"
		"[!2f!iaUQ2kaWQ!e!j>[Q2c!k!o>UW2!b>WQ!d:GW!!k&Jc7[&JeTfG?!!oQ2mGW!!"
		"oQ2gU:EW.O(?!!oQ2eTfa[FW!!oM+U:EW.O,M=UWCO-!!oQ2f:RWGO.A(M$U!Cb!ka"
		"[]!G8.M(U$[!Ca[!!i78&Jg%[&Jg7?!!e&J}c!!c#[&Jg7A&Jg$[!!k#]NaaPG?!!}"
		"o7[.W(O";

	int i, ch = 0;
	for (i = 0; i < (int) (sizeof(words) / sizeof(int)); ++i) {
		ch += words[i];
		words[i] = ch;
	}
	for (i = 0; (ch = *indices++); ) {
		if (ch > 97) {
			while (ch-- > 97) fontdata[i] = fontdata[i-2], ++i;
		} else if (ch > 32) {
			fontdata[i++] = words[ch - 33];
		}
	}
}

static int _fontprocess(int x, int y, int z, int c, int s)
{
	int i, j;
	for (i = 0; i < z; ++i) {
		for (j = (s==1 ? z-i : s==3 ? i+1 : 0); j < (s==2 ? i : s==4 ? z-i-1 : z); ++j)
			zoomfont[z][(y*z+i)*z+j][c] |= 1<<(7-x);
	}
	return z;
}

static void fontprocess(int z)
{
	int i, j, k, l, v;

	zoomfont[z] = calloc(16*z*z, 96);
	for (i = l = 0; i < 96; ++i) {
		for (j = 0; j < 16; ++j, l+=2) {
			v = fontdata[l] << 16 | fontdata[l+1];
			for (k = 0; k < 8; ++k, v >>= 4) {
				if ((v & 15) == 15) _fontprocess(k, j, z, i, 0); /* XXX? */
				if (v & 1) _fontprocess(k, j, z, i, 1); /* /| */
				if (v & 2) _fontprocess(k, j, z, i, 2); /* |\ */
				if (v & 4) _fontprocess(k, j, z, i, 3); /* \| */
				if (v & 8) _fontprocess(k, j, z, i, 4); /* |/ */
			}
		}
	}
}

static int printchar(SDL_Surface *s, int x, int y, int z, int c, int u, int v)
{
	int i, j;

	if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
		c -= (c<0 ? -96 : c<33 || c>126 ? c : 32);
		for (i = 0; i < 16*z; ++i) {
			for (j = 0; j < 8*z; ++j) {
				if (zoomfont[z][i*z+j%z][c] & (1<<(7-j/z)))
					putpixel(s, x+j, y+i, blend(u, v, i, 16*z-1));
			}
		}
	}

	return 8*z;
}

static void printstr(SDL_Surface *s, int x, int y, int z, const char *c, int u, int v)
{
	while (*c) {
		x += printchar(s, x, y, z, (Uint8)*c++, u, v);
	}
}

/******************************************************************************/
/* main routines */

static double playspeed = 1, targetspeed;
static int now, origintime, starttime, stoptime = 0, adjustspeed = 0;
static double startoffset = -1, startshorten = 1;
static int pcur[22], pfront[22], prear[22], pcheck[18], thru[22];
static int bga[3] = {-1,-1,0}, poorbga = 0;
static int score = 0, scocnt[5], scombo = 0, smaxcombo = 0;
static double gradefactor;
static int gradetime = 0, grademode, gauge = 256, survival = 150;

static SDL_Surface *sprite=0;
static int keymap[SDLK_LAST]; /* -1: none, 0..17: notes, 18..19: speed down/up */
#define MAXJOYB 20
#define MAXJOYA 4
static int joybmap[MAXJOYB]; /* arbitrary limitation but should be sufficient */
static int joyamap[MAXJOYA];
static int keypressed[2][18]; /* keypressed[0] for buttons, keypressed[1] for axes */
static int tkeyleft[18] = {41,67,93,119,145,0,223,171,197, 578,604,630,656,682,760,537,708,734};
static int tkeywidth[18] = {25,25,25,25,25,40,40,25,25, 25,25,25,25,25,40,40,25,25};
static int tpanel1 = 264, tpanel2 = 536, tbgax = 0, tbgay = 0;
#define WHITE 0x808080
#define BLUE 0x8080ff
static int tkeycolor[18] = {
	WHITE, BLUE, WHITE, BLUE, WHITE, 0xff8080, 0x80ff80, BLUE, WHITE,
	WHITE, BLUE, WHITE, BLUE, WHITE, 0xff8080, 0x80ff80, BLUE, WHITE};
#undef WHITE
#undef BLUE
static char *tgradestr[5] = {"MISS", "BAD", "GOOD", "GREAT", "COOL"};
static int tgradecolor[5][2] = {
	{0xff4040, 0xffc0c0}, {0xff40ff, 0xffc0ff}, {0xffff40, 0xffffc0},
	{0x40ff40, 0xc0ffc0}, {0x4040ff, 0xc0c0ff}};

static Mix_Chunk *beep;
static SDL_Joystick *joystick;

static SDLKey get_sdlkey_from_name(const char *str)
{
	SDLKey i;

	/* XXX maybe won't work in SDL 1.3 */
	for (i = SDLK_FIRST; i < SDLK_LAST; ++i) {
		if (stricmp(SDL_GetKeyName(i), str)) return i;
	}
	return SDLK_UNKNOWN;
}

static void read_keymap(void)
{
	static const struct { const char *name, *def; int base, limit; } envvars[] = {
		{"ANGOLMOIS_1P_KEYS", "z%button 3|s%button 6|x%button 2|d%button 7|c%button 1|"
		                      "left shift%axis 3|left alt|f%button 4|v%axis 2", 0, 9},
		{"ANGOLMOIS_2P_KEYS", "m|k|,|l|.|right shift|right alt|;|/", 9, 18},
		{"ANGOLMOIS_SPEED_KEYS", "f3|f4", 18, 20},
	};
	char *s, buf[256] = "";
	int i, j, k, l, sep;
	SDLKey key;

	for (i = 0; i < SDLK_LAST; ++i) keymap[i] = -1;
	for (i = 0; i < MAXJOYB; ++i) joybmap[i] = -1;
	for (i = 0; i < MAXJOYA; ++i) joyamap[i] = -1;

	for (i = 0; i < 3; ++i) {
		s = getenv(envvars[i].name);
		strncpy(buf, s ? s : envvars[i].def, sizeof(buf) - 1);
		s = buf;
		sep = 1;
		for (j = envvars[i].base; sep && j < envvars[i].limit; ) {
			k = strcspn(s, "%|"); /* both character is not used as a key name in SDL 1.2 */
			sep = s[k];
			s[k++] = '\0';
			key = get_sdlkey_from_name(s);
			if (key != SDLK_UNKNOWN) {
				keymap[key] = j;
			} else if (sscanf(s, "button %d", &l) >= 1 && l >= 0 && l < MAXJOYB) {
				joybmap[l] = j;
			} else if (sscanf(s, "axis %d", &l) >= 1 && l >= 0 && l < MAXJOYA) {
				joyamap[l] = j;
			} else {
				die("Unknown key name in the environment variable %s: %s", envvars[i].name, s);
			}
			if (sep != '%') ++j;
			s += k;
		}
	}
	
}

static void create_beep(void)
{
	Sint32 samples[12000]; /* approx. 0.14 seconds */
	int i;

	for (i = 0; i < 12000; ++i) {
		/* sawtooth wave at 3150 Hz, quadratic decay after 0.02 seconds. */
		samples[i] = (i%28-14)*(i<2000 ? 2000 : (12000-i)*(12000-i)/50000);
	}
	beep = Mix_QuickLoad_RAW((Uint8*)samples, sizeof samples);
}

static void init_video(void)
{
	int mode = (opt_fullscreen ? SDL_FULLSCREEN : SDL_SWSURFACE|SDL_DOUBLEBUF);
	if (opt_mode < EXCLUSIVE_MODE) {
		screen = SDL_SetVideoMode(800, 600, 32, mode);
	} else {
		screen = SDL_SetVideoMode(256, 256, 32, mode);
	}
	if (!screen) die("SDL Video Initialization Failure: %s", SDL_GetError());
	if (opt_mode < EXCLUSIVE_MODE) SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption(VERSION, 0);
}

static int initialize(void)
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0)
		die("SDL Initialization Failure: %s", SDL_GetError());
	atexit(SDL_Quit);
	if (opt_joystick >= 0) {
		SDL_JoystickEventState(SDL_ENABLE);
		joystick = SDL_JoystickOpen(opt_joystick);
		if (!joystick) die("SDL Joystick Initialization Failure: %s", SDL_GetError());
	}
	IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
	Mix_Init(MIX_INIT_OGG|MIX_INIT_MP3);
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)<0)
		die("SDL Mixer Initialization Failure: %s", Mix_GetError());

	if (opt_mode < EXCLUSIVE_MODE) init_video();

	fontdecompress();
	fontprocess(1);
	fontprocess(2);
	fontprocess(3);
	read_keymap(); /* this is unfortunate, but get_sdlkey_from_name depends on SDL_Init. */
	create_beep();
	return 0;
}

static void allocate_more_channels(int n)
{
	int i = XV_SIZE(sndchmap);
	n = Mix_AllocateChannels(Mix_AllocateChannels(-1) + n);
	XV_RESIZE(sndchmap, n);
	while (i < n) XV_AT(sndchmap,i++) = -1;
}

static void play_sound_finished(int ch)
{
	if (XV_AT(sndchmap,ch) >= 0 && sndres[XV_AT(sndchmap,ch)].ch == ch) {
		sndres[XV_AT(sndchmap,ch)].ch = -1;
	}
	XV_AT(sndchmap,ch) = -1;
}

static void play_sound(int i, int group)
{
	int ch;
	if (!sndres[i].res) return;
	while ((ch = Mix_PlayChannel(sndres[i].ch, sndres[i].res, 0)) < 0) {
		allocate_more_channels(32);
	}
	Mix_Volume(ch, group ? 96 : 128);
	Mix_GroupChannel(ch, group);
	sndres[i].ch = ch;
	XV_AT(sndchmap,ch) = i;
}

static SDL_Surface *stagefile_tmp;
static int lastinfo;

static void check_exit(void)
{
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)) {
			if (opt_mode >= EXCLUSIVE_MODE) fprintf(stderr, "\r%72s\r", "");
			exit(0);
		}
	}
}

static void resource_loaded(const char *path)
{
	int now = SDL_GetTicks();
	if (opt_showinfo && now - lastinfo >= INFO_INTERVAL) {
		lastinfo = now;
		if (opt_mode < EXCLUSIVE_MODE) {
			if (!path) path = "loading...";
			SDL_BlitSurface(stagefile_tmp, R(0,0,800,20), screen, R(0,580,800,20));
			printstr(screen, 797-8*strlen(path), 582, 1, path, 0x808080, 0xc0c0c0);
			SDL_Flip(screen);
		} else {
			if (path) {
				fprintf(stderr, "\r%72s\rLoading: %.63s", "", path);
			} else {
				fprintf(stderr, "\r%72s\rLoading done.", "");
			}
		}
		check_exit();
	}
}

static void play_show_stagefile(void)
{
	SDL_Surface *temp, *stagefile;
	char buf[256];
	int i, j, t;

	t = sprintf(buf, "Level %d | BPM %.2f%s | %d note%s [%dKEY%s]",
		value[V_PLAYLEVEL], bpm, "?"+(hasbpmchange==0), nnotes,
		"s"+(nnotes==1), has7keys ? 7 : 5, haslongnote ? "-LN" : "");

	if (opt_mode < EXCLUSIVE_MODE) {
		/*
		char ibuf[256];
		sprintf(ibuf, "%s: %s - %s", VERSION, metadata[M_ARTIST], metadata[M_TITLE]);
		SDL_WM_SetCaption(ibuf, 0);
		*/
		printstr(screen, 248, 284, 2, "loading bms file...", 0x202020, 0x808080);
		SDL_Flip(screen);

		stagefile_tmp = newsurface(800, 20);
		if (*metadata[M_STAGEFILE] && (temp = IMG_Load(adjust_path(metadata[M_STAGEFILE])))) {
			stagefile = SDL_DisplayFormat(temp);
			bicubic_interpolation(stagefile, screen);
			SDL_FreeSurface(temp);
			SDL_FreeSurface(stagefile);
		}
		if (opt_showinfo) {
			for (i = 0; i < 800; ++i) {
				for (j = 0; j < 42; ++j) putblendedpixel(screen, i, j, 0x101010, 64);
				for (j = 580; j < 600; ++j) putblendedpixel(screen, i, j, 0x101010, 64);
			}
			printstr(screen, 6, 4, 2, metadata[M_TITLE], 0x808080, 0xffffff);
			printstr(screen, 792-8*strlen(metadata[M_GENRE]), 4, 1, metadata[M_GENRE], 0x808080, 0xffffff);
			printstr(screen, 792-8*strlen(metadata[M_ARTIST]), 20, 1, metadata[M_ARTIST], 0x808080, 0xffffff);
			printstr(screen, 3, 582, 1, buf, 0x808080, 0xffffff);
			SDL_BlitSurface(screen, R(0,580,800,20), stagefile_tmp, R(0,0,800,20));
		}
		SDL_Flip(screen);
	} else if (opt_showinfo) {
		fprintf(stderr,
				"------------------------------------------------------------------------\n"
				"Title:    %s\nGenre:    %s\nArtist:   %s\n%s\n"
				"------------------------------------------------------------------------\n",
				metadata[M_TITLE], metadata[M_GENRE], metadata[M_ARTIST], buf);
	}

	t = SDL_GetTicks() + 3000;
	lastinfo = -1000;
	load_resource();
	if (opt_showinfo) {
		lastinfo = -1000; /* force update */
		resource_loaded(0);
		SDL_FreeSurface(stagefile_tmp);
	}
	while ((int)SDL_GetTicks() < t) check_exit();
	if (opt_mode < EXCLUSIVE_MODE && opt_bga != NO_BGA) init_video();
}

static void play_prepare(void)
{
	int i, j, c;

	/* configuration */
	origintime = starttime = SDL_GetTicks();
	targetspeed = playspeed;
	allocate_more_channels(64);
	Mix_ReserveChannels(1); /* so that the beep won't be affected */
	Mix_ChannelFinished(&play_sound_finished);
	gradefactor = 1.5 - value[V_RANK] * .25;

	if (opt_mode >= EXCLUSIVE_MODE) return;

	/* panel position */
	if (haspedal == 0) {
		tkeyleft[6] = tkeyleft[15] = -1;
		tpanel1 -= tkeywidth[6] + 1;
		tpanel2 += tkeywidth[15] + 1;
	}
	if (has7keys == 0) {
		tkeyleft[7] = tkeyleft[8] = tkeyleft[16] = tkeyleft[17] = -1;
		for (i = 9; i < 16; ++i) {
			if (i != 14 && tkeyleft[i] >= 0)
				tkeyleft[i] += tkeywidth[16] + tkeywidth[17] + 2;
		}
		if (tkeyleft[6] > 0)
			tkeyleft[6] -= tkeywidth[7] + tkeywidth[8] + 2;
		tpanel1 -= tkeywidth[7] + tkeywidth[8] + 2;
		tpanel2 += tkeywidth[16] + tkeywidth[17] + 2;
	}
	if (value[V_PLAYER] == 1) {
		for (i = 9; i < 18; ++i) tkeyleft[i] = -1;
	} else if (value[V_PLAYER] == 3) {
		for (i = 9; i < 18; ++i) tkeyleft[i] += tpanel1 - tpanel2;
		tpanel1 += 801 - tpanel2;
	}
	if (value[V_PLAYER] % 2) {
		tpanel2 = 0;
		tbgax = tpanel1 / 2 + 282;
	} else {
		tbgax = 272;
	}
	tbgay = 172;

	/* sprite */
	sprite = newsurface(1200, 600);
	for (i = 0; i < 18; ++i) {
		if (tkeyleft[i] < 0) continue;
		for (j = 140; j < 520; ++j)
			SDL_FillRect(sprite, R(tkeyleft[i],j,tkeywidth[i],1), blend(tkeycolor[i], 0, j-140, 1000));
		if (i < 9) {
			for (j = 0; j*2 < tkeywidth[i]; ++j)
				SDL_FillRect(sprite, R(800+tkeyleft[i]+j,0,tkeywidth[i]-2*j,600), blend(tkeycolor[i], 0xffffff, tkeywidth[i]-j, tkeywidth[i]));
		}
	}
	for (j = -244; j < 556; ++j) {
		for (i = -10; i < 20; ++i) {
			c = (i*2+j*3+750) % 2000;
			c = blend(0xc0c0c0, 0x606060, c>1000 ? 1850-c : c-150, 700);
			putpixel(sprite, j+244, i+10, c);
		}
		for (i = -20; i < 60; ++i) {
			c = (i*3+j*2+750) % 2000;
			c = blend(0xc0c0c0, 0x404040, c>1000 ? 1850-c : c-150, 700);
			putpixel(sprite, j+244, i+540, c);
		}
	}
	SDL_FillRect(sprite, R(tpanel1+20,0,(tpanel2?tpanel2:820)-tpanel1-40,600), 0);
	for (i = 0; i < 20; ++i) {
		for (j = 20; j*j+i*i > 400; --j) {
			putpixel(sprite, tpanel1+j, i+10, 0);
			putpixel(sprite, tpanel1+j, 539-i, 0);
			if (tpanel2) {
				putpixel(sprite, tpanel2-j-1, i+10, 0);
				putpixel(sprite, tpanel2-j-1, 539-i, 0);
			}
		}
	}
	if (!tpanel2 && !opt_mode) {
		SDL_FillRect(sprite, R(0,584,368,16), 0x404040);
		SDL_FillRect(sprite, R(4,588,360,8), 0);
	}
	SDL_FillRect(sprite, R(10,564,tpanel1,1), 0x404040);

	/* screen */
	SDL_FillRect(screen, 0, map(0));
	SDL_BlitSurface(sprite, R(0,0,800,30), screen, R(0,0,0,0));
	SDL_BlitSurface(sprite, R(0,520,800,80), screen, R(0,520,0,0));
}

static void update_grade(int grade, int delta)
{
	++scocnt[grade];
	grademode = grade;
	gradetime = now + 700;
	score += (int)(delta * (1 + 1. * scombo / nnotes));

	if (grade < 1) {
		scombo = 0;
		gauge -= 30;
		poorbga = now + 600;
	} else if (grade < 2) {
		scombo = 0;
		gauge -= 15;
	} else if (grade < 3) {
		/* do nothing */
	} else {
		++scombo;
		gauge += (grade<4 ? 2 : 3) + (scombo<100 ? scombo : 100) / 50;
	}

	if (scombo > smaxcombo) smaxcombo = scombo;
}

static int play_process(void)
{
	int i, j, k, l, ibottom, eop;
	double bottom, top, line, tmp;
	char buf[99];

	if (adjustspeed) {
		tmp = targetspeed - playspeed;
		if (-0.001 < tmp && tmp < 0.001) {
			adjustspeed = 0;
			playspeed = targetspeed;
			for (i = 0; i < 22; ++i) prear[i] = pfront[i];
		} else {
			playspeed += tmp * 0.1;
		}
	}

	now = SDL_GetTicks();
	if (now < stoptime) {
		bottom = startoffset;
	} else if (stoptime) {
		starttime = now;
		stoptime = 0;
		bottom = startoffset;
	} else {
		bottom = startoffset + MSEC_TO_MEASURE(now - starttime, bpm) / startshorten;
	}
	ibottom = (int)(bottom + 1) - 1;
	if (bottom > -1 && startshorten != shorten[ibottom]) {
		starttime += (int) (MEASURE_TO_MSEC(ibottom - startoffset, bpm) * startshorten);
		startoffset = ibottom;
		startshorten = shorten[ibottom];
	}
	line = bottom;/*adjust_object_time(bottom, 0.03/playspeed);*/
	top = adjust_object_time(bottom, 1.25/playspeed);
	eop = 1;
	for (i = 0; i < 22; ++i) {
		while (pfront[i] < nchannel[i] && channel[i][pfront[i]].time < bottom) ++pfront[i];
		while (prear[i] < nchannel[i] && channel[i][prear[i]].time <= top) ++prear[i];
		while (pcur[i] < nchannel[i] && channel[i][pcur[i]].time < line) {
			j = channel[i][pcur[i]].index;
			k = channel[i][pcur[i]].type;
			if (i == BGM_CHANNEL) {
				if (j) play_sound(j, 1);
			} else if (i == BGA_CHANNEL) {
				if (bga[k] >= 0 && imgres[bga[k]].movie) {
					SMPEG_stop(imgres[bga[k]].movie);
				}
				bga[k] = j;
				if (j >= 0 && imgres[j].movie) {
					SMPEG_rewind(imgres[j].movie);
					SMPEG_play(imgres[j].movie);
				}
			} else if (i == BPM_CHANNEL) {
				if ((tmp = (k == BPM_BY_INDEX ? bpmtab[j] : j))) {
					starttime = now;
					startoffset = bottom;
					bpm = tmp;
				}
			} else if (i == STOP_CHANNEL) {
				if (now >= stoptime) stoptime = now;
				if (k == STOP_BY_MSEC) {
					stoptime += j;
				} else { /* STOP_BY_192ND_OF_MEASURE */
					stoptime += (int) MEASURE_TO_MSEC(stoptab[j] / 192.0, bpm);
				}
				startoffset = bottom;
			} else if (opt_mode && k != INVNOTE) {
				if (j) play_sound(j, 0);
				if (k != LNDONE) update_grade(4, 300);
			}
			++pcur[i];
		}
		if (i<18 && !opt_mode) {
			for (; pcheck[i] < pcur[i]; ++pcheck[i]) {
				j = channel[i][pcheck[i]].type;
				if (j < 0 || j == INVNOTE || (j == LNDONE && !thru[i])) continue;
				tmp = channel[i][pcheck[i]].time;
				tmp = MEASURE_TO_MSEC(line - tmp, bpm) * shorten[(int)tmp] * gradefactor;
				if (tmp > 144) update_grade(0, 0); else break;
			}
			if (pfront[i] < nchannel[i]) eop = 0;
		}
	}

	while (SDL_PollEvent(&event)) {
		j = 0;
		k = -1;
		switch (event.type) {
		case SDL_QUIT:
			return (eop ? 0 : -1);
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_ESCAPE) return (eop ? 0 : -1);
			i = (event.type == SDL_KEYDOWN);
			k = keymap[event.key.keysym.sym];
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			i = (event.type == SDL_JOYBUTTONDOWN);
			k = joybmap[event.jbutton.button];
			break;
		case SDL_JOYAXISMOTION:
			i = (event.jaxis.value < -3200 ? -1 : event.jaxis.value > 3200 ? 1 : 0);
			j = 1;
			k = joyamap[event.jaxis.axis];
			break;
		}

		if (opt_mode >= EXCLUSIVE_MODE) continue;

		if (i && k == 18/*SPEED_DOWN*/) {
			if (targetspeed > 20) targetspeed -= 5;
			else if (targetspeed > 10) targetspeed -= 1;
			else if (targetspeed > 1) targetspeed -= .5;
			else if (targetspeed > .201) targetspeed -= .2;
			else continue;
			adjustspeed = 1;
			Mix_PlayChannel(0, beep, 0);
		}

		if (i && k == 19/*SPEED_UP*/) {
			if (targetspeed < 1) targetspeed += .2;
			else if (targetspeed < 10) targetspeed += .5;
			else if (targetspeed < 20) targetspeed += 1;
			else if (targetspeed < 95) targetspeed += 5;
			else continue;
			adjustspeed = 1;
			Mix_PlayChannel(0, beep, 0);
		}

		if (opt_mode || k < 0 || k >= 18 || tkeyleft[k] < 0) continue;

		if (!i || (j && i != keypressed[j][k])) { /* up, or down with the reverse direction */
			l = 1;
			if (j) {
				keypressed[j][k] = i;
			} else {
				if (keypressed[j][k] && --keypressed[j][k]) l = 0;
			}
			if (l && nchannel[k] && thru[k]) {
				for (j = pcur[k]; channel[k][j].type != LNDONE; --j);
				thru[k] = 0;
				tmp = MEASURE_TO_MSEC(channel[k][j].time - line, bpm) * shorten[(int)line] * gradefactor;
				if (-144 < tmp && tmp < 144) {
					channel[k][j].type ^= -1;
				} else {
					update_grade(0, 0);
				}
			}
		}

		if (i) { /* down */
			l = 1;
			if (j) {
				keypressed[j][k] = i;
			} else {
				if (keypressed[j][k]++) l = 0;
			}
			if (l && nchannel[k]) {
				j = pcur[k];
				l = pcur[k] - 1;
				tmp = (l >= 0 ? line - channel[k][l].time : 0);
				if (l < 0 || (j < nchannel[k] && tmp > channel[k][j].time - line)) l = j;
				if (channel[k][l].index) play_sound(channel[k][l].index, 0);

				for (j = pcur[k]; j < nchannel[k] && channel[k][j].type == INVNOTE; ++j);
				for (l = pcur[k] - 1; l >= 0 && channel[k][l].type == INVNOTE; --l);
				tmp = (l >= 0 ? line - channel[k][l].time : 0);
				if (l < 0 || (j < nchannel[k] && tmp > channel[k][j].time - line)) l = j;
				if (l < pcheck[k]) continue;

				if (channel[k][l].type == LNDONE) {
					if (thru[k]) update_grade(0, 0);
				} else if (channel[k][l].type >= 0) {
					tmp = MEASURE_TO_MSEC(channel[k][l].time - line, bpm) * shorten[(int)line] * gradefactor;
					if (tmp < 0) tmp *= -1;
					if (channel[k][l].type >= 0 && tmp < 144) {
						if (channel[k][l].type == LNSTART) thru[k] = 1;
						channel[k][l].type ^= -1;
						update_grade((tmp<14.4) + (tmp<48) + (tmp<84) + 1, 300 - tmp/144*300);
					}
				}
			}
		}
	}
	if (bottom > length) {
		if (opt_mode ? Mix_Playing(-1)==0 : Mix_GroupNewer(1)==-1) return 0;
	} else if (bottom < -1) {
		return 0;
	}

	if (opt_mode < EXCLUSIVE_MODE) {
		SDL_FillRect(screen, R(0,30,tpanel1,490), map(0x404040));
		if (tpanel2) SDL_FillRect(screen, R(tpanel2,30,800-tpanel2,490), map(0x404040));
		for (i = 0; i < 18; ++i) {
			if (tkeyleft[i] < 0) continue;
			SDL_FillRect(screen, R(tkeyleft[i],30,tkeywidth[i],490), map(0));
			if (keypressed[0][i] || keypressed[1][i]) {
				SDL_BlitSurface(sprite, R(tkeyleft[i],140,tkeywidth[i],380), screen, R(tkeyleft[i],140,0,0));
			}
		}
		SDL_SetClipRect(screen, R(0,30,800,490));
		for (i = 0; i < 18; ++i) {
			if (tkeyleft[i] < 0) continue;
			for (j = pfront[i]; j < prear[i] && channel[i][j].type == INVNOTE; ++j);
			if (j==prear[i] && prear[i]<nchannel[i] && channel[i][prear[i]].type==LNDONE) {
				SDL_BlitSurface(sprite, R(800+tkeyleft[i%9],0,tkeywidth[i%9],490), screen, R(tkeyleft[i],30,0,0));
			}
			for (; j < prear[i]; ++j) {
				k = (int)(525 - 400 * playspeed * adjust_object_position(bottom, channel[i][j].time));
				if (channel[i][j].type == LNSTART) {
					l = k + 5;
					/* the additional check can be omitted as the sanitization ensures LNDONE always exists */
					while (channel[i][++j].type != LNDONE);
					k = (int)(530 - 400 * playspeed * adjust_object_position(bottom, channel[i][j].time));
					if (k < 30) k = 30;
				} else if (channel[i][j].type == LNDONE) {
					k += 5;
					l = 520;
				} else if (channel[i][j].type == NOTE) {
					l = k + 5;
				} else {
					continue;
				}
				if (k > 0 && l > k) {
					SDL_BlitSurface(sprite, R(800+tkeyleft[i%9],0,tkeywidth[i%9],l-k), screen, R(tkeyleft[i],k,0,0));
				}
			}
		}
		for (i = ibottom; i < top; ++i) {
			j = (int)(530 - 400 * playspeed * adjust_object_position(bottom, i));
			SDL_FillRect(screen, R(0,j,tpanel1,1), map(0xc0c0c0));
			if (tpanel2) SDL_FillRect(screen, R(tpanel2,j,800-tpanel2,1), map(0xc0c0c0));
		}
		if (now < gradetime) {
			int delta = (gradetime - now - 400) / 15;
			if (delta < 0) delta = 0;
			printstr(screen, tpanel1/2-8*strlen(tgradestr[grademode]), 260 - delta, 2,
					tgradestr[grademode], tgradecolor[grademode][0], tgradecolor[grademode][1]);
			if (scombo > 1) {
				i = sprintf(buf, "%d COMBO", scombo);
				printstr(screen, tpanel1/2-4*i, 288 - delta, 1, buf, 0x808080, 0xffffff);
			}
			if (opt_mode) printstr(screen, tpanel1/2-24, 302 - delta, 1, "(AUTO)", 0x404040, 0xc0c0c0);
		}
		SDL_SetClipRect(screen, 0);

		i = (now - origintime) / 1000;
		j = duration / 1000;
		sprintf(buf, "SCORE %07d%c%4.1fx%c%02d:%02d / %02d:%02d%c@%9.4f%cBPM %6.2f",
				score, 0, targetspeed, 0, i/60, i%60, j/60, j%60, 0, bottom, 0, bpm);
		SDL_BlitSurface(sprite, R(0,0,800,30), screen, R(0,0,0,0));
		SDL_BlitSurface(sprite, R(0,520,800,80), screen, R(0,520,0,0));
		printstr(screen, 10, 8, 1, buf, 0, 0);
		printstr(screen, 5, 522, 2, buf+14, 0, 0);
		printstr(screen, tpanel1-94, 565, 1, buf+20, 0, 0x404040);
		printstr(screen, 95, 538, 1, buf+34, 0, 0);
		printstr(screen, 95, 522, 1, buf+45, 0, 0);
		i = (now - origintime) * tpanel1 / duration;
		printchar(screen, 6+(i<tpanel1?i:tpanel1), 548, 1, -1, 0x404040, 0x404040);
		if (!tpanel2 && !opt_mode) {
			if (gauge > 512) gauge = 512;
			k = (int)(160*startshorten*(1+bottom)) % 40; /* i.e. cycles four times per measure */
			i = (gauge<0 ? 0 : (gauge*400>>9) - k);
			j = (gauge>=survival ? 0xc0 : 0xc0 - k*4) << 16;
			SDL_FillRect(screen, R(4,588,i>360?360:i<5?5:i,8), map(j));
		}
	} else if (now - lastinfo >= INFO_INTERVAL) {
		lastinfo = now;
		i = (now - origintime) / 100;
		j = duration / 100;
		fprintf(stderr,
			"\r%72s\r%02d:%02d.%d / %02d:%02d.%d (@%9.4f) | BPM %6.2f | %d / %d notes",
			"", i/600, i/10%60, i%10, j/600, j/10%60, j%10, bottom, bpm, scombo, nnotes);
	}

	if (opt_bga != NO_BGA) {
		SDL_FillRect(screen, R(tbgax,tbgay,256,256), map(0));
		i = (now < poorbga ? 1<<POORBGA_LAYER : 1<<BGA_LAYER|1<<BGA2_LAYER);
		for (j = 0; j < 2; ++j) {
			if ((i>>j&1) && bga[j] >= 0 && imgres[bga[j]].surface) {
				SDL_BlitSurface(imgres[bga[j]].surface, R(0,0,256,256), screen, R(tbgax,tbgay,0,0));
			}
		}
	}

	if (screen) SDL_Flip(screen);
	return 1;
}

/******************************************************************************/
/* entry point */

static int play(void)
{
	int i;
	char *pos1, *pos2;

	if (initialize()) return 1;

	if (parse_bms() || sanitize_bms()) {
		die("Couldn't load BMS file: %s", bmspath);
	}
	if (opt_modf) {
		if (value[V_PLAYER] == 3) {
			shuffle_bms(opt_modf, 0);
		} else {
			shuffle_bms(opt_modf, 1);
			if (value[V_PLAYER] != 1) shuffle_bms(opt_modf, 2);
		}
	}
	get_bms_info();

	/* get basename of bmspath */
	pos1 = strrchr(bmspath, '/');
	pos2 = strrchr(bmspath, '\\');
	if (!pos1) pos1 = bmspath + 1;
	if (!pos2) pos2 = bmspath + 1;
	(pos1>pos2 ? pos1 : pos2)[1] = '\0';
	if (pos1 == pos2) { /* will be pos1 = pos2 = bmspath + 1 */
		bmspath[0] = '.';
		bmspath[1] = sep;
	}

	dirinit();
	play_show_stagefile();
	dirfinal();

	duration = get_bms_duration();
	play_prepare();
	lastinfo = -1000;
	while ((i = play_process()) > 0);

	if (!opt_mode && i == 0) {
		if (gauge >= survival) {
			printf("*** CLEARED! ***\n");
			for (i = 4; i >= 0; --i)
				printf("%-5s %4d    %s", tgradestr[i], scocnt[i], "\n"+(i!=2));
			printf("MAX COMBO %d\nSCORE %07d (max %07d)\n", smaxcombo, score, maxscore);
		} else {
			printf("YOU FAILED!\n");
		}
	} else if (opt_mode >= EXCLUSIVE_MODE) {
		fprintf(stderr, "\r%72s\r", "");
	}

	return 0;
}

int usage(void)
{
	fprintf(stderr,
		"%s -- the simple BMS player\n"
		"http://mearie.org/projects/angolmois/\n\n"
		"Usage: %s <options> <path>\n"
		"  Accepts any BMS, BME or BML file.\n"
		"  Resources should be in the same directory as the BMS file.\n\n"
		"Options:\n"
		"  -h, --help            This help\n"
		"  -V, --version         Shows the version\n"
		"  -a #.#, --speed #.#   Sets the initial play speed (default: 1.0x)\n"
		"  -#                    Same as '-a #.0'\n"
		"  -v, --autoplay        Enables AUTO PLAY (viewer) mode\n"
		"  -x, --exclusive       Enables exclusive (BGA and sound only) mode\n"
		"  -X, --sound-only      Enables sound only mode, equivalent to -xB\n"
		"  --fullscreen          Enables the fullscreen mode (default)\n"
		"  -w, --no-fullscreen   Disables the fullscreen mode\n"
		"  --info                Shows a brief information about the song (default)\n"
		"  -q, --no-info         Do not show an information about the song\n"
		"  -m, --mirror          Uses a mirror modifier\n"
		"  -s, --shuffle         Uses a shuffle modifier\n"
		"  -S, --shuffle-ex      Uses a shuffle modifier, even for scratches\n"
		"  -r, --random          Uses a random modifier\n"
		"  -R, --random-ex       Uses a random modifier, even for scratches\n"
		"  --bga                 Loads and shows the BGA (default)\n"
		"  -B, --no-bga          Do not load and show the BGA\n"
		"  -M, --no-movie        Do not load and show the BGA movie\n"
		"  -j #, --joystick #    Enable the joystick with index # (normally 0)\n\n"
		"Environment Variables:\n"
		"  ANGOLMOIS_1P_KEYS=<1>|<2>|<3>|<4>|<5>|<scratch>|<pedal>|<6>|<7>\n"
		"  ANGOLMOIS_2P_KEYS=<1>|<2>|<3>|<4>|<5>|<scratch>|<pedal>|<6>|<7>\n"
		"  ANGOLMOIS_SPEED_KEYS=<speed down>|<speed up>\n"
		"    Specifies the keys used for gameplay. Key names should follow them of\n"
		"    SDL (e.g. 'a', 'right shift' etc.). The mapping for 1P/2P is as follows:\n"
		"                   <2> <4> <6>\n"
		"      <scratch>  <1> <3> <5> <7>  <pedal>\n"
		"    One can map multiple keys by separating key names with '%%'.\n"
		"    For joystick, 'button #' and 'axis #' can also be used.\n"
		"    The default mapping is to use zsxdcfv and mk,l.;/ for 1P and 2P,\n"
		"    respectively; for joystick it is assumed that the normal Beatmania IIDX\n"
		"    controller is plugged in. F3 and F4 is used for speed adjustment.\n\n",
		VERSION, argv0);
	return 1;
}

int main(int argc, char **argv)
{
	static char *longargs[] =
		{"h--help", "V--version", "a--speed", "v--autoplay", "x--exclusive",
		 "X--sound-only", "w--windowed", "w--no-fullscreen", " --fullscreen",
		 " --info", "q--no-info", "m--mirror", "s--shuffle", "S--shuffle-ex",
		 "r--random", "R--random-ex", " --bga", "B--no-bga", " --movie", "M--no-movie",
		 "j--joystick", NULL};
	char buf[512]={0}, *arg;
	int i, j, cont;

	argv0 = argv[0];
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			if (!bmspath) bmspath = argv[i];
		} else if (!strcmp(argv[i], "--")) {
			if (!bmspath) bmspath = argv[++i];
			break;
		} else {
			if (argv[i][1] == '-') {
				for (j = 0; longargs[j]; ++j) {
					if (!strcmp(argv[i], longargs[j]+1)) {
						argv[i][1] = longargs[j][0];
						argv[i][2] = '\0';
						break;
					}
				}
				if (!longargs[j]) die("Invalid option: %s", argv[i]);
			}
			for (j = cont = 1; cont; ++j) {
				switch (argv[i][j]) {
				case 'h': case 'V': return usage();
				case 'v': opt_mode = AUTOPLAY_MODE; break;
				case 'x': opt_mode = EXCLUSIVE_MODE; break;
				case 'X': opt_mode = EXCLUSIVE_MODE; opt_bga = NO_BGA; break;
				case 'w': opt_fullscreen = 0; break;
				case 'q': opt_showinfo = 0; break;
				case 'm': opt_modf = MIRROR_MODF; break;
				case 's': opt_modf = SHUFFLE_MODF; break;
				case 'S': opt_modf = SHUFFLEEX_MODF; break;
				case 'r': opt_modf = RANDOM_MODF; break;
				case 'R': opt_modf = RANDOMEX_MODF; break;
				case 'a':
					cont = 0;
					arg = argv[i][++j] ? argv[i]+j : argv[++i];
					if (!arg) die("No argument to the option -a");
					playspeed = atof(arg);
					if (playspeed <= 0) playspeed = 1;
					if (playspeed < .1) playspeed = .1;
					if (playspeed > 99) playspeed = 99;
					break;
				case 'B': opt_bga = NO_BGA; break;
				case 'M': opt_bga = BGA_BUT_NO_MOVIE; break;
				case 'j':
					cont = 0;
					arg = argv[i][++j] ? argv[i]+j : argv[++i];
					if (!arg) die("No argument to the option -q");
					opt_joystick = atoi(arg);
					break;
				case '\0': cont = 0;
				case ' ': break;
				default:
					if (argv[i][j] >= '1' && argv[i][j] <= '9') {
						playspeed = argv[i][j] - '0';
					} else {
						die("Invalid option: -%c", argv[i][j]);
					}
				}
			}
		}
	}

	if (!bmspath && filedialog(buf)) bmspath = buf;
	return bmspath ? play() : usage();
}

/* vim: set ts=4 sw=4: */
