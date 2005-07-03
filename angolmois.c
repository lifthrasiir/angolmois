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
 *
 *
 * phase 2 (2005-07-04)
 * optimized some routines, especially key2index(), panel position and #BGA directive.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <smpeg.h>

/******************************************************************************/
/* constants, variables */

char *version = "TokigunStudio Angolmois version 0.1 final (phase 2)";

#define SWAP(x,y,t) {(t)=(x);(x)=(y);(y)=(t);}

const char *bmsheader[] = {
	"title", "genre", "artist", "stagefile", "bpm",
	"player", "playlevel", "rank", "lntype", "lnobj",
	"wav", "bmp", "bga", "stop", "stp",
	"random", "if", "else", "endif"
};
char *bmspath, respath[512], **bmsline, *metadata[4], *sndpath[1296], *imgpath[1296];
double bpm=130, bpmtab[1296];
int nbmsline, nblitcmd, value[5]={1,0,2,1,0}, (*blitcmd)[8], stoptab[1296];
Mix_Chunk *sndres[1296]={0,};
SDL_Surface *imgres[1296]={0,};
SMPEG *mpeg=0;

typedef struct { double t; int m,i; } bmsnote;
bmsnote **channel[22]={0,};
double shorten[2005], length;
int nchannel[22]={0,};

SDL_Surface *screen, *sprite, *stagefile_tmp;
SDL_Event event;
SDL_Rect rect[2]; int _rect=0;

char*fontdata="#[RCm#a&#e#;S&e#;C$#;#e&e2Y#e#G#f:i&);V0a;*e%Y#&g7+CC2NP)=aOS=NP"
	"I2+C#fbC(Y;)S#)S/Y3&#hX[S)>OPV=Q<I>^#hh/S#&;#$mseNg#;S#hRe+g#;#&iG;O`YaDDB"
	">R0)jW;C$)a$e&mfh7jSe~a$p?hWir%;#e~C$'jpa/Y;J`YAV0Y;a#hs?S&./#g#b#lB1S$1S$"
	"YBkR&[T#&#kR1[D0I$e#b<C$htb$);C\\*mBebC\\Z;e#iRb0S#)S#)f#krF$f&mRF0Y2lb##h"
	"LkRl2S#hsepS#)/;SC$iReya$g%ibg>fFhra/Y;F$eUeyi2$X88e#A$G2hq+SD0eDb$eHineRf"
	"&F$itA/V;C$e#F0F*hr_<)f?F0I<_$hrb$e@a$fDb$q2C$mbJ0)kbf}b$gehraD$i#a$hr&#g#"
	"eHa#iRIT)AS$*AO<)iRg}f'b$irZ`ZBNP)lre}VPZ>J$k2?;&f>F0I0?#hra$fMa$fhhra#gAN"
	"`I2*/#mBlreBC\\V#&/Y;a#ib;#j#hrgafviBi2YS$%lBNPY=b<F0hr$0Y@aS$*a?Z;$$hr$1O"
	"<_#nrb0S#*[#*[#*;b$hr_#&i#_#hr##';SC$&)+#i0#S&&i#gB+SD0j{r(hrf^qBa/S#b/Y;F"
	"`&htC$);a$e0F0IBkBF0);C0Y;a#hy&#hPkRgBF`*;jB1OT);[*)f#jbgQb/#hdC$);a$gaF0)"
	"hreJgPjR&/#f&&#eQhtC$Y;IT)A[TI<iR?S#j#hb##C@bP)e#jr##CBe~f%jBa#h2gpmBa$);C"
	"$gub#hRb/S#&#jbe=i?gbC\\V#&#jb;CDB;C$)>OT)1#jrhqjBi2YS$%mBNPY=b<&jr$$e??;V"
	";$$nbV[$&;#&;htb@C&?C&?C`*gr1C$e#S$f'1#e~f/n&###h=gGhrSHbJfLu%aT&)";
Uint8 rawfont[16][96], (*zoomfont[3])[96]={rawfont,0,};

double playspeed=1, targetspeed, startoffset=-1, startshorten=1, gradefactor;
int origintime, starttime, stoptime, adjustspeed, xflag, xnnotes, xscore, xduration;
int pcur[22], prear[22], pcheck[18], thru[22], bga[3]={-1,-1}, poorbga, bga_updated=1;
int score, scocnt[5], scombo, smaxcombo, gradetime, grademode, gauge=256;
int opt_mode, opt_showinfo=1, opt_fullscreen=1, opt_random, opt_egg;

SDL_AudioSpec aformat;
int keymap[18]={122,115,120,100,99,304,308,102,118,109,107,44,108,46,303,307,59,47};
int keypressed[18], tkeyleft[18], tkeywidth[18], tpanel1, tpanel2, tbga;
int tkeycolor[18]={
	0x808080,0x8080ff,0x808080,0x8080ff,0x808080,0xff8080,0x80ff80,0x8080ff,0x808080,
	0x808080,0x8080ff,0x808080,0x8080ff,0x808080,0xff8080,0x80ff80,0x8080ff,0x808080};
char *tgradestr[5]={"MISS", "BAD", "GOOD", "GREAT", "COOL"};
int tgradecolor[5][2]={
	{0xff4040, 0xffc0c0}, {0xff40ff, 0xffc0ff}, {0xffff40, 0xffffc0},
	{0x40ff40, 0xc0ffc0}, {0x4040ff, 0xc0c0ff}};

/******************************************************************************/
/* system dependent functions */

char *adjust_path(char*); /* DUMMY */

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
	{ return((c|32)-19)/26-3?c:c|32; }
int stricmp(char *a, char *b)
	{ while(*a&&*b&&lcase(*a)==lcase(*b))a++,b++;return*a==*b; }

char *strcopy(char*); /* DUMMY */
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

int remove_whitespace(char *s, int *i) {
	while(is_space(s[*i])) ++*i;
	if(!s[*i]) return 0;
	for(s+=*i; *s; s++);
	while(is_space(*--s));
	*++s = 0;
	return 1;
}

char *strcopy(char *s) {
	char *t; int i=0;
	while(s[i++]);
	t = malloc(i);
	for(i=0; t[i]=s[i]; i++);
	return t;
}

/******************************************************************************/
/* bms parser */

int key2index(char*s){
	char a[3]={0,}; int b;
	*a=*s; a[1]=s[1]; b=strtol(a,&s,36);
	return *s?-1:b;
}

