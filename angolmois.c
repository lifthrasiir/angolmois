/*
 * Angolmois -- the Simple BMS Player
 * Copyright (c) 2005, Kang Seonghoon (Tokigun).
 * Project Angolmois is copyright (c) 2003-2005, Choi Kaya (CHKY).
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

/******************************************************************************/
/* constants, variables */

char version[] = "TokigunStudio Angolmois version 0.0-alpha1-20050505";

#define ARRAYSIZE(x) (sizeof(x)/sizeof(*x))
const char *bmsheader[] = {
	"title", "genre", "artist", "stagefile",
	"bpm", "player", "playlevel", "rank", "total",
	"lntype", "lnobj", "wav", "bmp", "bga", "stop", "stp",
	"random", "if", "else", "endif"
};
char *bmspath, respath[512];
char **bmsline=0; int nbmsline=0;

char *metadata[4]={0,};
double bpm=130;
int value[6]={1,0,3,100,1,0};
#define v_player value[0]
#define v_playlevel value[1]
#define v_rank value[2]
#define v_total value[3]
#define lntype value[4]
#define lnobj value[5]

char *sndpath[1296]={0,}, *imgpath[1296]={0,};
Mix_Chunk *sndres[1296]={0,};
SDL_Surface *imgres[1296]={0,};
int stoptab[1296]={0,};
double bpmtab[1296]={0,};

typedef struct { double time; int type, index; } bmsnote;
bmsnote **channel[23]={0,};
double _shorten[1001]={0,},*shorten=_shorten+1;
int nchannel[23]={0,};
double length;

/******************************************************************************/
/* system dependent functions */

char *adjust_path(char*); /* will be removed when obfuscation */

#ifdef WIN32
#include <windows.h>
char sep = 92;
char _ofnfilter[] =
	"All Be-Music Source File (*.bms;*.bme;*.bml)\0*.bms;*.bme;*.bml\0"
	"Be-Music Source File (*.bms)\0*.bms\0"
	"Extended Be-Music Source File (*.bme)\0*.bme\0"
	"Longnote Be-Music Source File (*.bml)\0*.bml\0"
	"All Files (*.*)\0*.*\0";
char _ofntitle[] = "Choose a file to play";
int filedialog(char *buf) {
	OPENFILENAME ofn={76,0,0,_ofnfilter,0,0,0,buf,512,0,0,0,_ofntitle,OFN_HIDEREADONLY,0,0,0,0,0,0};
	return GetOpenFileName(&ofn);
}
int errormsg(char *c,char *s)
	{ char b[512];sprintf(b,c,s);return MessageBox(0,b,version,0); }

void dirinit() {}
void dirfinal() {}
Mix_Chunk *load_wav(char *file)
	{ return Mix_LoadWAV(adjust_path(file)); }
SDL_Surface *load_image(char *file)
	{ return IMG_Load(adjust_path(file)); }
#else
#include <dirent.h>
char *flist[2592]={0,}; int nfiles=0;
char sep = 47;
int filedialog(char *buf) { return 0; }
int errormsg(char *c,char *s)
	{ return fprintf(stderr,c,s); }
int lcase(char c)
	{ return((c|32)-19)/26==3?c|32:c; }
int stricmp(char *a, char *b)
	{ while(*a&&*b&&lcase(*a)==lcase(*b))a++,b++;return*a==*b; }

char *strcopy(char*); /* will be removed when obfuscation */
void dirinit() {
	DIR *d; struct dirent *e; int i=0,j=0;
	for(; bmspath[i]; j=bmspath[i++]-sep?j:i);
	if(j > 0) bmspath[j-1] = 0;
	if(d = opendir(j?bmspath:"."))
		while(e = readdir(d))
			flist[nfiles++] = strcopy(e->d_name);
	if(j > 0) bmspath[j-1] = sep;
}
void dirfinal() {
	while(nfiles--) free(flist[nfiles]);
}
Mix_Chunk *load_wav(char *file) {
	int i;
	for(i=0; i<nfiles; i++)
		if(stricmp(flist[i], file)) return Mix_LoadWAV(adjust_path(flist[i]));
	return 0;
}
SDL_Surface *load_image(char *file) {
	int i;
	for(i=0; i<nfiles; i++)
		if(stricmp(flist[i], file)) return IMG_Load(adjust_path(flist[i]));
	return 0;
}
#endif

/******************************************************************************/
/* general functions */

Uint32 crc32t[256]={0,};
void crc32_gen()
	{int i=0;while(i++<2048)crc32t[i/8]=(i%8?crc32t[i/8]/2:0)^(3988292384u*(crc32t[i/8]&1));}
Uint32 crc32(FILE *f)
	{Uint32 r=-1;while(!feof(f))r=(r>>8)^crc32t[(r^fgetc(f))&255];return~r;}
int is_space(int n) { return!(n-9&&n-10&&n-13&&n-32); }

char *adjust_path(char *path) {
	int i=0, j=0;
 
	if(*path != 47 && *path != 92) {
		while(bmspath[i]) {
			respath[i] = bmspath[i];
			if(respath[i++] == sep) j = i;
		}
	}
	for(i=0; *path; path++)
		respath[j++] = (*path==47 || *path==92 ? sep : *path);
	respath[j] = 0;
	return respath;
}

int remove_whitespace(char *str, int *i) {
	int j;
	if(is_space(str[*i])) {
		while(is_space(str[++*i]));
		if(!str[*i]) return 0;
	}
	for(j=*i; str[j]; j++);
	while(is_space(str[--j]));
	str[++j] = 0;
	return 1;
}

char *strcopy(char *src) {
	char *dest; int i=0;
	while(src[i++]);
	dest = malloc(i);
	for(i=0; dest[i]=src[i]; i++);
	return dest;
}

/******************************************************************************/
/* bms parser */

#define GET_CHANNEL(player, chan) ((player)*9+(chan)-1)
#define ADD_NOTE(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 0, (index))
#define ADD_INVNOTE(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 1, (index))
#define ADD_LNDONE(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 2, (index))
#define ADD_LNSTART(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 3, (index))
#define ADD_BGM(time, index) add_note(18, (time), 0, (index))
#define ADD_BGA(time, index) add_note(19, (time), 0, (index))
#define ADD_BGA2(time, index) add_note(19, (time), 1, (index))
#define ADD_POORBGA(time, index) add_note(19, (time), 2, (index))
#define ADD_BPM(time, index) add_note(20, (time), 0, (index))
#define ADD_BPM2(time, index) add_note(20, (time), 1, (index))
#define ADD_STOP(time, index) add_note(21, (time), 0, (index))
#define ADD_STP(time, index) add_note(22, (time), 0, (index))

