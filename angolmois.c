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
 * phase 3 (2005-07-05)
 * renamed important variables.
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

char *version = "TokigunStudio Angolmois version 0.1 final (phase 3)";

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
bmsnote **C[22]={0,};
double shorten[2005], length;
int N[22]={0,};

SDL_Surface *s, *S, *ss;
SDL_Event event;
SDL_Rect rect[2]; int _rect=0;

Uint8*Fd="#[RCm#a&#e#;S&e#;C$#;#e&e2Y#e#G#f:i&);V0a;*e%Y#&g7+CC2NP)=aOS=NPI2+C#"
    "fbC(Y;)S#)S/Y3&#hX[S)>OPV=Q<I>^#hh/S#&;#$mseNg#;S#hRe+g#;#&iG;O`YaDDB>R0)j"
    "W;C$)a$e&mfh7jSe~a$p?hWir%;#e~C$'jpa/Y;J`YAV0Y;a#hs?S&./#g#b#lB1S$1S$YBkR&"
    "[T#&#kR1[D0I$e#b<C$htb$);C\\*mBebC\\Z;e#iRb0S#)S#)f#krF$f&mRF0Y2lb##hLkRl2"
    "S#hsepS#)/;SC$iReya$g%ibg>fFhra/Y;F$eUeyi2$X88e#A$G2hq+SD0eDb$eHineRf&F$it"
    "A/V;C$e#F0F*hr_<)f?F0I<_$hrb$e@a$fDb$q2C$mbJ0)kbf}b$gehraD$i#a$hr&#g#eHa#i"
    "RIT)AS$*AO<)iRg}f'b$irZ`ZBNP)lre}VPZ>J$k2?;&f>F0I0?#hra$fMa$fhhra#gAN`I2*/"
    "#mBlreBC\\V#&/Y;a#ib;#j#hrgafviBi2YS$%lBNPY=b<F0hr$0Y@aS$*a?Z;$$hr$1O<_#nr"
    "b0S#*[#*[#*;b$hr_#&i#_#hr##';SC$&)+#i0#S&&i#gB+SD0j{r(hrf^qBa/S#b/Y;F`&htC"
    "$);a$e0F0IBkBF0);C0Y;a#hy&#hPkRgBF`*;jB1OT);[*)f#jbgQb/#hdC$);a$gaF0)hreJg"
    "PjR&/#f&&#eQhtC$Y;IT)A[TI<iR?S#j#hb##C@bP)e#jr##CBe~f%jBa#h2gpmBa$);C$gub#"
    "hRb/S#&#jbe=i?gbC\\V#&#jb;CDB;C$)>OT)1#jrhqjBi2YS$%mBNPY=b<&jr$$e??;V;$$nb"
    "V[$&;#&;htb@C&?C&?C`*gr1C$e#S$f'1#e~f/n&###h=gGhrSHbJfLu%aT&)";
Uint8 Fr[16][96], (*Fz[3])[96]={Fr,0,};

double playspeed=1, targetspeed, startoffset=-1, startshorten=1, gradefactor;
int origintime, starttime, stoptime, adjustspeed, xflag, xnnotes, xscore, xduration;
int P[22], prear[22], pcheck[18], thru[22], bga[3]={-1,-1}, poorbga, bga_updated=1;
int score, scocnt[5], scombo, smaxcombo, gradetime, grademode, gauge=256;
int o[5]={0,1,1};

SDL_AudioSpec aformat;
int keymap[18]={122,115,120,100,99,304,308,102,118,109,107,44,108,46,303,307,59,47};
int keypressed[18], tl[18], tw[18], tc[18], t1, t2, ta;
char *tgradestr[5]={"MISS", "BAD", "GOOD", "GREAT", "COOL"};
int tgradecolor[5][2]={
	{0xff4040, 0xffc0c0}, {0xff40ff, 0xffc0ff}, {0xffff40, 0xffffc0},
	{0x40ff40, 0xc0ffc0}, {0x4040ff, 0xc0c0ff}};

/******************************************************************************/
/* general functions */

int is_space(int n) { return!(n-9&&n-10&&n-13&&n-32); }