int compare_bmsline(const void *a, const void *b) {
	int i, j;
	for(i=0; i<6; i++)
		if(j = (*(char**)a)[i] - (*(char**)b)[i]) return j;
	return 0;
}

int compare_bmsnote(const void *a, const void *b) {
	bmsnote *A=*(bmsnote**)a, *B=*(bmsnote**)b;
	return (A->t > B->t ? 1 : A->t < B->t ? -1 : A->m - B->m);
}

void add_note(int i, double t, int m, int j) {
	bmsnote *x = malloc(sizeof(bmsnote));
	x->t = t; x->m = m; x->i = j;
	channel[i] = realloc(channel[i], sizeof(bmsnote*) * (nchannel[i]+1));
	channel[i][nchannel[i]++] = x;
}

void remove_note(int i, int j) {
	if(channel[i][j]) {
		if(i < 18 && channel[i][j]->i) {
			add_note(18, channel[i][j]->t, 0, channel[i][j]->i);
		}
		free(channel[i][j]);
		channel[i][j] = 0;
	}
}

int parse_bms() {
	FILE* fp;
	int i, j, k, a, b, c;
	int prev[36]={0,};
	int r=1, ignore=0;
	int measure, chan;
	double t;
	char *s, *z;
	
	if(fp = fopen(bmspath, "r")) {
		dirinit();
		for(s = malloc(1024); fgets(s, 1024, fp); s--) {
			if(*s++ == 35) {
				for(i=0; i<19; i++) {
					for(j=0; bmsheader[i][j]; j++)
						if((bmsheader[i][j]|32) != (s[j]|32)) break;
					if(!bmsheader[i][j]) break;
				}
				if(i == 15) {
					if(is_space(s[j]))
						if(j = abs(atoi(s+j)))
							r = rand() % j + 1;
				} else if(i == 16) {
					if(is_space(s[j]))
						ignore = (r != atoi(s+j));
				} else if(i == 17) {
					ignore = !ignore;
				} else if(i == 18) {
					ignore = 0;
				}
				
				if(!ignore) {
					if(i < 4) {
						if(remove_whitespace(s, &j)) {
							if(metadata[i]) free(metadata[i]);
							metadata[i] = strcopy(s+j);
						}
					} else if(i < 5) {
						if(is_space(s[j])) {
							bpm = atof(s+j);
						} else {
							if((i = key2index(s+j)) + 1 && is_space(s[j+2]))
								bpmtab[i] = atof(s+j+2);
						}
					} else if(i < 9) {
						if(is_space(s[j]))
							value[i-5] = atoi(s+j);
					} else if(i < 10) {
						if(is_space(s[j])) {
							while(is_space(s[++j]));
							if(s[j]) value[4] = key2index(s+j);
						}
					} else if(i < 11) {
						if((i = key2index(s+j)) + 1) {
							j += 2;
							if(remove_whitespace(s, &j)) {
								if(sndpath[i]) { free(sndpath[i]); sndpath[i] = 0; }
								sndpath[i] = strcopy(s+j);
							}
						}
					} else if(i < 12) {
						if((i = key2index(s+j)) + 1) {
							j += 2;
							if(remove_whitespace(s, &j)) {
								if(imgpath[i]) { free(imgpath[i]); imgpath[i] = 0; }
								imgpath[i] = strcopy(s+j);
							}
						}
					} else if(i < 13) {
						i = nblitcmd;
						blitcmd = realloc(blitcmd, sizeof(int) * 8 * (i+1));
						blitcmd[i][0] = key2index(s+j);
						if(is_space(s[j+=2]) && remove_whitespace(s, &j)) {
							blitcmd[i][1] = key2index(s+j);
							z = s + j + 2;
							for(j=2; *z && j<8; j++)
								blitcmd[i][j] = strtol(z, &z, 10);
							if(j < 8) blitcmd[i][0] = -1; else nblitcmd++;
						} else {
							blitcmd[i][0] = -1;
						}
					} else if(i < 14) {
						if((i = key2index(s+j)) + 1 && is_space(s[j+2]))
							stoptab[i] = atoi(s+j+2);
					} else if(i < 15) {
						if(sscanf(s+j, "%d.%d %d", &i, &j, &k) > 2)
							add_note(21, i+j/1e3, 1, k);
					}
					
					for(i=0; i<5; i++)
						if((s[i]-8)/10 != 4) break;
					if(i>4 && s[5]==58 && s[6]) {
						bmsline = realloc(bmsline, sizeof(char*) * (nbmsline+1));
						bmsline[nbmsline] = strcopy(s);
						nbmsline++;
					}
				}
			}
		}
		free(s);
		fclose(fp);

		qsort(bmsline, nbmsline, sizeof(char*), compare_bmsline);
		for(i=0; i<nbmsline; i++) {
			j = 0;
			for(k=0; k<5; k++)
				j = j * 10 + bmsline[i][k] - 48;
			measure = j / 100; chan = j % 100;
			if(chan == 2) {
				shorten[measure+1] = atof(bmsline[i]+6);
			} else {
				j = 6;
				remove_whitespace(bmsline[i], &j);
				for(k=j; bmsline[i][k]; k++);
				a = (k - j) / 2;
				for(k=0; k<a; k++,j+=2) {
					b = key2index(bmsline[i]+j);
					c = 8 + chan%10 - chan/10%2*9;
					t = measure + 1. * k / a;
					if(b) {
						if(chan == 1) {
							add_note(18, t, 0, b);
						} else if(chan == 3 && (b/36<16 && b%36<16)) {
							add_note(20, t, 0, b/36*16+b%36);
						} else if(chan == 4) {
							add_note(19, t, 0, b);
						} else if(chan == 6) {
							add_note(19, t, 2, b);
						} else if(chan == 7) {
							add_note(19, t, 1, b);
						} else if(chan == 8) {
							add_note(20, t, 1, b);
						} else if(chan == 9) {
							add_note(21, t, 0, b);
						} else if(chan % 10 != 0 && chan > 9 && chan < 30) {
							if(value[4] && b == value[4]) {
								if(nchannel[c] && channel[c][nchannel[c]-1]->m==0) {
									channel[c][nchannel[c]-1]->m = 3;
									add_note(c, t, 2, b);
								}
							} else {
								add_note(c, t, 0, b);
							}
						} else if(chan % 10 != 0 && chan > 29 && chan < 50) {
							add_note(c, t, 1, b);
						}
					}
					if(chan % 10 != 0 && chan > 49 && chan < 70) {
						if(value[3] == 1 && b) {
							if(prev[c]) {
								prev[c] = 0;
								add_note(c, t, 2, 0);
							} else {
								prev[c] = b;
								add_note(c, t, 3, b);
							}
						} else if(value[3] == 2) {
							if(prev[c] || prev[c] != b) {
								if(prev[c]) {
									if(prev[c+18] + 1 < measure) {
										add_note(c, prev[c+18]+1, 2, 0);
									} else if(prev[c] != b) {
										add_note(c, t, 2, 0);
									}
								}
								if(b && (prev[c]!=b || prev[c+18]+1<measure)) {
									add_note(c, t, 3, b);
								}
								prev[c+18] = measure;
								prev[c] = b;
							}
						}
					}
				}
			}
			free(bmsline[i]);
		}
		free(bmsline);
		length = measure + 2;
		for(i=0; i<18; i++) {
			if(prev[i]) {
				t = (value[3] == 2 && prev[i+18]+1 < measure ? prev[i+18]+1 : length-1);
				add_note(i/10*9+i%10, t, 2, 0);
			}
		}
		
		for(i=0; i<22; i++) {
			if(channel[i]) {
				qsort(channel[i], nchannel[i], sizeof(bmsnote*), compare_bmsnote);
				if(i != 18 && i < 21) {
					b = 0; t = -1;
					for(j=0; j<=nchannel[i]; j++) {
						if(j == nchannel[i] || channel[i][j]->t > t) {
							if(t >= 0) {
								c = 0;
								for(; k<j; k++) {
									r = channel[i][k]->m;
									if(i<18 ? (c & 1<<r) || (b ? (a&4)==0 || r<2 : r != ((a&12)==8 ? 3 : a&1 ? 0 : 1)) : i==19 ? c & 1<<r : c) {
										remove_note(i, k);
									} else {
										c |= 1 << r;
									}
								}
								b = (b ? (a&12)!=4 : (a&12)==8);
							}
							if(j < nchannel[i]) break;
							a = 0;
							k = j;
							t = channel[i][j]->t;
						}
						a |= 1 << channel[i][j]->m;
					}
					if(i<18 && b) {
						while(j >= 0 && !channel[i][--j]);
						if(j >= 0 && channel[i][j]->m == 3) remove_note(i, j);
					}
				}
				for(j=k=0; j<nchannel[i]; j++)
					if(channel[i][j]) channel[i][j-k] = channel[i][j]; else k++;
				nchannel[i] -= k;
			}
		}

		for(i=0; i<4; i++)
			if(!metadata[i]) {
				metadata[i] = malloc(1);
				*metadata[i] = 0;
			}
		for(i=0; i<2005; i++)
			if(shorten[i] <= .001) shorten[i] = 1;

		return 0;
	} else {
		return 1;
	}
}