int getdigit(int n)
	{ return 47<n&&n<58?n-48:((n|32)-19)/26==3?(n|32)-87:-1296; }
int key2index(int a, int b)
	{ return getdigit(a)*36+getdigit(b); }

int compare_bmsline(const void *a, const void *b) {
	int i, j;
	for(i=0; i<6; i++)
		if(j = (*(char**)a)[i] - (*(char**)b)[i]) return j;
	return 0;
}

int compare_bmsnote(const void *a, const void *b) {
	bmsnote *A=*(bmsnote**)a, *B=*(bmsnote**)b;
	return (A->time > B->time ? 1 : A->time < B->time ? -1 : A->type - B->type);
}

void add_note(int chan, double time, int type, int index) {
	bmsnote *temp;
	temp = malloc(sizeof(bmsnote));
	temp->time = time; temp->type = type; temp->index = index;
	channel[chan] = realloc(channel[chan], sizeof(bmsnote*) * (nchannel[chan]+1));
	channel[chan][nchannel[chan]++] = temp;
}

void remove_note(int chan, int index) {
	if(channel[chan][index]) {
		if(chan < 18 && channel[chan][index]->index) {
			ADD_BGM(channel[chan][index]->time, channel[chan][index]->index);
		}
		free(channel[chan][index]);
		channel[chan][index] = 0;
	}
}

int parse_bms() {
	FILE* fp;
	int i, j, k, a, b, c;
	int prev[40]={0,};
	int rnd=1, ignore=0;
	int measure, chan;
	double t;
	char *line=malloc(1024);
	
	fp = fopen(bmspath, "r");
	if(!fp) return 1;
	dirinit();

	srand(time(0));
	while(fgets(line, 1024, fp)) {
		if(line[0] != 35) continue;
		line++;

		for(i=0; i<ARRAYSIZE(bmsheader); i++) {
			for(j=0; bmsheader[i][j]; j++)
				if((bmsheader[i][j]|32) != (line[j]|32)) break;
			if(!bmsheader[i][j]) break;
		}
		switch(i) {
		case 16: /* random */
			if(is_space(line[j]))
				if(i = abs(atoi(line+j)))
					rnd = rand() % i + 1;
			break;
		case 17: /* if */
			if(is_space(line[j]))
				ignore = (rnd != atoi(line+j));
			break;
		case 18: /* else */
			ignore = !ignore;
			break;
		case 19: /* endif */
			ignore = 0;
			break;
		}
		
		if(!ignore) {
			switch(i) {
			case 0: /* title */
			case 1: /* genre */
			case 2: /* artist */
			case 3: /* stagefile */
				if(!remove_whitespace(line, &j)) break;
				metadata[i] = strcopy(line+j);
				break;
			case 4: /* bpm */
				if(is_space(line[j])) {
					bpm = atof(line+j);
				} else {
					i = key2index(line[j], line[j+1]);
					if(i < 0 || !is_space(line[j+2])) break;
					bpmtab[i] = atof(line+j+2);
				}
				break;
			case 5: /* player */
			case 6: /* playlevel */
			case 7: /* rank */
			case 8: /* total */
			case 9: /* lntype */
				if(is_space(line[j]))
					value[i-5] = atoi(line+j);
				break;
			case 10: /* lnobj */
				if(!is_space(line[j])) break;
				while(is_space(line[++j]));
				if(line[j])
					lnobj = key2index(line[j], line[j+1]);
				break;
			case 11: /* wav## */
				i = key2index(line[j], line[j+1]);
				if(i < 0) break;
				j += 2;
				if(!remove_whitespace(line, &j)) break;
				if(sndpath[i]) { free(sndpath[i]); sndpath[i] = 0; }
				sndpath[i] = strcopy(line+j);
				break;
			case 12: /* bmp## */
				i = key2index(line[j], line[j+1]);
				if(i < 0) break;
				j += 2;
				if(!remove_whitespace(line, &j)) break;
				if(imgpath[i]) { free(imgpath[i]); imgpath[i] = 0; }
				imgpath[i] = strcopy(line+j);
				break;
			case 13: /* bga## */
				/* TODO */
				break;
			case 14: /* stop## */
				i = key2index(line[j], line[j+1]);
				if(i < 0 || !is_space(line[j+2])) break;
				stoptab[i] = atoi(line+j+2);
				break;
			case 15: /* stp## */
				/* TODO */
				break;
			}
			
			for(i=0; i<5; i++)
				if((line[i]-8)/10 != 4) break;
			if(i>4 && line[5]==58 && line[6]) {
				bmsline = realloc(bmsline, sizeof(char*) * (nbmsline+1));
				bmsline[nbmsline] = strcopy(line);
				nbmsline++;
			}
		}
		line--;
	}
	free(line);
	fclose(fp);

	qsort(bmsline, nbmsline, sizeof(char*), compare_bmsline);
	for(i=0; i<nbmsline; i++) {
		j = 0;
		for(k=0; k<5; k++)
			j = j * 10 + bmsline[i][k] - 48;
		measure = j / 100; chan = j % 100;
		if(chan == 2) {
			shorten[measure] = atof(bmsline[i]+6);
		} else {
			j = 6;
			remove_whitespace(bmsline[i], &j);
			for(k=j; bmsline[i][k]; k++);
			a = (k - j) / 2;
			for(k=0; k<a; k++,j+=2) {
				b = key2index(bmsline[i][j], bmsline[i][j+1]);
				t = measure + 1. * k / a;
				if(b) {
					if(chan == 1) {
						ADD_BGM(t, b);
					} else if(chan == 3 && (b/36<16 && b%36<16)) {
						ADD_BPM(t, b/36*16+b%36);
					} else if(chan == 4) {
						ADD_BGA(t, b);
					} else if(chan == 6) {
						ADD_POORBGA(t, b);
					} else if(chan == 7) {
						ADD_BGA2(t, b);
					} else if(chan == 8) {
						ADD_BPM2(t, b);
					} else if(chan == 9) {
						ADD_STOP(t, b);
					} else if(chan % 10 != 0 && chan > 9 && chan < 30) {
						if(lnobj && b == lnobj) {
							c = GET_CHANNEL(chan>20, chan%10);
							if(nchannel[c] && channel[c][nchannel[c]-1]->type==0) {
								channel[c][nchannel[c]-1]->type = 3;
								ADD_LNDONE(chan>20, chan%10, t, b);
							}
						} else {
							ADD_NOTE(chan>20, chan%10, t, b);
						}
					} else if(chan % 10 != 0 && chan > 29 && chan < 50) {
						ADD_INVNOTE(chan>40, chan%10, t, b);
					}
				}
				if(chan % 10 != 0 && chan > 49 && chan < 70) {
					if(lntype == 1 && b) {
						if(prev[chan-50]) {
							prev[chan-50] = 0;
							ADD_LNDONE(chan>60, chan%10, t, 0);
						} else {
							prev[chan-50] = b;
							ADD_LNSTART(chan>60, chan%10, t, b);
						}
					} else if(lntype == 2) {
						if(prev[chan-50] || prev[chan-50] != b) {
							if(prev[chan-50]) {
								if(prev[chan-30] + 1 < measure) {
									ADD_LNDONE(chan>60, chan%10, prev[chan-30]+1, 0);
								} else if(prev[chan-50] != b) {
									ADD_LNDONE(chan>60, chan%10, t, 0);
								}
							}
							if(b && (prev[chan-50]!=b || prev[chan-30]+1<measure)) {
								ADD_LNSTART(chan>60, chan%10, t, b);
							}
							prev[chan-30] = measure;
							prev[chan-50] = b;
						}
					}
				}
			}
		}
		free(bmsline[i]);
	}
	free(bmsline);
	length = measure + 1;
	for(i=0; i<20; i++) {
		if(prev[i]) {
			if(lntype == 2 && prev[i+20] + 1 < measure) {
				ADD_LNDONE(i>10, i%10, prev[i+20]+1, 0);
			} else {
				ADD_LNDONE(i>10, i%10, length, 0);
			}
		}
	}
	
	for(i=0; i<23; i++) {
		if(!channel[i]) continue;
		qsort(channel[i], nchannel[i], sizeof(bmsnote*), compare_bmsnote);
		if(i != 18 && i < 21) {
			b = 0; t = -1;
			for(j=0; j<=nchannel[i]; j++) {
				if(j == nchannel[i] || channel[i][j]->time > t) {
					if(t >= 0) {
						c = 0;
						for(; k<j; k++) {
							if(
								i<18 ?
									(c & 1<<channel[i][k]->type) ||
									(b ?
										(a&4)==0 || channel[i][k]->type < 2 :
										channel[i][k]->type != ((a&12)==8 ? 3 : a&1 ? 0 : 1)
									)
								: i==19 ? c & 1<<channel[i][k]->type : c
							) {
								remove_note(i, k);
							} else {
								c |= 1 << channel[i][k]->type;
							}
						}
						b = (b ? (a&12)!=4 : (a&12)==8);
					}
					if(j == nchannel[i]) break;
					a = 0;
					k = j;
					t = channel[i][j]->time;
				}
				a |= 1 << channel[i][j]->type;
			}
			if(i<18 && b) {
				while(j >= 0 && !channel[i][--j]);
				if(j >= 0 && channel[i][j]->type == 3) remove_note(i, j);
			}
		}
		k = 0;
		for(j=0; j<nchannel[i]; j++) {
			if(channel[i][j]) channel[i][j-k] = channel[i][j]; else k++;
		}
		nchannel[i] -= k;
	}

	for(i=0; i<4; i++)
		if(!metadata[i]) {
			metadata[i] = malloc(1);
			*metadata[i] = 0;
		}
	for(i=0; i<1000; i++)
		if(shorten[i] <= 0.001)
			shorten[i] = 1.0;
	shorten[-1] = shorten[0];

	return 0;
}