char *adjust_path(char *p) {
	extern int sep;
	int i=0, j=0;
 
	if(*p-47 && *p-92) {
		for(; bmspath[i]; i++)
			if((respath[i] = bmspath[i]) == sep) j = i + 1;
	}
	for(; *p; p++) respath[j++] = (*p-47 && *p-92 ? *p : sep);
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

void decompress(char *s, char *t) {
	int i, j;
	for(i=j=0; *s; s++) {
		if(*s > 98) {
			for(j=*s++-97; --j; t++) *t = t[34-*s];
		} else if(*s > 34) {
			i |= (*s - 35) << j*6%8;
			if(j++&3) { *t++ = i & 0xff; i >>= 8; }
		}
	}
}

/******************************************************************************/
/* system dependent functions */

#ifdef WIN32
#include <windows.h>
char sep = 92;
int filedialog(char *s) {
	char O[207];
	OPENFILENAME o={76,0,0,O,0,0,0,s,512,0,0,0,"Choose a file to play",
		OFN_HIDEREADONLY,0,0,0,0,0,0};
	decompress("$T)>C+7<PW7@VHY;C/X>X,Z;H$E4LT9<CCE-Q+9>VP&f(H$g(OH%#s5##}]iQ#7"
		"'AW8I>G8)<C#|NH$h}H$#6R\\Y<Q`)@H$}NO$hzO$33OT)geV$ffM#fdM###",O);
	return GetOpenFileName(&o);
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
/* bms parser */

int key2index(char*s){
	char a[3]={0,}; int b;
	*a=*s; a[1]=s[1]; b=strtol(a,&s,36);
	return *s?-1:b;
}

int compare_bmsline(const void *a, const void *b) {
	int i, j;
	for(i=0; i<6; i++)
		if(j = i[*(char**)a] - i[*(char**)b]) return j;
	return 0;
}

int compare_bmsnote(const void *a, const void *b) {
	bmsnote *A=*(bmsnote**)a, *B=*(bmsnote**)b;
	return (A->t > B->t ? 1 : A->t < B->t ? -1 : A->m - B->m);
}

void add_note(int i, double t, int m, int j) {
	bmsnote *x = malloc(sizeof(bmsnote));
	x->t = t; x->m = m; x->i = j;
	C[i] = realloc(C[i], sizeof(bmsnote*) * (N[i]+1));
	C[i][N[i]++] = x;
}

void remove_note(int i, int j) {
	if(C[i][j]) {
		if(i < 18 && C[i][j]->i) {
			add_note(18, C[i][j]->t, 0, C[i][j]->i);
		}
		free(C[i][j]);
		C[i][j] = 0;
	}
}

int parse_bms() {
	FILE* fp;
	int i, j, k, a, b, c, p[36]={0,}, r=1, g=0, x, y;
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
						g = (r != atoi(s+j));
				} else if(i == 17) {
					g = !g;
				} else if(i == 18) {
					g = 0;
				}
				
				if(!g) {
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
					
					strtol(s, &z, 10);
					if(s+5==z && s[5]==58 && s[6]) {
						bmsline = realloc(bmsline, sizeof(char*) * ++nbmsline);
						bmsline[nbmsline-1] = strcopy(s);
					}
				}
			}
		}
		free(s);
		fclose(fp);

		qsort(bmsline, nbmsline, sizeof(char*), compare_bmsline);
		for(i=0; i<nbmsline; i++) {
			x = atoi(bmsline[i]); y = x % 100; x /= 100;
			if(y == 2) {
				shorten[x+1] = atof(bmsline[i]+6);
			} else {
				j = 6;
				remove_whitespace(bmsline[i], &j);
				for(k=j; bmsline[i][k]; k++);
				a = (k - j) / 2;
				for(k=0; k<a; k++,j+=2) {
					b = key2index(bmsline[i]+j);
					c = 8 + y%10 - y/10%2*9;
					t = x + 1. * k / a;
					if(b) {
						if(y == 1) {
							add_note(18, t, 0, b);
						} else if(y == 3 && (b/36<16 && b%36<16)) {
							add_note(20, t, 0, b/36*16+b%36);
						} else if(y == 4) {
							add_note(19, t, 0, b);
						} else if(y == 6) {
							add_note(19, t, 2, b);
						} else if(y == 7) {
							add_note(19, t, 1, b);
						} else if(y == 8) {
							add_note(20, t, 1, b);
						} else if(y == 9) {
							add_note(21, t, 0, b);
						} else if(y % 10 != 0 && y > 9 && y < 30) {
							if(value[4] && b == value[4]) {
								if(N[c] && C[c][N[c]-1]->m==0) {
									C[c][N[c]-1]->m = 3;
									add_note(c, t, 2, b);
								}
							} else {
								add_note(c, t, 0, b);
							}
						} else if(y % 10 != 0 && y > 29 && y < 50) {
							add_note(c, t, 1, b);
						}
					}
					if(y % 10 != 0 && y > 49 && y < 70) {
						if(value[3] == 1 && b) {
							if(p[c]) {
								p[c] = 0;
								add_note(c, t, 2, 0);
							} else {
								p[c] = b;
								add_note(c, t, 3, b);
							}
						} else if(value[3] == 2) {
							if(p[c] || p[c] != b) {
								if(p[c]) {
									if(p[c+18] + 1 < x) {
										add_note(c, p[c+18]+1, 2, 0);
									} else if(p[c] != b) {
										add_note(c, t, 2, 0);
									}
								}
								if(b && (p[c]!=b || p[c+18]+1<x)) {
									add_note(c, t, 3, b);
								}
								p[c+18] = x;
								p[c] = b;
							}
						}
					}
				}
			}
			free(bmsline[i]);
		}
		free(bmsline);
		length = x + 2;
		for(i=0; i<18; i++)
			if(p[i]) {
				t = (value[3] == 2 && p[i+18]+1 < x ? p[i+18]+1 : length-1);
				add_note(i/10*9+i%10, t, 2, 0);
			}
		
		for(i=0; i<22; i++)
			if(C[i]) {
				qsort(C[i], N[i], sizeof(bmsnote*), compare_bmsnote);
				if(i != 18 && i < 21) {
					b = 0; t = -1;
					for(j=0; j<=N[i]; j++) {
						if(j == N[i] || C[i][j]->t > t) {
							if(t >= 0) {
								c = 0;
								for(; k<j; k++) {
									r = C[i][k]->m;
									if(i<18 ? (c & 1<<r) || (b ? (a&4)==0 || r<2 : r != ((a&12)==8 ? 3 : a&1 ? 0 : 1)) : i==19 ? c & 1<<r : c) {
										remove_note(i, k);
									} else {
										c |= 1 << r;
									}
								}
								b = (b ? (a&12)!=4 : (a&12)==8);
							}
							if(j < N[i]) break;
							a = 0;
							k = j;
							t = C[i][j]->t;
						}
						a |= 1 << C[i][j]->m;
					}
					if(i<18 && b) {
						while(j >= 0 && !C[i][--j]);
						if(j >= 0 && C[i][j]->m == 3) remove_note(i, j);
					}
				}
				for(j=k=0; j<N[i]; j++)
					if(C[i][j]) C[i][j-k] = C[i][j]; else k++;
				N[i] -= k;
			}

		for(i=0; i<4; i++)
			if(!metadata[i]) {
				metadata[i] = malloc(1);
				*metadata[i] = 0;
			}
		for(i=0; i<2005; i++)
			if(shorten[i] <= .001) shorten[i] = 1;

		return 0;
	} 
	return 1;
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