SDL_Surface *newsurface(int f, int w, int h); /* DUMMY */
SDL_Rect *newrect(int x, int y, int w, int h); /* DUMMY */
int load_resource(void (*callback)(char*)) {
	SDL_Surface *temp;
	int i, j;

	for(i=0; i<1296; i++) {
		if(sndpath[i]) {
			if(callback) callback(sndpath[i]);
			sndres[i] = load_wav(sndpath[i]);
			for(j=0; sndpath[i][j]; j++);
			j = (j>3 ? *(int*)(sndpath[i]+j-4) : 0) | 0x20202020;
			if(j != 0x33706d2e && j != 0x2e6d7033) {
				free(sndpath[i]); sndpath[i] = 0;
			}
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
	
	callback("processing...");
	for(i=0; i<nblitcmd; i++) {
		if(blitcmd[i][0]>=0 && blitcmd[i][1]>=0 && imgres[blitcmd[i][1]]) {
			temp = imgres[blitcmd[i][0]];
			if(!temp) {
				imgres[blitcmd[i][0]] = temp = newsurface(SDL_SWSURFACE, 256, 256);
				SDL_FillRect(temp, 0, 0);
				SDL_SetColorKey(temp, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
			}
			blitcmd[i][2] *= (blitcmd[i][2] > 0);
			blitcmd[i][3] *= (blitcmd[i][3] > 0);
			if(blitcmd[i][4] > blitcmd[i][2] + 256) blitcmd[i][4] = blitcmd[i][2] + 256;
			if(blitcmd[i][5] > blitcmd[i][3] + 256) blitcmd[i][5] = blitcmd[i][3] + 256;
			SDL_BlitSurface(imgres[blitcmd[i][1]],
				newrect(blitcmd[i][2], blitcmd[i][3], blitcmd[i][4]-blitcmd[i][2], blitcmd[i][5]-blitcmd[i][3]),
				temp, newrect(blitcmd[i][6], blitcmd[i][7], 0, 0));
		}
	}
	free(blitcmd);
	
	return 0;
}

/*
bit 0: it uses 7 keys?
bit 1: it uses long-note?
bit 2: it uses pedal?
bit 3: it has bpm variation?
*/
void get_bms_info() {
	int i, j;
	
	xflag = xnnotes = 0;
	if(nchannel[7] || nchannel[8] || nchannel[16] || nchannel[17]) xflag |= 1;
	if(nchannel[6] || nchannel[15]) xflag |= 4;
	if(nchannel[20]) xflag |= 8;
	for(i=0; i<18; i++) {
		for(j=0; j<nchannel[i]; j++) {
			if(channel[i][j]->m > 1) xflag |= 2;
			if(channel[i][j]->m < 3) ++xnnotes;
		}
	}
	for(i=0; i<xnnotes; i++)
		xscore += (int)(300 * (1 + 1. * i / xnnotes));
}

double adjust_object_position(double base, double time); /* DUMMY */
int get_bms_duration() {
	int i, j, time, rtime, ttime;
	double pos, xbpm, tmp;

	xbpm = bpm;
	time = rtime = 0; pos = -1;
	while(1) {
		for(i=-1,j=0; j<22; j++)
			if(pcur[j] < nchannel[j] && (i < 0 || channel[j][pcur[j]]->t < channel[i][pcur[i]]->t)) i = j;
		if(i < 0) {
			time += (int)(adjust_object_position(pos, length) * 24e7 / xbpm);
			break;
		}
		time += (int)(adjust_object_position(pos, channel[i][pcur[i]]->t) * 24e7 / xbpm);
		j = channel[i][pcur[i]]->i; ttime = 0;
		if(i < 19) {
			if(sndres[j]) ttime = (int)(sndres[j]->alen / .1764);
		} else if(i == 20) {
			tmp = (channel[i][pcur[i]]->m ? bpmtab[j] : j);
			if(tmp > 0) {
				xbpm = tmp;
			} else if(tmp < 0) {
				time += (int)(adjust_object_position(-1, pos) * 24e7 / xbpm);
				break;
			}
		} else if(i == 21) {
			if(channel[i][pcur[i]]->m) time += j;
			else time += (int)(stoptab[j] * 125e4 * shorten[(int)(pos+1)] / xbpm);
		}
		if(rtime < time + ttime) rtime = time + ttime;
		pos = channel[i][pcur[i]]->t; pcur[i]++;
	}
	return (time > rtime ? time : rtime) / 1000;
}

void clone_bms() {
	bmsnote *ptr;
	int i, j;

	for(i=0; i<9; i++) {
		if(nchannel[i+9]) free(channel[i+9]);
		nchannel[i+9] = nchannel[i];
		channel[i+9] = malloc(sizeof(bmsnote*) * nchannel[i]);
		for(j=0; j<nchannel[i]; j++) {
			channel[i+9][j] = ptr = malloc(sizeof(bmsnote));
			ptr->t = channel[i][j]->t;
			ptr->m = channel[i][j]->m;
			ptr->i = channel[i][j]->i;
		}
	}
}

void shuffle_bms(int mode, int player) {
	bmsnote **tempchan, **result[18]={0,};
	int nresult[18]={0,};
	int map[18], perm[18], nmap;
	int ptr[18]={0,}, thru[18]={0,}, target[18];
	int thrumap[18], thrurmap[18];
	int i, j, flag=1, temp;
	double t;

	for(i=0; i<18; i++) map[i] = perm[i] = i;
	if(!nchannel[7] && !nchannel[8] && !nchannel[16] && !nchannel[17])
		map[7] = map[8] = map[16] = map[17] = -1;
	if(!nchannel[6] && !nchannel[15])
		map[6] = map[15] = -1;
	if(mode != 3 && mode != 5)
		map[5] = map[6] = map[14] = map[15] = -1;
	if(player) {
		for(i=0; i<9; i++)
			map[player==1 ? i+9 : i] = -1;
	}
	for(i=j=0; i<18; i++)
		if(map[i] < 0) j++;
		else map[i-j] = map[i];
	nmap = 18 - j;

	if(mode < 2) { /* mirror */
		for(i=0,j=nmap-1; i<j; i++,j--) {
			SWAP(channel[map[i]], channel[map[j]], tempchan);
			SWAP(nchannel[map[i]], nchannel[map[j]], temp);
		}
	} else if(mode < 4) { /* shuffle */
		for(i=nmap-1; i>0; i--) {
			j = rand() % i;
			SWAP(channel[map[i]], channel[map[j]], tempchan);
			SWAP(nchannel[map[i]], nchannel[map[j]], temp);
		}
	} else if(mode < 6) { /* random */
		while(flag) {
			for(i=nmap-1; i>0; i--) {
				j = rand() % i;
				SWAP(perm[i], perm[j], temp);
			}
			t = 1e4;
			flag = 0;
			for(i=0; i<nmap; i++) {
				if(ptr[map[i]] < nchannel[map[i]]) {
					flag = 1; target[i] = 1;
					if(t > channel[map[i]][ptr[map[i]]]->t)
						t = channel[map[i]][ptr[map[i]]]->t - 1e-4;
				} else {
					target[i] = 0;
				}
			}
			t += 2e-4;
			for(i=0; i<nmap; i++) {
				if(target[i] && channel[map[i]][ptr[map[i]]]->t > t)
					target[i] = 0;
			}
			for(i=0; i<nmap; i++) {
				if(target[i]) {
					temp = channel[map[i]][ptr[map[i]]]->m;
					if(temp == 2) {
						j = thrumap[i];
						thru[j] = 2;
					} else {
						j = perm[i];
						while(thru[j]) j = perm[thrurmap[j]];
						if(temp == 3) {
							thrumap[i] = j;
							thrurmap[j] = i;
							thru[j] = 1;
						}
					}
					result[j] = realloc(result[j], sizeof(bmsnote*) * (nresult[j]+1));
					result[j][nresult[j]++] = channel[map[i]][ptr[map[i]]++];
				}
			}
			for(i=0; i<nmap; i++)
				if(thru[i] == 2) thru[i] = 0;
		}
		for(i=0; i<nmap; i++) {
			free(channel[map[i]]);
			channel[map[i]] = result[i];
			nchannel[map[i]] = nresult[i];
		}
	}
}

/******************************************************************************/
/* general graphic functions */

int getpixel(SDL_Surface *s, int x, int y)
	{ return((Uint32*)s->pixels)[x+y*s->pitch/4]; }
int putpixel(SDL_Surface *s, int x, int y, int c)
	{ return((Uint32*)s->pixels)[x+y*s->pitch/4]=c; }
int blend(int x, int y, int a, int b)
	{ int i=0;for(;i<24;i+=8)y+=((x>>i&255)-(y>>i&255))*a/b<<i;return y; }
putblendedpixel(SDL_Surface *s, int x, int y, int c, int o)
	{ putpixel(s, x, y, blend(getpixel(s,x,y), c, o, 255)); }
SDL_Surface *newsurface(int f, int w, int h)
	{ return SDL_CreateRGBSurface(f,w,h,32,0xff0000,0xff00,0xff,0); }
SDL_Rect *newrect(int x, int y, int w, int h)
	{ SDL_Rect*r=rect+_rect++%2;r->x=x;r->y=y;r->w=w;r->h=h;return r; }

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
	int i, j, k, r, g, b, c, xx, yy, a[4][2], d;

	dx = x = 0;
	ww = src->w - 1; hh = src->h - 1;
	w = dest->w - 1; h = dest->h - 1;
	for(i=0; i<=w; i++) {
		dy = y = 0;
		for(j=0; j<=h; j++) {
			r = g = b = 0;
			for(k=0; k<4; k++) {
				a[k][0] = bicubic_kernel((x+k-1)*w-i*ww, w);
				a[k][1] = bicubic_kernel((y+k-1)*h-j*hh, h);
			}
			for(k=0; k<16; k++) {
				xx = x + k/4 - 1; yy = y + k%4 - 1;
				if(xx>=0 && xx<=ww && yy>=0 && yy<=hh) {
					c = getpixel(src, xx, yy); d = a[k/4][0] * a[k%4][1] >> 6;
					r += (c&255) * d; g += (c>>8&255) * d; b += (c>>16&255) * d;
				}
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

int _fontprocess(int x, int y, int z, int c, int s) {
	int i, j;
	for(i=0; i<z; i++)
		for(j=(s==1?z-i:s==3?i+1:0); j<(s==2?i:s==4?z-i-1:z); j++)
			zoomfont[z-1][(y*z+i)*z+j][c] |= 1<<(7-x);
	return z;
}

void fontprocess(int z) {
	int i, j, k, l, t[1596];
	if(z) {
		if(zoomfont[z-1]) return;
		zoomfont[z-1] = malloc(96*16*z*z);
		for(i=0; i<96; i++) {
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
		for(i=j=k=0; i<1536; fontdata++) {
			if(*fontdata > 98) {
				for(k=*fontdata++-97; --k; i++) t[i] = t[i-*fontdata+34];
			} else if(*fontdata > 34) {
				j |= (*fontdata - 35) << k*6%8;
				if(k++&3) { t[i++] = j & 0xff; j >>= 8; }
			}
		}
		for(i=0; i<1536; i++) rawfont[i%16][i/16] = t[i];
	}
}

void fontfinalize() {
	int i;
	for(i=1; i<3; i++)
		if(zoomfont[i]) free(zoomfont[i]);
}

int printchar(SDL_Surface *s, int x, int y, int z, int c, int u, int v) {
	int i, j;
	if(!is_space(c)) {
		c -= (c<0 ? -96 : c<33 || c>126 ? c : 32);
		for(i=0; i<16*z; i++)
			for(j=0; j<8*z; j++)
				if(zoomfont[z-1][i*z+j%z][c]&(1<<(7-j/z)))
					putpixel(s, x+j, y+i, blend(u, v, i, 16*z-1));
	}
	return 8*z;
}

void printstr(SDL_Surface *s, int x, int y, int z, char *c, int u, int v)
	{ while(*c)x+=printchar(s,x,y,z,(Uint8)*c++,u,v); }

/******************************************************************************/
/* main routines */

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
	screen = SDL_SetVideoMode(800, 600, 32, opt_fullscreen ? SDL_FULLSCREEN : SDL_SWSURFACE|SDL_DOUBLEBUF);
	if(!screen)
		return errormsg("SDL Video Initialization Failure: %s", SDL_GetError());
	SDL_ShowCursor(SDL_DISABLE);
	if(Mix_OpenAudio(aformat.freq=44100, aformat.format=MIX_DEFAULT_FORMAT, aformat.channels=2, 2048)<0)
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
	free(blitcmd);

	if(sprite) SDL_FreeSurface(sprite);
	if(mpeg) { SMPEG_stop(mpeg); SMPEG_delete(mpeg); }
	Mix_HookMusic(0, 0);
	Mix_CloseAudio();
	fontfinalize();
	dirfinal();
}

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
			value[1], bpm, "?"+((xflag&8)==0), xnnotes,
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
	for(i=0; i<18; i++) {
		tkeyleft[i] = -1;
		tkeywidth[i] = ((i+2)%9>6 ? 40 : 25);
	}
	for(i=0; i<5; i++) tkeyleft[i] = 41 + 26 * i;
	tkeyleft[5] = 0; tpanel1 = 171; tpanel2 = 0;
	if(xflag & 1) { tkeyleft[7] = 171; tkeyleft[8] = 197; tpanel1 += 52; }
	if(xflag & 4) { tkeyleft[6] = tpanel1; tpanel1 += 41; }
	if(value[0] > 1) {
		for(i=0; i<9; i++)
			if(tkeyleft[i] >= 0) {
				if(i == 5) tkeyleft[14] = 760;
				else if(i == 6) tkeyleft[15] = 760 - tkeyleft[6];
				else tkeyleft[i+9] = tkeyleft[i] + (xflag&1 ? 537 : 589);
			}
		tpanel2 = 800 - tpanel1;
		if(value[0] == 3) {
			for(i=9; i<18; i++) tkeyleft[i] += tpanel1 - tpanel2;
			tpanel1 += 801 - tpanel2; tpanel2 = 0;
		}
	}
	tbga = ((tpanel2 ? tpanel2 : 800) - tpanel1) / 2;
	
	/* sprite */
	sprite = newsurface(SDL_SWSURFACE, 1200, 600);
	for(i=0; i<18; i++) {
		if(tkeyleft[i] >= 0) {
			for(j=140; j<520; j++)
				SDL_FillRect(sprite, newrect(tkeyleft[i],j,tkeywidth[i],1), blend(tkeycolor[i], 0, j-140, 1000));
			if(i < 9) {
				for(j=0; j*2<tkeywidth[i]; j++)
					SDL_FillRect(sprite, newrect(800+tkeyleft[i]+j,0,tkeywidth[i]-2*j,600), blend(tkeycolor[i], 0xffffff, tkeywidth[i]-j, tkeywidth[i]));
			}
		}
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
	if(!tpanel2 && !opt_mode) {
		SDL_FillRect(sprite, newrect(0,584,368,16), 0x404040);
		SDL_FillRect(sprite, newrect(4,588,360,8), 0);
	}
	SDL_FillRect(sprite, newrect(10,564,tpanel1,1), 0x404040);

	/* screen */
	SDL_FillRect(screen, 0, 0);
	SDL_BlitSurface(sprite, newrect(0,0,800,30), screen, newrect(0,0,0,0));
	SDL_BlitSurface(sprite, newrect(0,520,800,80), screen, newrect(0,520,0,0));

	/* configuration */
	origintime = starttime = SDL_GetTicks();
	targetspeed = playspeed;
	Mix_AllocateChannels(128);
	for(i=0; i<22; pcur[i++]=0);
	gradefactor = 1.5 - value[2] / 4.;
}

double adjust_object_time(double base, double offset) {
	int i = (int)(base+1);
	if((i - base) * shorten[i] > offset)
		return base + offset / shorten[i];
	offset -= (i - base) * shorten[i];
	while(shorten[++i] <= offset)
		offset -= shorten[i];
	return i - 1 + offset / shorten[i];
}

double adjust_object_position(double base, double time) {
	int i = (int)(base+1), j = (int)(time+1);
	base = (time - j + 1) * shorten[j] - (base - i + 1) * shorten[i];
	while(i < j) base += shorten[i++];
	return base;
}

int play_process() {
	int i, j, k, l, m, t, ibottom;
	double bottom, top, tmp;
	char buf[99];

	if(adjustspeed) {
		tmp = targetspeed - playspeed;
		if(-0.001 < tmp && tmp < 0.001) {
			adjustspeed = 0; playspeed = targetspeed;
			for(i=0; i<22; i++) prear[i] = pcur[i];
		} else {
			playspeed += tmp * 0.015;
		}
	}

	t = SDL_GetTicks();
	if(t < stoptime) {
		bottom = startoffset;
	} else if(stoptime) {
		starttime = t;
		stoptime = 0;
		bottom = startoffset;
	} else {
		bottom = startoffset + (t - starttime) * bpm / startshorten / 24e4;
	}
	ibottom = (int)(bottom + 1);
	if(bottom > -1 && startshorten != shorten[ibottom]) {
		starttime += (int)((ibottom - startoffset - 1) * 24e4 * startshorten / bpm);
		startoffset = ibottom - 1;
		startshorten = shorten[ibottom];
	}
	top = adjust_object_time(bottom, 1.25/playspeed);
	for(i=0; i<22; i++) {
		while(prear[i] < nchannel[i] && channel[i][prear[i]]->t <= top) prear[i]++;
		while(pcur[i] < nchannel[i] && channel[i][pcur[i]]->t < bottom) {
			j = channel[i][pcur[i]]->i;
			if(i == 18) {
				if(sndres[j]) {
					j = Mix_PlayChannel(-1, sndres[j], 0);
					if(j >= 0) { Mix_Volume(j, 96); Mix_GroupChannel(j, 1); }
				} else if(sndpath[j]) {
					if(!mpeg || SMPEG_status(mpeg) != SMPEG_PLAYING) {
						if(mpeg) {
							Mix_HookMusic(0, 0);
							SMPEG_delete(mpeg);
						}
						if(mpeg = SMPEG_new(sndpath[j], 0, 0)) {
							SMPEG_actualSpec(mpeg, &aformat);
							Mix_HookMusic(SMPEG_playAudioSDL, mpeg);
							SMPEG_enableaudio(mpeg, 1);
							SMPEG_play(mpeg);
						}
					}
				}
			} else if(i == 19) {
				bga[channel[i][pcur[i]]->m] = j;
				bga_updated = 1;
			} else if(i == 20) {
				if(tmp = (channel[i][pcur[i]]->m ? bpmtab[j] : j)) {
					starttime = t;
					startoffset = bottom;
					bpm = tmp;
				}
			} else if(i == 21) {
				if(t >= stoptime) stoptime = t;
				if(channel[i][pcur[i]]->m) {
					stoptime += j;
				} else {
					stoptime += (int)(stoptab[j] * 1250 * startshorten / bpm);
				}
				startoffset = bottom;
			} else if(opt_mode && channel[i][pcur[i]]->m != 1 && sndres[j]) {
				j = Mix_PlayChannel(-1, sndres[j], 0);
				if(j >= 0) Mix_GroupChannel(j, 0);
			}
			pcur[i]++;
		}
		if(i<18 && !opt_mode) {
			for(; pcheck[i] < pcur[i]; pcheck[i]++) {
				j = channel[i][pcheck[i]]->m;
				if(j >= 0 && j - 1 && (j - 2 || thru[i])) {
					tmp = channel[i][pcheck[i]]->t;
					tmp = (bottom - tmp) * shorten[(int)(tmp+1)] / bpm * gradefactor;
					if(tmp > 6e-4) {
						scocnt[0]++; scombo = 0; grademode = 0; gauge -= 12;
						poorbga = t + 600; gradetime = t + 700;
					} else {
						break;
					}
				}
			}
		}
	}

	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			return -1;
		} else if(event.type == SDL_KEYUP) {
			if(event.key.keysym.sym == SDLK_ESCAPE) {
				return -1;
			} else if(!opt_mode) {
				for(i=0; i<18; i++) {
					if(tkeyleft[i] >= 0 && event.key.keysym.sym == keymap[i]) {
						keypressed[i] = 0;
						if(nchannel[i] && thru[i]) {
							for(j=pcur[i]+1; channel[i][j]->m != 2; j--);
							thru[i] = 0;
							tmp = (channel[i][j]->t - bottom) * shorten[ibottom] / bpm * gradefactor;
							if(-6e-4 < tmp && tmp < 6e-4) {
								channel[i][j]->m ^= -1;
							} else {
								scocnt[0]++; scombo = 0; grademode = 0; gauge -= 12;
								poorbga = t + 600; gradetime = t + 700;
							}
						}
					}
				}
			}
		} else if(event.type == SDL_KEYDOWN) {
			if(event.key.keysym.sym == SDLK_F3) {
				adjustspeed = 1;
				if(targetspeed > 20) targetspeed -= 5;
				else if(targetspeed > 10) targetspeed -= 1;
				else if(targetspeed > 1) targetspeed -= .5;
				else if(targetspeed > .201) targetspeed -= .2;
				else adjustspeed = 0;
			} else if(event.key.keysym.sym == SDLK_F4) {
				adjustspeed = 1;
				if(targetspeed < 1) targetspeed += .2;
				else if(targetspeed < 10) targetspeed += .5;
				else if(targetspeed < 20) targetspeed += 1;
				else if(targetspeed < 95) targetspeed += 5;
				else adjustspeed = 0;
			} else if(!opt_mode) {
				for(i=0; i<18; i++) {
					if(tkeyleft[i] >= 0 && event.key.keysym.sym == keymap[i]) {
						keypressed[i] = 1;
						if(nchannel[i]) {
							j = (pcur[i] < 1 || (pcur[i] < nchannel[i] && channel[i][pcur[i]-1]->t + channel[i][pcur[i]]->t < 2*bottom) ? pcur[i] : pcur[i]-1);
							if(sndres[channel[i][j]->i]) {
								l = Mix_PlayChannel(-1, sndres[channel[i][j]->i], 0);
								if(l >= 0) Mix_GroupChannel(l, 0);
							}
							if(j < pcur[i]) {
								while(j >= 0 && channel[i][j]->m == 1) j--;
								if(j < 0) continue;
							} else {
								while(j < nchannel[i] && channel[i][j]->m == 1) j++;
								if(j == nchannel[i]) continue;
							}
							if(pcur[i] < nchannel[i] && channel[i][pcur[i]]->m == 2) {
								scocnt[0]++; scombo = 0; grademode = 0; gauge -= 12;
								poorbga = t + 600; gradetime = t + 700;
							} else if(channel[i][j]->m != 2) {
								tmp = (channel[i][j]->t - bottom) * shorten[ibottom] / bpm * gradefactor;
								if(tmp < 0) tmp *= -1;
								if(channel[i][j]->m >= 0 && tmp < 6e-4) {
									if(channel[i][j]->m == 3) thru[i] = 1;
									channel[i][j]->m ^= -1;
									if(tmp < 0.6e-4) grademode = 4;
									else if(tmp < 2.0e-4) grademode = 3;
									else if(tmp < 3.5e-4) grademode = 2;
									else grademode = 1;
									scocnt[grademode]++; gradetime = t + 700;
									score += (int)((300 - tmp * 5e5) * (1 + 1. * scombo / xnnotes));
									if(grademode > 2) {
										scombo++; gauge += (grademode<4 ? 3 : 5) + scombo / 100;
									} else if(grademode < 2) {
										scombo = 0; gauge -= 5;
									}
									if(scombo > smaxcombo) smaxcombo = scombo;
								}
							}
						}
					}
				}
			}
		}
	}
	if(bottom > length) {
		i = (!mpeg || SMPEG_status(mpeg) != SMPEG_PLAYING);
		j = (opt_mode ? Mix_Playing(-1)==0 : Mix_GroupNewer(1)==-1);
		if(i && j) return 0;
	} else if(bottom < -1) {
		return 0;
	}

	SDL_FillRect(screen, newrect(0,30,tpanel1,490), 0x404040);
	if(tpanel2) SDL_FillRect(screen, newrect(tpanel2,30,800-tpanel2,490), 0x404040);
	for(i=0; i<18; i++) {
		if(tkeyleft[i] >= 0) {
			SDL_FillRect(screen, newrect(tkeyleft[i],30,tkeywidth[i],490), 0);
			if(keypressed[i]) {
				SDL_BlitSurface(sprite, newrect(tkeyleft[i],140,tkeywidth[i],380), screen, newrect(tkeyleft[i],140,0,0));
			}
		}
	}
	SDL_SetClipRect(screen, newrect(0,30,800,490));
	for(i=0; i<18; i++) {
		if(tkeyleft[i] >= 0) {
			m = 0;
			for(j=pcur[i]; j<prear[i]; j++) {
				k = (int)(525 - 400 * playspeed * adjust_object_position(bottom, channel[i][j]->t));
				if(channel[i][j]->m == 3) {
					l = k + 5;
					k = (int)(530 - 400 * playspeed * adjust_object_position(bottom, channel[i][++j]->t));
					if(k < 30) k = 30;
				} else if(channel[i][j]->m == 2) {
					k += 5;
					l = 520;
				} else if(channel[i][j]->m == 0) {
					l = k + 5;
				} else {
					continue;
				}
				if(k > 0 && l > k) {
					SDL_BlitSurface(sprite, newrect(800+tkeyleft[i%9],0,tkeywidth[i%9],l-k), screen, newrect(tkeyleft[i],k,0,0));
				}
				m++;
			}
			if(!m && prear[i]<nchannel[i] && channel[i][prear[i]]->m==2) {
				SDL_BlitSurface(sprite, newrect(800+tkeyleft[i%9],0,tkeywidth[i%9],490), screen, newrect(tkeyleft[i],30,0,0));
			}
		}
	}
	for(i=ibottom-1; i<top; i++) {
		j = (int)(530 - 400 * playspeed * adjust_object_position(bottom, i));
		SDL_FillRect(screen, newrect(0,j,tpanel1,1), 0xc0c0c0);
		if(tpanel2) SDL_FillRect(screen, newrect(tpanel2,j,800-tpanel2,1), 0xc0c0c0);
	}
	if(opt_egg) {
		for(i=pcur[20]; i<prear[20]; i++) {
			j = (int)(530 - 400 * playspeed * adjust_object_position(bottom, channel[20][i]->t));
			SDL_FillRect(screen, newrect(0,j,tpanel1,1), 0xffff00);
		}
		for(i=pcur[21]; i<prear[21]; i++) {
			j = (int)(530 - 400 * playspeed * adjust_object_position(bottom, channel[21][i]->t));
			SDL_FillRect(screen, newrect(0,j,tpanel1,1), 0x00ff00);
		}
	}
	if(t < gradetime) {
		for(i=0; tgradestr[grademode][i]; i++);
		printstr(screen, tpanel1/2-8*i, 292, 2, tgradestr[grademode],
				tgradecolor[grademode][0], tgradecolor[grademode][1]);
		if(scombo > 1) {
			i = sprintf(buf, "%d COMBO", scombo);
			printstr(screen, tpanel1/2-4*i, 320, 1, buf, 0x808080, 0xffffff);
		}
		if(!grademode) bga_updated = 1;
	}
	SDL_SetClipRect(screen, 0);
	if(bga_updated > 0 || (bga_updated < 0 && t >= poorbga)) {
		SDL_FillRect(screen, newrect(tbga,172,256,256), 0);
		if(t < poorbga) {
			if(bga[2] >= 0 && imgres[bga[2]])
				SDL_BlitSurface(imgres[bga[2]], newrect(0,0,256,256), screen, newrect(tbga,172,0,0));
			bga_updated = -1;
		} else {
			for(i=0; i<2; i++)
				if(bga[i] >= 0 && imgres[bga[i]])
					SDL_BlitSurface(imgres[bga[i]], newrect(0,0,256,256), screen, newrect(tbga,172,0,0));
			bga_updated = 0;
		}
	}

	i = (t - origintime) / 1000; j = xduration / 1000;
	sprintf(buf, "SCORE %07d%c%4.1fx%c%02d:%02d / %02d:%02d%c@%9.4f%cBPM %6.2f",
			score, 0, targetspeed, 0, i/60, i%60, j/60, j%60, 0, bottom, 0, bpm);
	SDL_BlitSurface(sprite, newrect(0,0,800,30), screen, newrect(0,0,0,0));
	SDL_BlitSurface(sprite, newrect(0,520,800,80), screen, newrect(0,520,0,0));
	printstr(screen, 10, 8, 1, buf, 0, 0);
	printstr(screen, 5, 522, 2, buf+14, 0, 0);
	printstr(screen, tpanel1-94, 565, 1, buf+20, 0, 0x404040);
	printstr(screen, 95, 538, 1, buf+34, 0, 0);
	printstr(screen, 95, 522, 1, buf+45, 0, 0);
	i = (t - origintime) * tpanel1 / xduration;
	printchar(screen, 6+(i<tpanel1?i:tpanel1), 548, 1, -1, 0x404040, 0x404040);
	if(!tpanel2 && !opt_mode) {
		if(gauge > 512) gauge = 512;
		i = (gauge<0 ? 0 : (gauge*400>>9) - (int)(160*startshorten*(1+bottom)) % 40);
		SDL_FillRect(screen, newrect(4,588,i>360?360:i<5?5:i,8), 0xc00000);
	}

	SDL_Flip(screen);
	return 1;
}

/******************************************************************************/
/* entry point */

int play() {
	int i;
	if(initialize()) return 1;
	if(parse_bms()) {
		finalize(); SDL_Quit();
		return *bmspath && errormsg("Couldn't load BMS file: %s", bmspath);
	}
	if(*value == 4) clone_bms();
	if(opt_random) {
		shuffle_bms(opt_random, *value!=3);
		if(*value%2==0) shuffle_bms(opt_random, 2);
	}
	get_bms_info();
	play_show_stagefile();
	xduration = get_bms_duration();
	play_prepare();
	while((i = play_process())>0);
	finalize();
	if(!opt_mode && i == 0) {
		if(gauge > 150) {
			printf("*** CLEARED! ***\n");
			for(i=4; i>=0; i--)
				printf("%-5s %4d    %s", tgradestr[i], scocnt[i], "\n"+(i!=2));
			printf("MAX COMBO %d\nSCORE %07d (max %07d)\n", smaxcombo, score, xscore);
		} else {
			printf("YOU FAILED!\n");
		}
	}
	return 0;
}

int credit() {
	SDL_Surface *credit;
	char *s[] = {
		"TokigunStudio Angolmois", "\"the Simple BMS Player\"", version + 24,
		"Original Character Design from", "Project Angolmois", "by", "Choi Kaya (CHKY)", "[ http://angolmois.net/ ]",
		"Programmed & Obfuscated by", "Kang Seonghoon (Tokigun)", "[ http://tokigun.net/ ]",
		"Graphics & Interface Design by", "Kang Seonghoon (Tokigun)",
		"Special Thanks to", "Park J. K. (mono*)", "Park Jiin (Mithrandir)", "Hye-Shik Chang (perky)",
		"Greetings", "Kang Junho (MysticMist)", "Joon-cheol Park (exman)",
		"Jae-kyun Lee (kida)", "Park Byeong-uk (Minan2DJ07)",
		"Park Jaesong (klutzy)", "HanIRC #tokigun, #perky", "ToEZ2DJ.net",
		"Powered by", "SDL, SDL_mixer, SDL_image", "gVim, Python", "and",
		"DemiSoda Apple/Grape", "POCARISWEAT", "Shovel Works",
		"Copyright (c) 2005, Kang Seonghoon (Tokigun).",
		"This program is free software; you can redistribute it and/or",
		"modify it under the terms of the GNU General Public License",
		"as published by the Free Software Foundation; either version 2",
		"of the License, or (at your option) any later version.",
		"for more information, visit http://dev.tokigun.net/angolmois/.",
	};
	char f[] = "DNNTITITTITTIQFFFQFFFFFFFQFFEFFFKKKKKW";
	int y[] = {
		20, 80, 100, 200, 220, 255, 275, 310, 410, 430, 465, 580, 600, 800,
		820, 855, 890, 1000, 1020, 1055, 1090, 1125, 1160, 1195, 1230, 1400,
		1420, 1455, 1490, 1510, 1545, 1580, 2060, 2090, 2110, 2130, 2150, 2180
	};
	int c[] = {0x4040c0, 0x408040, 0x808040, 0x808080, 0x8080c0, 0x80c080, 0xc0c080, 0xc0c0c0};
	int i, j, t = -750;

	opt_fullscreen = 0;
	if(initialize()) return 1;
	credit = newsurface(SDL_SWSURFACE, 800, 2200);
	SDL_FillRect(credit, 0, 0x000010);
	SDL_FillRect(credit, newrect(0, 1790, 800, 410), 0);
	for(i=1; i<16; i++)
		SDL_FillRect(credit, newrect(0, 1850-i*5, 800, 5), i);
	for(i=0; i<38; i++) {
		for(j=0; s[i][j]; j++);
		printstr(credit, 400-j*(f[i]%3+1)*4, y[i], f[i]%3+1, s[i], c[f[i]/3-22], 0xffffff);
	}

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
	int i, j, use_buf;

	use_buf = (argc<2 || !*argv[1] ? !!filedialog(buf) : 0);
	if(argc > 2) {
		playspeed = atof(argv[2]);
		if(playspeed <= 0) playspeed = 1;
		if(playspeed < .1) playspeed = .1;
		if(playspeed > 99) playspeed = 99;
	}
	if(argc > 3) {
		for(j=0; i=argv[3][j]; j++) {
			if((i|32) == 'v') opt_mode = 1;
			else if((i|32) == 'i') opt_showinfo = i&32;
			else if((i|32) == 'w') opt_fullscreen = !(i&32);
			else if((i|32) == 'm') opt_random = 1;
			else if((i|32) == 's') opt_random = (i=='s' ? 2 : 3);
			else if((i|32) == 'r') opt_random = (i=='r' ? 4 : 5);
			else if(j == 42 && i == '*') opt_egg = 1;
		}
	}
	while(--argc > 3) {
		if(argc < 22 && (i = atoi(argv[argc]))) keymap[argc-4] = i;
	}

	srand(time(0));
	if(argc > 1 || use_buf) {
		bmspath = use_buf ? buf : argv[1];
		return play();
	} else {
		return credit();
	}
}

/* vim: set ts=4 sw=4: */