int load_resource(void (*callback)(char*)) {
	SDL_Surface *temp;
	int i;
	
	for(i=0; i<1296; i++) {
		if(sndpath[i]) {
			if(callback) callback(sndpath[i]);
			sndres[i] = load_wav(sndpath[i]);
			free(sndpath[i]); sndpath[i] = 0;
		}
		if(imgpath[i]) {
			if(callback) callback(imgpath[i]);
			if(temp = load_image(imgpath[i])) {
				imgres[i] = SDL_DisplayFormat(temp);
				SDL_FreeSurface(temp);
				SDL_SetColorKey(imgres[i], SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
			}
			free(imgpath[i]); imgpath[i] = 0;
		}
	}
	return 0;
}

/*
bit 0: it uses 7 keys?
bit 1: it uses long-note?
bit 2: it uses pedal?
bit 3: it has bpm variation?
...
*/
int get_bms_info(int *flag, int *nnotes, int *score, int *duration) {
	int i, j;
	
	*flag = *nnotes = 0;
	if(nchannel[7] || nchannel[8] || nchannel[16] || nchannel[17]) *flag |= 1;
	if(nchannel[6] || nchannel[15]) *flag |= 4;
	if(nchannel[20]) *flag |= 8;
	for(i=0; i<18; i++)
		for(j=0; j<nchannel[i]; j++) {
			if(channel[i][j]->type > 1) *flag |= 2;
			if(channel[i][j]->type < 3) ++*nnotes;
		}
	
	return 0;
}

/******************************************************************************/
/* general graphic functions */

SDL_Surface *screen;
SDL_Event event;
SDL_Rect rect[8]; int _rect=0;

int getpixel(SDL_Surface *s, int x, int y)
	{ return((Uint32*)s->pixels)[x+y*s->pitch/4]; }
int putpixel(SDL_Surface *s, int x, int y, int c)
	{ return((Uint32*)s->pixels)[x+y*s->pitch/4]=c; }
void drawhline(SDL_Surface *s, int x1, int x2, int y, int c)
	{ while(x1<x2) putpixel(s, x1++, y, c); }
void drawvline(SDL_Surface *s, int x, int y1, int y2, int c)
	{ while(y1<y2) putpixel(s, x, y1++, c); }
int blend(int x, int y, int a, int b)
	{ int i=0;for(;i<24;i+=8)y+=((x>>i&255)-(y>>i&255))*a/b<<i;return y; }
void putblendedpixel(SDL_Surface *s, int x, int y, int c, int o)
	{ putpixel(s, x, y, blend(getpixel(s,x,y), c, o, 255)); }
SDL_Surface *newsurface(int f, int w, int h)
	{ return SDL_CreateRGBSurface(f,w,h,32,0xff0000,0xff00,0xff,0); }
SDL_Rect *newrect(int x, int y, int w, int h)
	{ SDL_Rect*r=rect+_rect++%8;r->x=x;r->y=y;r->w=w;r->h=h;return r; }

int bicubic_kernel(int x, int y) {
	if(x < 0) x = -x;
	if(x < y) {
		return ((y*y - 2*x*x + x*x/y*x) << 11) / y / y;
	} else if(x < y * 2) {
		return ((4*y*y - 8*x*y + 5*x*x - x*x/y*x) << 11) / y / y;
	} else {
		return 0;
	}
}

int bicubic_interpolation(SDL_Surface *src, SDL_Surface *dest) {
	int x, dx, y, dy, ww, hh, w, h;
	int i, j, k, r, g, b, c, xx, yy, a;

	dx = x = 0;
	ww = src->w - 1; hh = src->h - 1;
	w = dest->w - 1; h = dest->h - 1;
	for(i=0; i<=w; i++) {
		dy = y = 0;
		for(j=0; j<=h; j++) {
			r = g = b = 0;
			for(k=0; k<16; k++) {
				xx = x + k/4 - 1; yy = y + k%4 - 1;
				if(xx<0 || xx>ww || yy<0 || yy>hh) continue;
				c = getpixel(src, xx, yy);
				a = bicubic_kernel(xx*w-i*ww, w) * bicubic_kernel(yy*h-j*hh, h) >> 6;
				r += (c&255) * a; g += (c>>8&255) * a; b += (c>>16&255) * a;
			}
			r = (r<0 ? 0 : r>>24 ? 255 : r>>16);
			g = (g<0 ? 0 : g>>24 ? 255 : g>>16);
			b = (b<0 ? 0 : b>>24 ? 255 : b>>16);
			putpixel(dest, i, j, r | (g<<8) | (b<<16));
			dy += hh; if(dy > h) { y++; dy -= h; }
		}
		dx += ww; if(dx > w) { x++; dx -= w; }
	}

	return 0;
}

/******************************************************************************/
/* font functions */

Uint8 fontmap[]="\0\2\3\6\7\10\13\14\16\20\30\33\34\36$,03678;<>?@ACU]^`acfghkl"
	"nopsvwx{|~\177\201\202\237\303\333\371\376";
Uint8 fontinfo[]="\37>',\37==8==M\\\256\211\255K==========MNM{M================"
	"==>==========K=\26\315&]=]=]=_-=?==]]]``]]=]]]]_]-\20-7";
Uint8 fontdata[]="\\WWWWWWWWWWWW\\.::::..$...66662'67;O7;O64));IIH;*III;))CDE'+\
	.4E?&8JJJ9IKFK9+++.-+.444444.+4.++++++.4.ZZT.TZZ....T......4T...%'+.4C=;EEG\
	LRNEE;0:3++++++<;EEG,08MCU;EE&1&&EE;,16FFFFU''UCCCT&&EE;;EECTEEEE;U&&'+....\
	.;EEE;EEEE;;EEEE<&EE;...$$$......$$$...4'+.4C4.+'T$$TC4.+'+.4C;EEE'+.$..;>A\
	@@@@B=;)06EEEUEEETEEETEEEET15ECCCCE51SFEEEEEEFSUCCCTCCCCUUCCCTCCCCC15ECCGEE\
	51EEEEUEEEEET........T&&&&&&EEE;EFJQMMQJFECCCCCCCCCUEPUUIIEEEEEEENRLGEEE06E\
	EEEEE60TEEEETCCCC;EEEEEIL;(&TEEEETEEEE;EEC;&&EE;T.........EEEEEEEEE;EEEEEEE\
	60)EEEEIIIU66>EP;00;PE>VYF:......U&&(,08MCU:44444444:=C4.+'%:++++++++:)06E>\
	U4.+';&&<EEE<CCCTEEEEET;EECCEE;&&&<EEEEE<;EEEUCE;,//.T.....<EEEE<&EE;CCCTEE\
	EEEEE..$.......&&$$&&&&EEE;CCEFJQQJFE0+++++++++OUIIIIEETEEEEEEE;EEEEEE;TEEE\
	EEETCCC<EEEEEE<&&&TEECCCCC;EC;&&E;..T...///,EEEEEEE;EEEEE60)EEEIIIU6>E6006E\
	>EEEE51+.4CU(,08MCU,....M....,................M....,....MM[X,";
Uint8 rawfont[16][95]={0,}, (*zoomfont[16])[95]={rawfont,0,};

int _fontprocess(int x, int y, int z, int c, int s) {
	int i, j;
	for(i=0; i<z; i++)
		for(j=(s==1?z-i:s==3?i+1:0); j<(s==2?i:s==4?z-i-1:z); j++)
			zoomfont[z-1][(y*z+i)*z+j][c] |= 1<<(7-x);
	return z;
}

void fontprocess(int z) {
	int i, j, k, l;
	if(z) {
		if(zoomfont[z-1]) return;
		zoomfont[z-1] = malloc(95*16*z*z);
		for(i=0; i<95; i++) {
			for(j=0; j<16*z*z; j++)
				zoomfont[z-1][j][i] = 0;
			for(j=0; j<16; j++)
				for(k=0; k<8; k++) {
					l = (j>0?rawfont[j-1][i]<<k>>6&7:0)<<6 |
						(rawfont[j][i]<<k>>6&7)<<3 |
						(j<15?rawfont[j+1][i]<<k>>6&7:0);
					if((i==3 || i==20) && k<2) l |= (l & 0222) << 1;
					if((i==3 || i==20) && k>6) l |= (l & 0222) >> 1;
					if(l & 0x10) {
						_fontprocess(k, j, z, i, 0);
					} else {
						if((l & 0xda) == 0xa || (l & 0x3f) == 0xb) /* /| */
							_fontprocess(k, j, z, i, 1);
						if((l & 0x1b2) == 0x22 || (l & 0x3f) == 0x26) /* |\ */
							_fontprocess(k, j, z, i, 2);
						if((l & 0x9b) == 0x88 || (l & 0x1f8) == 0xc8) /* \| */
							_fontprocess(k, j, z, i, 3);
						if((l & 0xb6) == 0xa0 || (l & 0x1f8) == 0x1a0) /* |/ */
							_fontprocess(k, j, z, i, 4);
					}
				}
		}
	} else {
		for(i=k=0; i<95; i++)
			for(j=--fontinfo[i]/16; j<=fontinfo[i]%16; k++)
				if(!is_space(fontdata[k])) rawfont[j++][i] = fontmap[fontdata[k]-36];
	}
}

void fontfinalize() {
	int i;
	for(i=1; i<ARRAYSIZE(zoomfont); i++)
		if(zoomfont[i]) free(zoomfont[i]);
}

int printchar(SDL_Surface *s, int x, int y, int z, int c, int u, int v) {
	int i, j;
	if(!is_space(c)) {
		c -= (c<33 || c>126 ? c : 32);
		for(i=0; i<16*z; i++)
			for(j=0; j<8*z; j++)
				if(zoomfont[z-1][i*z+j%z][c]&(1<<(7-j/z)))
					putpixel(s, x+j, y+i, blend(u, v, i, 16*z-1));
	}
	return 8*z;
}

void printstr(SDL_Surface *s, int x, int y, int z, char *S, int u, int v)
	{ while(*S)x+=printchar(s,x,y,z,(Uint8)*S++,u,v); }

/******************************************************************************/
/* main routines */

double playspeed=1;
int starttime;
double startoffset, startshorten;
int xflag, xnnotes, xscore, xduration;
int pcur[23]={0,}, pfront[23]={0,}, prear[23]={0,}, pcheck[18]={0,}, thru[23]={0,};
int bga[3]={-1,-1,-1}, bga_updated=0;
int score=0, scocnt[2]={0,}, scombo=0, gradetime=0, grademode=0;

const char *arglist[] = {
	"viewer", "showinfo", "hideinfo", "window", "fullscreen", "quality",
	"x", "speed", "lntype"
};
int opt_mode=0, opt_showinfo=1, opt_fullscreen=1, opt_quality=10;

SDL_Surface *sprite=0;
int keymap[18]={
	SDLK_z, SDLK_s, SDLK_x, SDLK_d, SDLK_c, SDLK_LSHIFT, SDLK_LALT, SDLK_f, SDLK_v,
	SDLK_m, SDLK_k, SDLK_COMMA, SDLK_l, SDLK_PERIOD, SDLK_RSHIFT, SDLK_RALT, SDLK_SEMICOLON, SDLK_SLASH};
int keypressed[18]={0,};
int tkeyleft[18]={41,67,93,119,145,0,223,171,197, 578,604,630,656,682,760,537,708,734};
int tkeywidth[18]={25,25,25,25,25,40,40,25,25, 25,25,25,25,25,40,40,25,25};
int tpanel1=264, tpanel2=536, tbga=272;
#define WHITE 0x808080
#define BLUE 0x8080ff
int tkeycolor[18]={
	WHITE,BLUE,WHITE,BLUE,WHITE,0xff8080,0x80ff80,BLUE,WHITE,
	WHITE,BLUE,WHITE,BLUE,WHITE,0xff8080,0x80ff80,BLUE,WHITE};
#undef WHITE
#undef BLUE

int check_exit() {
	int i = 0;
	while(SDL_PollEvent(&event)) {
		if(event.type==SDL_QUIT || (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE)) i = 1;
	}
	return i;
}

int initialize() {
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0)
		return errormsg("SDL Initialization Failure: %s", SDL_GetError());
	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(800, 600, 32, opt_fullscreen ? SDL_SWSURFACE|SDL_FULLSCREEN : SDL_SWSURFACE|SDL_DOUBLEBUF);
	if(!screen)
		return errormsg("SDL Video Initialization Failure: %s", SDL_GetError());
	SDL_ShowCursor(SDL_DISABLE);
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2<<opt_quality)<0)
		return errormsg("SDL Mixer Initialization Failure: %s", Mix_GetError());
	SDL_WM_SetCaption(version, 0);

	fontprocess(0);
	fontprocess(2);
	fontprocess(3);
	return 0;
}
	