/*
bit 0: it uses 7 keys?
bit 1: it uses long-note?
bit 2: it uses pedal?
bit 3: it has bpm variation?
*/
void get_bms_info() {
	int i, j;
	
	xflag = xnnotes = 0;
	if(N[7] || N[8] || N[16] || N[17]) xflag |= 1;
	if(N[6] || N[15]) xflag |= 4;
	if(N[20]) xflag |= 8;
	for(i=0; i<18; i++) {
		for(j=0; j<N[i]; j++) {
			if(C[i][j]->m > 1) xflag |= 2;
			if(C[i][j]->m < 3) ++xnnotes;
		}
	}
	for(i=0; i<xnnotes; i++)
		xscore += (int)(300 * (1 + 1. * i / xnnotes));
}

int get_bms_duration() {
	int i, j, t=0, r=0, d;
	double p=-1, b=bpm, q;

	while(1) {
		for(i=-1,j=0; j<22; j++)
			if(P[j] < N[j] && (i < 0 || C[j][P[j]]->t < C[i][P[i]]->t)) i = j;
		if(i < 0) {
			t += (int)(adjust_object_position(p, length) * 24e7 / b);
			break;
		}
		t += (int)(adjust_object_position(p, C[i][P[i]]->t) * 24e7 / b);
		j = C[i][P[i]]->i; d = 0;
		if(i < 19) {
			if(sndres[j]) d = (int)(sndres[j]->alen / .1764);
		} else if(i == 20) {
			q = (C[i][P[i]]->m ? bpmtab[j] : j);
			if(q > 0) {
				b = q;
			} else if(q < 0) {
				t += (int)(adjust_object_position(-1, p) * 24e7 / b);
				break;
			}
		} else if(i == 21) {
			if(C[i][P[i]]->m) t += j;
			else t += (int)(stoptab[j] * 125e4 * shorten[(int)(p+1)] / b);
		}
		if(r < t + d) r = t + d;
		p = C[i][P[i]]->t; P[i]++;
	}
	return (t > r ? t : r) / 1000;
}

void clone_bms() {
	bmsnote *p;
	int i, j;

	for(i=0; i<9; i++) {
		if(N[i+9]) free(C[i+9]);
		N[i+9] = N[i];
		C[i+9] = malloc(sizeof(bmsnote*) * N[i]);
		for(j=0; j<N[i]; j++) {
			C[i+9][j] = p = malloc(sizeof(bmsnote));
			p->t = C[i][j]->t; p->m = C[i][j]->m; p->i = C[i][j]->i;
		}
	}
}