void finalize() {
	int i, j;
 
	for(i=0; i<4; i++) {
		if(metadata[i]) free(metadata[i]);
	}
	for(i=0; i<1296; i++) {
		if(sndpath[i]) free(sndpath[i]);
		if(imgpath[i]) free(imgpath[i]);
		if(sndres[i]) Mix_FreeChunk(sndres[i]);
		if(imgres[i]) SDL_FreeSurface(imgres[i]);
	}
	for(i=0; i<22; i++) {
		if(channel[i]) {
			for(j=0; j<nchannel[i]; j++)
				free(channel[i][j]);
			free(channel[i]);
		}
	}

	if(sprite) SDL_FreeSurface(sprite);
	Mix_CloseAudio();
	fontfinalize();
	dirfinal();
}

SDL_Surface *stagefile_tmp;

void callback_resource(char *path) {
	int i;
	for(i=0; path[i]; i++);
	SDL_BlitSurface(stagefile_tmp, newrect(0,0,800,20), screen, newrect(0,580,800,20));
	printstr(screen, 797-8*i, 582, 1, path, 0x808080, 0xc0c0c0);
	SDL_Flip(screen);
	check_exit();
}

void play_show_stagefile() {
	SDL_Surface *temp, *stagefile;
	char buf[256];
	int i, j, t;

	sprintf(buf, "%s: %s - %s", version, metadata[2], metadata[0]);
	SDL_WM_SetCaption(buf, 0);
	printstr(screen, 248, 284, 2, "loading bms file...", 0x202020, 0x808080);
	SDL_Flip(screen);

	stagefile_tmp = newsurface(SDL_SWSURFACE, 800, 20);
	if(metadata[3] && (temp = IMG_Load(adjust_path(metadata[3])))) {
		stagefile = SDL_DisplayFormat(temp);
		bicubic_interpolation(stagefile, screen);
		SDL_FreeSurface(temp);
		SDL_FreeSurface(stagefile);
	}
	if(opt_showinfo) {
		for(i=0; i<800; i++) {
			for(j=0; j<42; j++) putblendedpixel(screen, i, j, 0x101010, 64);
			for(j=580; j<600; j++) putblendedpixel(screen, i, j, 0x101010, 64);
		}
		printstr(screen, 6, 4, 2, metadata[0], 0x808080, 0xffffff);
		for(i=0; metadata[1][i]; i++);
		for(j=0; metadata[2][j]; j++);
		printstr(screen, 792-8*i, 4, 1, metadata[1], 0x808080, 0xffffff);
		printstr(screen, 792-8*j, 20, 1, metadata[2], 0x808080, 0xffffff);
		i = sprintf(buf, "Level %d | BPM %.2f%s | %d note%s [%dKEY%s]",
			v_playlevel, bpm, "?"+((xflag&8)==0), xnnotes,
			"s"+(xnnotes==1), (xflag&1) ? 7 : 5, (xflag&2) ? "-LN" : "");
		printstr(screen, 3, 582, 1, buf, 0x808080, 0xffffff);
		SDL_BlitSurface(screen, newrect(0,580,800,20), stagefile_tmp, newrect(0,0,800,20));
	}
	SDL_Flip(screen);

	t = SDL_GetTicks() + 3000;
	load_resource(opt_showinfo ? callback_resource : 0);
	if(opt_showinfo) {
		callback_resource("loading...");
		SDL_FreeSurface(stagefile_tmp);
	}
	while((int)SDL_GetTicks() < t && !check_exit());
}

void play_prepare() {
	int i, j, c;
	
	/* panel position */
	if((xflag&4) == 0) {
		tkeyleft[6] = tkeyleft[15] = -1;
		tpanel1 -= tkeywidth[6] + 1;
		tpanel2 += tkeywidth[15] + 1;
	}
	if((xflag&1) == 0) {
		tkeyleft[7] = tkeyleft[8] = tkeyleft[16] = tkeyleft[17] = -1;
		for(i=9; i<16; i++)
			if(i!=14 && tkeyleft[i]>=0)
				tkeyleft[i] += tkeywidth[16] + tkeywidth[17] + 2;
		if(tkeyleft[6] > 0)
			tkeyleft[6] -= tkeywidth[7] + tkeywidth[8] + 2;
		tpanel1 -= tkeywidth[7] + tkeywidth[8] + 2;
		tpanel2 += tkeywidth[16] + tkeywidth[17] + 2;
	}
	if(v_player == 1) {
		for(i=9; i<18; i++) tkeyleft[i] = -1;
		tpanel2 = 0;
	} else if(v_player == 3) {
		for(i=9; i<18; i++) tkeyleft[i] += tpanel1 - tpanel2;
		tpanel1 += 801 - tpanel2;
		tpanel2 = 0;
		tbga = tpanel1 / 2 + 282;
	}
	
	/* sprite */
	sprite = newsurface(SDL_SWSURFACE, 1200, 600);
	for(i=0; i<18; i++) {
		if(tkeyleft[i] < 0) continue;
		for(j=140; j<520; j++)
			SDL_FillRect(sprite, newrect(tkeyleft[i],j,tkeywidth[i],1), blend(tkeycolor[i], 0, j-140, 1000));
		for(j=0; j*2<tkeywidth[i]; j++)
			SDL_FillRect(sprite, newrect(800+tkeyleft[i]+j,0,tkeywidth[i]-2*j,600), blend(tkeycolor[i], 0xffffff, tkeywidth[i]-j, tkeywidth[i]));
	}
	for(j=-244; j<556; j++) {
		for(i=-10; i<20; i++) {
			c = (i*2+j*3+750) % 2000;
			c = blend(0xc0c0c0, 0x606060, c>1000 ? 1850-c : c-150, 700);
			putpixel(sprite, j+244, i+10, c);
		}
		for(i=-20; i<60; i++) {
			c = (i*3+j*2+750) % 2000;
			c = blend(0xc0c0c0, 0x404040, c>1000 ? 1850-c : c-150, 700);
			putpixel(sprite, j+244, i+540, c);
		}
	}
	SDL_FillRect(sprite, newrect(tpanel1+20,0,(tpanel2?tpanel2:820)-tpanel1-40,600), 0);
	for(i=0; i<20; i++) {
		for(j=20; j*j+i*i>400; j--) {
			putpixel(sprite, tpanel1+j, i+10, 0);
			putpixel(sprite, tpanel1+j, 539-i, 0);
			if(tpanel2) {
				putpixel(sprite, tpanel2-j-1, i+10, 0);
				putpixel(sprite, tpanel2-j-1, 539-i, 0);
			}
		}
	}

	/* screen */
	SDL_FillRect(screen, 0, 0);
	SDL_BlitSurface(sprite, newrect(0,0,800,30), screen, newrect(0,0,0,0));
	SDL_BlitSurface(sprite, newrect(0,520,800,80), screen, newrect(0,520,0,0));
	starttime = SDL_GetTicks();
	startoffset = -1;
	startshorten = 1;
	Mix_AllocateChannels(128);
}