void shuffle_bms(int x, int y) {
	bmsnote **s, **r[18]={0,};
	int n[18]={0,}, m[18], p[18], o, q[18]={0,}, u[18]={0,}, g[18], v[18], w[18], i, j, k, f=1;
	double t;

	for(i=0; i<18; i++) m[i] = p[i] = i;
	if(!N[7] && !N[8] && !N[16] && !N[17])
		m[7] = m[8] = m[16] = m[17] = -1;
	if(!N[6] && !N[15])
		m[6] = m[15] = -1;
	if(x != 3 && x != 5)
		m[5] = m[6] = m[14] = m[15] = -1;
	if(y)
		for(i=0; i<9; i++)
			m[y-1 ? i : i+9] = -1;
	for(i=j=0; i<18; i++)
		if(m[i] < 0) j++; else m[i-j] = m[i];
	o = 18 - j;

	if(x < 2) { /* mirror */
		for(i=0,j=o-1; i<j; i++,j--) {
			SWAP(C[m[i]], C[m[j]], s);
			SWAP(N[m[i]], N[m[j]], k);
		}
	} else if(x < 4) { /* shuffle */
		for(i=o-1; i>0; i--) {
			j = rand() % i;
			SWAP(C[m[i]], C[m[j]], s);
			SWAP(N[m[i]], N[m[j]], k);
		}
	} else if(x < 6) { /* random */
		while(f) {
			for(i=o-1; i>0; i--) {
				j = rand() % i;
				SWAP(p[i], p[j], k);
			}
			t = 9e9;
			for(f=i=0; i<o; i++) {
				if(q[m[i]] < N[m[i]]) {
					f = 1; g[i] = 1;
					if(t > C[m[i]][q[m[i]]]->t)
						t = C[m[i]][q[m[i]]]->t - 1e-9;
				} else {
					g[i] = 0;
				}
			}
			t += 2e-9;
			for(i=0; i<o; i++)
				if(g[i] && C[m[i]][q[m[i]]]->t > t) g[i] = 0;
			for(i=0; i<o; i++) {
				if(g[i]) {
					k = C[m[i]][q[m[i]]]->m;
					if(k == 2) {
						j = v[i];
						u[j] = 2;
					} else {
						j = p[i];
						while(u[j]) j = p[w[j]];
						if(k == 3) {
							v[i] = j;
							w[j] = i;
							u[j] = 1;
						}
					}
					r[j] = realloc(r[j], sizeof(bmsnote*) * (n[j]+1));
					r[j][n[j]++] = C[m[i]][q[m[i]]++];
				}
			}
			for(i=0; i<o; i++)
				if(u[i] == 2) u[i] = 0;
		}
		for(i=0; i<o; i++) {
			free(C[m[i]]);
			C[m[i]] = r[i];
			N[m[i]] = n[i];
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
	{ putpixel(s,x,y,blend(getpixel(s,x,y),c,o,255)); }
SDL_Surface *newsurface(int f, int w, int h)
	{ return SDL_CreateRGBSurface(f,w,h,32,255<<16,65280,255,0); }
SDL_Rect *R(int x, int y, int w, int h)
	{ SDL_Rect*r=rect+_rect++%2;r->x=x;r->y=y;r->w=w;r->h=h;return r; }

int bicubic_kernel(int x, int y) {
	return (x<y ?
			((y*y - 2*x*x + x*x/y*x) << 11)/y/y :
			x<y*2 ?
			((4*y*y - 8*x*y + 5*x*x - x*x/y*x) << 11)/y/y : 0);
}

bicubic_interpolation(SDL_Surface *s, SDL_Surface *d) {
	int x, y, u, v, w, h, i, j, k, l, r, g, b, a, c, p[4], q[4];

	w = d->w - 1; h = d->h - 1;
	for(i=u=x=0; i<=w; i++) {
		v = y = 0;
		for(k=x; k<x+4; k++)
			p[k-x] = bicubic_kernel(abs(k*w+i-w-i*s->w), w);
		for(j=0; j<=h; j++) {
			r = g = b = 0;
			for(l=y; l<y+4; l++)
				q[l-y] = bicubic_kernel(abs(l*h+j-h-j*s->h), h);
			for(k=x; k<x+4; k++)
				for(l=y; l<y+4; l++)
					if(k>0 && k<=s->w && l>0 && l<=s->h) {
						c = getpixel(s, k-1, l-1); a = p[k-x] * q[l-y] >> 6;
						r += (c&255) * a; g += (c>>8&255) * a; b += (c>>16&255) * a;
					}
			putpixel(d, i, j,
				(r<0 ? 0 : r>>24 ? 255 : r>>16) |
				(g<0 ? 0 : g>>24 ? 255 : g>>16)<<8 |
				(b<0 ? 0 : b>>24 ? 255 : b>>16)<<16);
			if((v+=s->h-1) > h) { y++; v -= h; }
		}
		if((u+=s->w-1) > w) { x++; u -= w; }
	}
	SDL_FreeSurface(s);
}

/******************************************************************************/
/* font functions */

int FP(int x, int y, int z, int c, int s) {
	int i, j;
	for(i=0; i<z; i++)
		for(j=(s==1?z-i:s==3?i+1:0); j<(s==2?i:s==4?z-i-1:z); j++)
			Fz[z-1][(y*z+i)*z+j][c] |= 1<<(7-x);
	return z;
}

void Fp(int z) {
	int i=0, j, k, l; char t[1536];
	if(z) {
		if(Fz[z-1]) return;
		for(Fz[z-1]=malloc(1536*z*z); i<96; i++) {
			for(j=0; j<16*z*z; j++)
				Fz[z-1][j][i] = 0;
			for(j=0; j<16; j++)
				for(k=0; k<8; k++) {
					l = (j>0?Fr[j-1][i]<<k>>6&7:0)<<6 |
						(Fr[j][i]<<k>>6&7)<<3 |
						(j<15?Fr[j+1][i]<<k>>6&7:0);
					if((i==3 || i==20) && k<2) l |= (l & 146) << 1;
					if((i==3 || i==20) && k>6) l |= (l & 146) >> 1;
					if(l & 16) {
						FP(k, j, z, i, 0);
					} else {
						if((l & 218) == 10 || (l & 63) == 11) /* /| */
							FP(k, j, z, i, 1);
						if((l & 434) == 34 || (l & 63) == 38) /* |\ */
							FP(k, j, z, i, 2);
						if((l & 155) == 136 || (l & 504) == 200) /* \| */
							FP(k, j, z, i, 3);
						if((l & 182) == 160 || (l & 504) == 416) /* |/ */
							FP(k, j, z, i, 4);
					}
				}
		}
	} else {
		for(decompress(Fd, t); i<1536; i++) Fr[i%16][i/16] = t[i];
	}
}

int F_(SDL_Surface *s, int x, int y, int z, int c, int u, int v) {
	int i, j;
	if(!is_space(c)) {
		c -= c<0 ? -96 : c<33 || c>126 ? c : 32;
		for(i=0; i<16*z; i++)
			for(j=0; j<8*z; j++)
				if(Fz[z-1][i*z+j%z][c]&(1<<(7-j/z)))
					putpixel(s, x+j, y+i, blend(u, v, i, 16*z-1));
	}
	return 8*z;
}

void F(SDL_Surface *s, int x, int y, int z, char *c, int u, int v)
	{ while(*c)x+=F_(s,x,y,z,(Uint8)*c++,u,v); }

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
	s = SDL_SetVideoMode(800, 600, 32, o[2] ? SDL_FULLSCREEN : SDL_SWSURFACE|SDL_DOUBLEBUF);
	if(!s)
		return errormsg("SDL Video Initialization Failure: %s", SDL_GetError());
	SDL_ShowCursor(SDL_DISABLE);
	if(Mix_OpenAudio(aformat.freq=44100, aformat.format=MIX_DEFAULT_FORMAT, aformat.channels=2, 2048)<0)
		return errormsg("SDL Mixer Initialization Failure: %s", Mix_GetError());
	SDL_WM_SetCaption(version, 0);

	Fp(0);
	Fp(2);
	Fp(3);
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
		if(C[i]) {
			for(j=0; j<N[i]; j++)
				free(C[i][j]);
			free(C[i]);
		}
	}
	free(blitcmd);

	if(S) SDL_FreeSurface(S);
	if(ss) SDL_FreeSurface(ss);
	if(mpeg) { SMPEG_stop(mpeg); SMPEG_delete(mpeg); }
	Mix_HookMusic(0, 0);
	Mix_CloseAudio();
	for(i=1; i<3; i++)
		if(Fz[i]) free(Fz[i]);
	dirfinal();
}

int callback_resource(char *path) {
	int i;
	for(i=0; path[i]; i++);
	SDL_BlitSurface(ss, R(0,0,800,20), s, R(0,580,800,20));
	F(s, 797-8*i, 582, 1, path, 0x808080, 0xc0c0c0);
	SDL_Flip(s);
	return check_exit();
}

int load_resource() {
	SDL_Surface *t;
	int i, j, *r;

	for(i=0; i<1296; i++) {
		if(sndpath[i]) {
			if(o[1] && callback_resource(sndpath[i])) return 1;
			sndres[i] = load_wav(sndpath[i]);
			for(j=0; sndpath[i][j]; j++);
			j = (j>3 ? *(int*)(sndpath[i]+j-4) : 0) | 0x20202020;
			if(j != 0x33706d2e && j != 0x2e6d7033) {
				free(sndpath[i]); sndpath[i] = 0;
			}
		}
		if(imgpath[i]) {
			if(o[1] && callback_resource(imgpath[i])) return 1;
			if(t = load_image(imgpath[i])) {
				imgres[i] = SDL_DisplayFormat(t);
				SDL_FreeSurface(t);
				SDL_SetColorKey(imgres[i], SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
			}
			free(imgpath[i]); imgpath[i] = 0;
		}
	}
	
	if(o[1] && callback_resource("loading...")) return 1;
	for(i=0; i<nblitcmd; i++) {
		r = blitcmd[i];
		if(*r>=0 && r[1]>=0 && imgres[r[1]]) {
			t = imgres[*r];
			if(!t) {
				imgres[*r] = t = newsurface(SDL_SWSURFACE, 256, 256);
				SDL_FillRect(t, 0, 0);
				SDL_SetColorKey(t, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
			}
			r[2] *= (r[2] > 0); r[3] *= (r[3] > 0);
			if(r[4] > r[2] + 256) r[4] = r[2] + 256;
			if(r[5] > r[3] + 256) r[5] = r[3] + 256;
			SDL_BlitSurface(imgres[r[1]], R(r[2], r[3], r[4]-r[2], r[5]-r[3]), t, R(r[6], r[7], 0, 0));
		}
	}
	free(blitcmd);

	return 0;
}

int play_show_stagefile() {
	SDL_Surface *u;
	char c[256];
	int i, j, t;

	sprintf(c, "%s: %s - %s", version, metadata[2], metadata[0]);
	SDL_WM_SetCaption(c, 0);
	F(s, 248, 284, 2, "loading bms file...", 0x202020, 0x808080);
	SDL_Flip(s);

	ss = newsurface(SDL_SWSURFACE, 800, 20);
	if(metadata[3] && (u = IMG_Load(adjust_path(metadata[3])))) {
		bicubic_interpolation(SDL_DisplayFormat(u), s);
		SDL_FreeSurface(u);
	}
	if(o[1]) {
		for(i=0; i<800; i++) {
			for(j=0; j<42; j++) putblendedpixel(s, i, j, 0x101010, 64);
			for(j=580; j<600; j++) putblendedpixel(s, i, j, 0x101010, 64);
		}
		F(s, 6, 4, 2, metadata[0], 0x808080, 0xffffff);
		for(i=0; metadata[1][i]; i++);
		for(j=0; metadata[2][j]; j++);
		F(s, 792-8*i, 4, 1, metadata[1], 0x808080, 0xffffff);
		F(s, 792-8*j, 20, 1, metadata[2], 0x808080, 0xffffff);
		sprintf(c, "Level %d | BPM %.2f%s | %d note%s [%dKEY%s]",
			value[1], bpm, "?"+((xflag&8)==0), xnnotes,
			"s"+(xnnotes==1), (xflag&1) ? 7 : 5, (xflag&2) ? "-LN" : "");
		F(s, 3, 582, 1, c, 0x808080, 0xffffff);
		SDL_BlitSurface(s, R(0,580,800,20), ss, R(0,0,800,20));
	}
	SDL_Flip(s);

	t = SDL_GetTicks() + 3000;
	if(load_resource()) return 1;
	while((int)SDL_GetTicks() < t && !check_exit());
	return 0;
}

void play_prepare() {
	int i, j, c;

	/* panel position */
	for(i=0; i<18; i++) {
		tl[i] = -1;
		tw[i] = (i+2)%9>6 ? 40 : 25;
		tc[i] = 0x808080 | 255 << 16 >> (i%9-5 ? i%9-6 ? 24-i%9%2*8 : 8 : 0);
	}
	for(i=0; i<5; i++) tl[i] = 41 + 26 * i;
	tl[5] = 0; t1 = 171; t2 = 0;
	if(xflag & 1) { tl[7] = 171; tl[8] = 197; t1 += 52; }
	if(xflag & 4) { tl[6] = t1; t1 += 41; }
	if(value[0] > 1) {
		for(i=0; i<9; i++)
			if(tl[i] >= 0) {
				if(i == 5) tl[14] = 760;
				else if(i == 6) tl[15] = 760 - tl[6];
				else tl[i+9] = tl[i] + (xflag&1 ? 537 : 589);
			}
		t2 = 800 - t1;
		if(value[0] == 3) {
			for(i=9; i<18; i++) tl[i] += t1 - t2;
			t1 += 801 - t2; t2 = 0;
		}
	}
	ta = ((t2 ? t2 : 800) - t1) / 2;
	
	/* sprite */
	S = newsurface(SDL_SWSURFACE, 1200, 600);
	for(i=0; i<18; i++) {
		if(tl[i] >= 0) {
			for(j=140; j<520; j++)
				SDL_FillRect(S, R(tl[i],j,tw[i],1), blend(tc[i], 0, j-140, 1000));
			if(i < 9) {
				for(j=0; j*2<tw[i]; j++)
					SDL_FillRect(S, R(800+tl[i]+j,0,tw[i]-2*j,600), blend(tc[i], 0xffffff, tw[i]-j, tw[i]));
			}
		}
	}
	for(j=-244; j<556; j++) {
		for(i=-10; i<20; i++) {
			c = (i*2+j*3+750) % 2000;
			c = blend(0xc0c0c0, 0x606060, c>1000 ? 1850-c : c-150, 700);
			putpixel(S, j+244, i+10, c);
		}
		for(i=-20; i<60; i++) {
			c = (i*3+j*2+750) % 2000;
			c = blend(0xc0c0c0, 0x404040, c>1000 ? 1850-c : c-150, 700);
			putpixel(S, j+244, i+540, c);
		}
	}
	SDL_FillRect(S, R(t1+20,0,(t2?t2:820)-t1-40,600), 0);
	for(i=0; i<20; i++) {
		for(j=20; j*j+i*i>400; j--) {
			putpixel(S, t1+j, i+10, 0);
			putpixel(S, t1+j, 539-i, 0);
			if(t2) {
				putpixel(S, t2-j-1, i+10, 0);
				putpixel(S, t2-j-1, 539-i, 0);
			}
		}
	}
	if(!t2 && !*o) {
		SDL_FillRect(S, R(0,584,368,16), 0x404040);
		SDL_FillRect(S, R(4,588,360,8), 0);
	}
	SDL_FillRect(S, R(10,564,t1,1), 0x404040);

	/* screen */
	SDL_FillRect(s, 0, 0);
	SDL_BlitSurface(S, R(0,0,800,30), s, R(0,0,0,0));
	SDL_BlitSurface(S, R(0,520,800,80), s, R(0,520,0,0));

	/* configuration */
	origintime = starttime = SDL_GetTicks();
	targetspeed = playspeed;
	Mix_AllocateChannels(128);
	for(i=0; i<22; P[i++]=0);
	gradefactor = 1.5 - value[2] / 4.;
}

int play_process() {
	int i, j, k, l, m, t, z;
	double x, y, u;
	char c[99];

	if(adjustspeed) {
		u = targetspeed - playspeed;
		if(-.001 < u && u < .001) {
			adjustspeed = 0; playspeed = targetspeed;
			for(i=0; i<22; i++) prear[i] = P[i];
		} else {
			playspeed += u * .015;
		}
	}

	t = SDL_GetTicks();
	if(t < stoptime) {
		x = startoffset;
	} else if(stoptime) {
		starttime = t;
		stoptime = 0;
		x = startoffset;
	} else {
		x = startoffset + (t - starttime) * bpm / startshorten / 24e4;
	}
	z = (int)(x + 1);
	if(x > -1 && startshorten != shorten[z]) {
		starttime += (int)((z - startoffset - 1) * 24e4 * startshorten / bpm);
		startoffset = z - 1;
		startshorten = shorten[z];
	}
	y = adjust_object_time(x, 1.25/playspeed);
	for(i=0; i<22; i++) {
		while(prear[i] < N[i] && C[i][prear[i]]->t <= y) prear[i]++;
		while(P[i] < N[i] && C[i][P[i]]->t < x) {
			j = C[i][P[i]]->i;
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
						if(mpeg = SMPEG_new(adjust_path(sndpath[j]), 0, 0)) {
							SMPEG_actualSpec(mpeg, &aformat);
							Mix_HookMusic(SMPEG_playAudioSDL, mpeg);
							SMPEG_enableaudio(mpeg, 1);
							SMPEG_play(mpeg);
						}
					}
				}
			} else if(i == 19) {
				bga[C[i][P[i]]->m] = j;
				bga_updated = 1;
			} else if(i == 20) {
				if(u = (C[i][P[i]]->m ? bpmtab[j] : j)) {
					starttime = t;
					startoffset = x;
					bpm = u;
				}
			} else if(i == 21) {
				if(t >= stoptime) stoptime = t;
				if(C[i][P[i]]->m) {
					stoptime += j;
				} else {
					stoptime += (int)(stoptab[j] * 1250 * startshorten / bpm);
				}
				startoffset = x;
			} else if(*o && C[i][P[i]]->m != 1 && sndres[j]) {
				j = Mix_PlayChannel(-1, sndres[j], 0);
				if(j >= 0) Mix_GroupChannel(j, 1);
			}
			P[i]++;
		}
		if(i<18 && !*o) {
			for(; pcheck[i] < P[i]; pcheck[i]++) {
				j = C[i][pcheck[i]]->m;
				if(j >= 0 && j - 1 && (j - 2 || thru[i])) {
					u = C[i][pcheck[i]]->t;
					u = (x - u) * shorten[(int)(u+1)] / bpm * gradefactor;
					if(u > 6e-4) {
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
		k = event.key.keysym.sym;
		if(event.type == SDL_QUIT) {
			return 2;
		} else if(event.type == SDL_KEYUP) {
			if(k == SDLK_ESCAPE) {
				return 2;
			} else if(!*o) {
				for(i=0; i<18; i++) {
					if(tl[i] >= 0 && k == keymap[i]) {
						keypressed[i] = 0;
						if(N[i] && thru[i]) {
							for(j=P[i]+1; C[i][j]->m != 2; j--);
							thru[i] = 0;
							u = (C[i][j]->t - x) * shorten[z] / bpm * gradefactor;
							if(-6e-4 < u && u < 6e-4) {
								C[i][j]->m ^= -1;
							} else {
								scocnt[0]++; scombo = 0; grademode = 0; gauge -= 12;
								poorbga = t + 600; gradetime = t + 700;
							}
						}
					}
				}
			}
		} else if(event.type == SDL_KEYDOWN) {
			if(adjustspeed = k == SDLK_F3) {
				if(targetspeed > 20) targetspeed -= 5;
				else if(targetspeed > 10) targetspeed -= 1;
				else if(targetspeed > 1) targetspeed -= .5;
				else if(targetspeed > .201) targetspeed -= .2;
				else adjustspeed = 0;
			} else if(adjustspeed = k == SDLK_F4) {
				if(targetspeed < 1) targetspeed += .2;
				else if(targetspeed < 10) targetspeed += .5;
				else if(targetspeed < 20) targetspeed += 1;
				else if(targetspeed < 95) targetspeed += 5;
				else adjustspeed = 0;
			} else if(!*o) {
				for(i=0; i<18; i++) {
					if(tl[i] >= 0 && k == keymap[i]) {
						keypressed[i] = 1;
						if(N[i]) {
							j = (P[i] < 1 || (P[i] < N[i] && C[i][P[i]-1]->t + C[i][P[i]]->t < 2*x) ? P[i] : P[i]-1);
							if(sndres[C[i][j]->i] && (l = Mix_PlayChannel(-1, sndres[C[i][j]->i], 0)) >= 0) Mix_GroupChannel(l, 0);
							if(j < P[i]) {
								while(j >= 0 && C[i][j]->m == 1) j--;
								if(j < 0) continue;
							} else {
								while(j < N[i] && C[i][j]->m == 1) j++;
								if(j == N[i]) continue;
							}
							if(P[i] < N[i] && C[i][P[i]]->m == 2) {
								scocnt[0]++; scombo = 0; grademode = 0; gauge -= 12;
								poorbga = t + 600; gradetime = t + 700;
							} else if(C[i][j]->m != 2) {
								u = (C[i][j]->t - x) * shorten[z] / bpm * gradefactor * 1e5;
								if(C[i][j]->m >= 0 && (u = u<0 ? -u : u) < 60) {
									if(C[i][j]->m == 3) thru[i] = 1;
									C[i][j]->m ^= -1;
									if(u < 6) grademode = 4;
									else if(u < 20) grademode = 3;
									else if(u < 35) grademode = 2;
									else grademode = 1;
									scocnt[grademode]++; gradetime = t + 700;
									score += (int)((300 - u * 5) * (1 + 1. * scombo / xnnotes));
									if(grademode > 2) {
										if(++scombo > smaxcombo) smaxcombo++;
										gauge += (grademode<4 ? 3 : 5) + scombo / 100;
									} else if(grademode < 2) {
										scombo = 0; gauge -= 5;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if(x > length) {
		if((!mpeg || SMPEG_status(mpeg) != SMPEG_PLAYING) && Mix_GroupNewer(1)<0) return 1;
	} else if(x < -1) {
		return 1;
	}

	SDL_FillRect(s, R(0,30,t1,490), 0x404040);
	if(t2) SDL_FillRect(s, R(t2,30,800-t2,490), 0x404040);
	for(i=0; i<18; i++) {
		if(tl[i] >= 0) {
			SDL_FillRect(s, R(tl[i],30,tw[i],490), 0);
			if(keypressed[i]) SDL_BlitSurface(S, R(tl[i],140,tw[i],380), s, R(tl[i],140,0,0));
		}
	}
	SDL_SetClipRect(s, R(0,30,800,490));
	for(i=0; i<18; i++) {
		if(tl[i] >= 0) {
			m = 0;
			for(j=P[i]; j<prear[i]; j++) {
				k = (int)(525 - 400 * playspeed * adjust_object_position(x, C[i][j]->t));
				if(C[i][j]->m == 3) {
					l = k + 5;
					k = (int)(530 - 400 * playspeed * adjust_object_position(x, C[i][++j]->t));
					if(k < 30) k = 30;
				} else if(C[i][j]->m == 2) {
					k += 5;
					l = 520;
				} else if(C[i][j]->m == 0) {
					l = k + 5;
				} else {
					continue;
				}
				if(k > 0 && l > k) {
					SDL_BlitSurface(S, R(800+tl[i%9],0,tw[i%9],l-k), s, R(tl[i],k,0,0));
				}
				m++;
			}
			if(!m && prear[i]<N[i] && C[i][prear[i]]->m==2)
				SDL_BlitSurface(S, R(800+tl[i%9],0,tw[i%9],490), s, R(tl[i],30,0,0));
		}
	}
	for(i=z-1; i<y; i++) {
		j = (int)(530 - 400 * playspeed * adjust_object_position(x, i));
		SDL_FillRect(s, R(0,j,t1,1), 0xc0c0c0);
		if(t2) SDL_FillRect(s, R(t2,j,800-t2,1), 0xc0c0c0);
	}
	if(o[4]) {
		for(i=P[20]; i<prear[20]; i++) {
			j = (int)(530 - 400 * playspeed * adjust_object_position(x, C[20][i]->t));
			SDL_FillRect(s, R(0,j,t1,1), 0xffff00);
		}
		for(i=P[21]; i<prear[21]; i++) {
			j = (int)(530 - 400 * playspeed * adjust_object_position(x, C[21][i]->t));
			SDL_FillRect(s, R(0,j,t1,1), 0x00ff00);
		}
	}
	if(t < gradetime) {
		for(i=0; tgradestr[grademode][i]; i++);
		F(s, t1/2-8*i, 292, 2, tgradestr[grademode],
				tgradecolor[grademode][0], tgradecolor[grademode][1]);
		if(scombo > 1) {
			i = sprintf(c, "%d COMBO", scombo);
			F(s, t1/2-4*i, 320, 1, c, 0x808080, 0xffffff);
		}
		if(!grademode) bga_updated = 1;
	}
	SDL_SetClipRect(s, 0);
	if(bga_updated > 0 || (bga_updated < 0 && t >= poorbga)) {
		SDL_FillRect(s, R(ta,172,256,256), 0);
		if(t < poorbga) {
			if(bga[2] >= 0 && imgres[bga[2]])
				SDL_BlitSurface(imgres[bga[2]], R(0,0,256,256), s, R(ta,172,0,0));
			bga_updated = -1;
		} else {
			for(i=0; i<2; i++)
				if(bga[i] >= 0 && imgres[bga[i]])
					SDL_BlitSurface(imgres[bga[i]], R(0,0,256,256), s, R(ta,172,0,0));
			bga_updated = 0;
		}
	}

	i = (t - origintime) / 1000; j = xduration / 1000;
	sprintf(c, "SCORE %07d%c%4.1fx%c%02d:%02d / %02d:%02d%c@%9.4f%cBPM %6.2f",
			score, 0, targetspeed, 0, i/60, i%60, j/60, j%60, 0, x, 0, bpm);
	SDL_BlitSurface(S, R(0,0,800,30), s, R(0,0,0,0));
	SDL_BlitSurface(S, R(0,520,800,80), s, R(0,520,0,0));
	F(s, 10, 8, 1, c, 0, 0);
	F(s, 5, 522, 2, c+14, 0, 0);
	F(s, t1-94, 565, 1, c+20, 0, 0x404040);
	F(s, 95, 538, 1, c+34, 0, 0);
	F(s, 95, 522, 1, c+45, 0, 0);
	i = (t - origintime) * t1 / xduration;
	F_(s, 6+(i<t1?i:t1), 548, 1, -1, 0x404040, 0x404040);
	if(!t2 && !*o) {
		if(gauge > 512) gauge = 512;
		i = (gauge<0 ? 0 : (gauge*400>>9) - (int)(160*startshorten*(1+x)) % 40);
		SDL_FillRect(s, R(4,588,i>360?360:i<5?5:i,8), 0xc00000);
	}

	SDL_Flip(s);
	return 0;
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
	if(o[3]) {
		shuffle_bms(o[3], *value!=3);
		if(*value%2==0) shuffle_bms(o[3], 2);
	}
	get_bms_info();
	if(play_show_stagefile()) {
		i = 0;
	} else {
		xduration = get_bms_duration();
		play_prepare();
		while(!(i = play_process()));
	}
	finalize();
	if(!*o && i == 1) {
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
	SDL_Surface *r;
	char *d[] = {
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
		"of the License, or (at your o) any later version.",
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

	o[2] = 0;
	if(initialize()) return 1;
	r = newsurface(SDL_SWSURFACE, 800, 2200);
	SDL_FillRect(r, 0, 0x000010);
	SDL_FillRect(r, R(0, 1790, 800, 410), 0);
	for(i=1; i<16; i++)
		SDL_FillRect(r, R(0, 1850-i*5, 800, 5), i);
	for(i=0; i<38; i++) {
		for(j=0; d[i][j]; j++);
		F(r, 400-j*(f[i]%3+1)*4, y[i], f[i]%3+1, d[i], c[f[i]/3-22], 0xffffff);
	}

	SDL_FillRect(s, 0, 0x000010);
	while(++t < 1820 && !check_exit()) {
		SDL_BlitSurface(r, R(0,t,800,600), s, 0);
		SDL_Flip(s);
		SDL_Delay(20);
	}
	if(t == 1820) {
		SDL_FillRect(s, R(0, 0, 800, 100), 0);
		SDL_Flip(s);
		t = SDL_GetTicks() + 8000;
		while((int)SDL_GetTicks() < t && !check_exit());
	}
	
	SDL_FreeSurface(r);
	finalize();
	return 0;
}

int main(int c, char **v) {
	char s[512]={0,};
	int i, j, u;

	u = (c<2 || !*v[1] ? !!filedialog(s) : 0);
	if(c > 2) {
		playspeed = atof(v[2]);
		if(playspeed <= 0) playspeed = 1;
		if(playspeed < .1) playspeed = .1;
		if(playspeed > 99) playspeed = 99;
	}
	if(c > 3) {
		for(j=0; i=v[3][j]; j++) {
			if((i|32) == 'v') *o = 1;
			else if((i|32) == 'i') o[1] = i&32;
			else if((i|32) == 'w') o[2] = !(i&32);
			else if((i|32) == 'm') o[3] = 1;
			else if((i|32) == 's') o[3] = (i=='s' ? 2 : 3);
			else if((i|32) == 'r') o[3] = (i=='r' ? 4 : 5);
			else if(j == 42 && i == 42) o[4] = 1;
		}
	}
	while(--c > 3) {
		if(c < 22 && (i = atoi(v[c]))) keymap[c-4] = i;
	}

	srand(time(0));
	if(c > 1 || u) {
		bmspath = u ? s : v[1];
		return play();
	} else {
		return credit();
	}
}

/* vim: set ts=4 sw=4: */