double adjust_object_time(double base, double offset) {
	int i = (int)(base+1)-1;
	if((i + 1 - base) * shorten[i] > offset)
		return base + offset / shorten[i];
	offset -= (i + 1 - base) * shorten[i];
	while(shorten[++i] <= offset)
		offset -= shorten[i];
	return i + offset / shorten[i];
}

double adjust_object_position(double base, double time) {
	int i = (int)(base+1)-1, j = (int)(time+1)-1;
	base = (time - j) * shorten[j] - (base - i) * shorten[i];
	while(i < j) base += shorten[i++];
	return base;
}

int play_process() {
	int i, j, k, ibottom;
	double bottom, top, line, tmp;
	char buf[30];

	bottom = startoffset + (SDL_GetTicks() - starttime) * bpm / startshorten / 24e4;
	ibottom = (int)(bottom + 1) - 1;
	if(bottom > -1 && startshorten != shorten[ibottom]) {
		starttime += (int)((ibottom - startoffset) * 24e4 * startshorten / bpm);
		startoffset = ibottom;
		startshorten = shorten[ibottom];
	}
	line = adjust_object_time(bottom, 0.03/playspeed);
	top = adjust_object_time(bottom, 1.25/playspeed);
	for(i=0; i<18; i++) {
		while(pfront[i] < nchannel[i] && channel[i][pfront[i]]->time < bottom) pfront[i]++;
		while(prear[i] < nchannel[i] && channel[i][prear[i]]->time <= top) prear[i]++;
		k = pcur[i];
		while(pcur[i] < nchannel[i] && channel[i][pcur[i]]->time <= line) {
			if(opt_mode && sndres[channel[i][pcur[i]]->index])
				Mix_PlayChannel(-1, sndres[channel[i][pcur[i]]->index], 0);
			pcur[i]++;
		}
		if(!opt_mode) {
			if(k < pcur[i]) pcheck[i] = 1;
			if(pcheck[i]) {
				tmp = channel[i][pcur[i]-1]->time;
				tmp = (line - tmp) * shorten[(int)tmp];
				if(channel[i][pcur[i]-1]->type < 0) {
					pcheck[i] = 0;
				} else if(channel[i][pcur[i]-1]->type == 0 && tmp > 0.08) {
					pcheck[i] = 0; scocnt[1]++; scombo = 0;
					gradetime = SDL_GetTicks() + 700; grademode = 1;
				}
			}
		}
	}
	for(i=18; i<22; i++) {
		while(pcur[i] < nchannel[i] && channel[i][pcur[i]]->time < line) {
			if(i == 18) {
				if(sndres[channel[i][pcur[i]]->index]) {
					j = Mix_PlayChannel(-1, sndres[channel[i][pcur[i]]->index], 0);
					if(j >= 0) Mix_Volume(j, 96);
				}
			} else if(i == 19) {
				bga[channel[i][pcur[i]]->type] = channel[i][pcur[i]]->index;
				bga_updated = 1;
			} else if(i == 20) {
				starttime += (int)((bottom - startoffset) * 24e4 * startshorten / bpm);
				startoffset = bottom;
				bpm = (channel[i][pcur[i]]->type ? bpmtab[channel[i][pcur[i]]->index] : channel[i][pcur[i]]->index);
			}
			pcur[i]++;
		}
	}

	k = 0;
	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			k = 1;
		} else if(event.type == SDL_KEYUP) {
			if(event.key.keysym.sym == SDLK_ESCAPE) {
				k = 1; continue;
			}
			for(i=0; i<18; i++)
				if(event.key.keysym.sym == keymap[i])
					keypressed[i] = 0;
		} else if(event.type == SDL_KEYDOWN) {
			if(opt_mode) continue;
			for(i=0; i<18; i++) {
				if(tkeyleft[i]>=0 && event.key.keysym.sym==keymap[i]) {
					keypressed[i] = 1;
					if(!nchannel[i]) continue;
					j = (pcur[i] < 1 || (pcur[i] < nchannel[i] && channel[i][pcur[i]-1]->time + channel[i][pcur[i]]->time < 2*line) ? pcur[i] : pcur[i]-1);
					if(sndres[channel[i][j]->index])
						Mix_PlayChannel(-1, sndres[channel[i][j]->index], 0);
					tmp = (channel[i][j]->time - line) * shorten[(int)line];
					if(channel[i][j]->type >= 0 && -0.08 < tmp && tmp < 0.05) {
						channel[i][j]->type ^= -1; scocnt[0]++; scombo++;
						gradetime = SDL_GetTicks() + 700; grademode = 0;
					}
				}
			}
		}
	}
	if(k) return -1;
	if((bottom < -1.01 || bottom > length) && !Mix_Playing(-1)) return 0;

	SDL_FillRect(screen, newrect(0,30,tpanel1,490), 0x404040);
	if(tpanel2) SDL_FillRect(screen, newrect(tpanel2,30,800-tpanel2,490), 0x404040);
	for(i=0; i<18; i++) {
		if(tkeyleft[i] < 0) continue;
		SDL_FillRect(screen, newrect(tkeyleft[i],30,tkeywidth[i],490), 0);
		if(keypressed[i]) {
			SDL_BlitSurface(sprite, newrect(tkeyleft[i],140,tkeywidth[i],380), screen, newrect(tkeyleft[i],140,0,0));
		}
	}
	SDL_SetClipRect(screen, newrect(0,30,800,490));
	for(i=0; i<18; i++) {
		if(tkeyleft[i] < 0) continue;
		for(j=pfront[i]; j<prear[i]; j++) {
			if(channel[i][j]->type >= 0) {
				SDL_BlitSurface(
					sprite, newrect(800+tkeyleft[i%9],0,tkeywidth[i%9],5),
					screen, newrect(tkeyleft[i],(int)(525-400*playspeed*adjust_object_position(bottom,channel[i][j]->time)),0,0));
			}
		}
	}
	for(i=(int)top; i>=ibottom; i--) {
		SDL_FillRect(screen, newrect(0,(int)(530-400*playspeed*adjust_object_position(bottom,i)),tpanel1,1), 0xc0c0c0);
		if(tpanel2) SDL_FillRect(screen, newrect(tpanel2,(int)(530-400*playspeed*adjust_object_position(bottom,i)),800-tpanel2,1), 0xc0c0c0);
	}
	SDL_SetClipRect(screen, 0);
	if(bga_updated) {
		SDL_FillRect(screen, newrect(tbga,172,256,256), 0);
		for(i=0; i<2; i++)
			if(bga[i] >= 0 && imgres[bga[i]])
				SDL_BlitSurface(imgres[bga[i]], 0, screen, newrect(tbga,172,256,256));
		bga_updated = 0;
	}
	if((int)SDL_GetTicks() < gradetime) {
		if(grademode) {
			printstr(screen, 79, 292, 2, "MISS", 0xff4040, 0xffc0c0);
		} else {
			printstr(screen, 63, 292, 2, "GREAT!", 0x40ff40, 0xc0ffc0);
			if(scombo > 1) {
				i = sprintf(buf, "%d COMBO", scombo);
				printstr(screen, 111-4*i, 320, 1, buf, 0x808080, 0xffffff);
			}
		}
	}

	SDL_BlitSurface(sprite, newrect(0,0,800,30), screen, newrect(0,0,0,0));
	sprintf(buf, "%.4f (%d) :: BPM %.2f", bottom, Mix_Playing(-1), bpm);
	printstr(screen, 10, 8, 1, buf, 0x000000, 0x000000);

	SDL_Flip(screen);
	return 1;
}

/******************************************************************************/
/* entry point */

int play() {
	int t;
	if(initialize()) return 1;
	if(parse_bms()) {
		finalize(); SDL_Quit();
		return *bmspath && errormsg("Couldn't load BMS file: %s", bmspath);
	}
	get_bms_info(&xflag, &xnnotes, &xscore, &xduration);
	play_show_stagefile();
	play_prepare();
	while((t = play_process())>0);
	finalize();
	return t;
}

int credit() {
	SDL_Surface *credit;
	int i, t = -750;

	opt_fullscreen = 0;
	if(initialize()) return 1;
	credit = newsurface(SDL_SWSURFACE, 800, 2200);
	SDL_FillRect(credit, 0, 0x000010);
	SDL_FillRect(credit, newrect(0, 1790, 800, 410), 0);
	for(i=1; i<16; i++)
		SDL_FillRect(credit, newrect(0, 1850-i*5, 800, 5), i);
#define printstrc(s, l, y, z, c) printstr(credit, 400-l*z*4, y, z, s, c, 0xffffff)
	printstrc("TokigunStudio Angolmois", 23, 20, 3, 0x4040c0);
	printstrc("\"the Simple BMS Player\"", 23, 80, 1, 0x8080c0);
	printstrc(version + 24, 27, 100, 1, 0x8080c0);
	printstrc("Original Character Design from", 30, 200, 1, 0xc0c080);
	printstrc("Project Angolmois", 17, 220, 2, 0x808040);
	printstrc("by", 2, 255, 1, 0xc0c080);
	printstrc("Choi Kaya (CHKY)", 16, 275, 2, 0x808040);
	printstrc("[ http://angolmois.net/ ]", 25, 310, 1, 0xc0c080);
	printstrc("Programmed & Obfuscated by", 26, 410, 1, 0xc0c080);
	printstrc("Kang Seonghoon (Tokigun)", 24, 430, 2, 0x808040);
	printstrc("[ http://tokigun.net/ ]", 23, 465, 1, 0xc0c080);
	printstrc("Graphics & Interface Design by", 30, 580, 1, 0xc0c080);
	printstrc("Kang Seonghoon (Tokigun)", 24, 600, 2, 0x808040);
	printstrc("Special Thanks to", 17, 800, 1, 0x80c080);
	printstrc("Park J. K. (mono*)", 18, 820, 2, 0x408040);
	printstrc("Park Jiin (Mithrandir)", 22, 855, 2, 0x408040);
	printstrc("Hye-Shik Chang (perky)", 22, 890, 2, 0x408040);
	printstrc("Greetings", 9, 1000, 1, 0x80c080);
	printstrc("Kang Junho (MysticMist)", 23, 1020, 2, 0x408040);
	printstrc("Joon-cheol Park (exman)", 23, 1055, 2, 0x408040);
	printstrc("Jae-kyun Lee (kida)", 19, 1090, 2, 0x408040);
	printstrc("Park Byeong-uk (Minan2DJ07)", 27, 1125, 2, 0x408040);
	printstrc("Park Jaesong (klutzy)", 21, 1160, 2, 0x408040);
	printstrc("HanIRC #tokigun, #perky", 23, 1195, 2, 0x408040);
	printstrc("ToEZ2DJ.net", 11, 1230, 2, 0x408040);
	printstrc("Powered by", 10, 1400, 1, 0x80c080);
	printstrc("SDL, SDL_mixer, SDL_image", 25, 1420, 2, 0x408040);
	printstrc("gVim, Python", 12, 1455, 2, 0x408040);
	printstrc("and", 3, 1490, 1, 0x408040);
	printstrc("DemiSoda Apple/Grape", 20, 1510, 2, 0x408040);
	printstrc("POCARISWEAT", 11, 1545, 2, 0x408040);
	printstrc("Shovel Works", 12, 1580, 2, 0x408040);
	printstrc("Copyright (c) 2005, Kang Seonghoon (Tokigun).", 45, 2060, 1, 0x808080);
	printstrc("This program is free software; you can redistribute it and/or", 61, 2090, 1, 0x808080);
	printstrc("modify it under the terms of the GNU General Public License", 59, 2110, 1, 0x808080);
	printstrc("as published by the Free Software Foundation; either version 2", 62, 2130, 1, 0x808080);
	printstrc("of the License, or (at your option) any later version.", 54, 2150, 1, 0x808080);
	printstrc("for more information, visit http://dev.tokigun.net/angolmois/.", 62, 2180, 1, 0xc0c0c0);
#undef printstrc

	SDL_FillRect(screen, 0, 0x000010);
	while(++t < 1820 && !check_exit()) {
		SDL_BlitSurface(credit, newrect(0,t,800,600), screen, 0);
		SDL_Flip(screen);
		SDL_Delay(20);
	}
	if(t == 1820) {
		SDL_FillRect(screen, newrect(0, 0, 800, 100), 0);
		SDL_Flip(screen);
		t = SDL_GetTicks() + 8000;
		while((int)SDL_GetTicks() < t && !check_exit());
	}
	
	SDL_FreeSurface(credit);
	finalize();
	return 0;
}

int main(int argc, char **argv) {
	char buf[512]={0,};
	int i, j, k, use_buf;
	
	if(argc < 2 || !*argv[1]) {
		use_buf = 1;
		if(!filedialog(buf)) use_buf = 0;
	} else {
		use_buf = 0;
	}
	for(i=2; i<argc; i++) {
		for(j=0; j<ARRAYSIZE(arglist); j++) {
			for(k=0; arglist[j][k] && argv[i][k]; k++)
				if((arglist[j][k]|32) != (argv[i][k]|32)) break;
			if(!arglist[j][k]) break;
		}
		switch(j) {
		case 0: /* viewer */
			opt_mode = 1;
			break;
		case 1: /* showinfo */
		case 2: /* hideinfo */
			opt_showinfo = (j==1);
			break;
		case 3: /* window */
		case 4: /* fullscreen */
			opt_fullscreen = (j==4);
			break;
		case 5: /* quality */
			opt_quality = atoi(argv[i]+k) + 7;
			break;
		case 6: /* x<speed> */
		case 7: /* speed<speed> */
			playspeed = atof(argv[i]+k);
			break;
		case 8: /* lntype<#> */
			lntype = atoi(argv[i]+k);
			break;
		}
	}
	if(argc > 1 || use_buf) {
		if(playspeed <= 0) playspeed = 1.0;
		if(playspeed < 0.1) playspeed = 0.1;
		if(playspeed > 99.0) playspeed = 99.0;
		if(opt_quality < 8) opt_quality = 8;
		if(opt_quality > 12) opt_quality = 12;
		bmspath = use_buf ? buf : argv[1];
		return play();
	} else {
		return credit();
	}
}

/* vim: set ts=4 sw=4: */
