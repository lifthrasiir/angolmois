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

/*
 * this code is development snapshot of angolmois.
 * i will not answer any letters before alpha version is released. please wait :)
 * <url: http://pandora.sapzil.info/dev/angolmois/>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

#ifdef WIN32
const char sep = 92;
#else
const char sep = 47;
#endif

const int nbmsheader = 20;
const char *bmsheader[nbmsheader] = {
	"title", "genre", "artist", "stagefile",
	"bpm", "player", "playlevel", "rank", "total",
	"lntype", "lnobj", "wav", "bmp", "bga", "stop", "stp",
	"random", "if", "else", "endif"
};
const char bmspath[512];
char respath[512];
char **bmsline=0;
int nbmsline=0;

char *metadata[4]={0,};
float bpm=0;
int value[6]={1,0,3,100,0,0};
#define v_player value[0]
#define v_playlevel value[1]
#define v_rank value[2]
#define v_total value[3]
#define lntype value[4]
#define lnobj value[5]

Mix_Chunk *sndres[1296]={0,};
SDL_Surface *imgres[1296]={0,};
int stoptab[1296]={0,};
float bpmtab[1296]={0,};

typedef struct { float time; int type, index; } bmsnote;
bmsnote **channel[23]={0,};
float shorten[1000]={0,};
int nchannel[23]={0,};
#define P1(n) (n)
#define P2(n) ((n)+9)
#define BGM (18)
#define BGA (19)
#define BPM (20)
#define STOP (21)
#define STOP2 (22)

int isspace(char n) { return n==8 || n==10 || n==13 || n==32; }
int getdigit(char n) { return 47<a && a<58 ? a-48 : ((a|32)-19)/26==3 ? (a|32)-87 : -1296; }
int key2index(char a, char b) { return getdigit(a) * 36 + getdigit(b); }

char *adjust_path(char *path) {
	int i, j=0;
	if(*path != 47 && *path != 92) {
		for(i=0; bmspath[i]; ) {
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
	if(!isspace(str[*i])) return 0;
	while(isspace(str[++*i]));
	if(!str[*i]) return 0;
	for(j=*i; str[j]; j++);
	while(isspace(str[--j]));
	str[++j] = 0;
	return 1;
}

int compare_bmsline(const void *a, const void *b) {
	int i, j;
	for(i=0; i<6; i++)
		if(j = ((char*)a)[i] - ((char*)b)[i]) return j;
	return 0;
}

void add_note(int chan, float time, int type, int index) {
	bmsnote *temp;
	temp = malloc(sizeof(bmsnote));
	temp->time = time; temp->type = type; temp->index = index;
	channel[chan] = realloc(channel[chan], sizeof(bmsnote*) * (nchannel+1));
	channel[chan][nchannel++] = temp;
}

int parse_bms(FILE *fp) {
	int i, j, k, a, b, c;
	int rnd=1, ignore=0;
	int measure, chan;
	float time;
	char line[1024];
	SDL_Surface *tempsurf;
	
	srand(time(0));
	while(fgets(str, 1024, fp)) {
		if(*str++ != '#') continue;

		for(i=0; i<nbmsheader; i++) {
			for(j=0; bmsheader[i][j]; j++)
				if((bmsheader[i][j]|32) != (str[j]|32)) break;
			if(!bmsheader[i][j]) break;
		}
		switch(i) {
		case 16: /* random */
			if(isspace(line[j]))
				if(i = abs(atoi(line+j)))
					rnd = rand() % i + 1;
			break;
		case 17: /* if */
			if(isspace(line[j]))
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
				metadata[i] = malloc(k-j+1);
				for(k=j; line[k]; k++) metadata[i][k] = line[k-j];
				break;
			case 4: /* bpm */
				if(isspace(line[j])) {
					bpm = atof(line+j);
				} else {
					i = key2index(line[j], line[j+1]);
					if(i < 0 || !isspace(line[j+2])) break;
					bpmtab[i] = atof(line+j+2);
				}
				break;
			case 5: /* player */
			case 6: /* playlevel */
			case 7: /* rank */
			case 8: /* total */
			case 9: /* lntype */
				if(isspace(line[j]))
					value[i-5] = atoi(line+j);
				break;
			case 10: /* lnobj */
				if(!isspace(line[j])) break;
				while(isspace(line[++j]));
				if(line[j])
					value[5] = key2index(line[j], line[j+1]);
				break;
			case 11: /* wav## */
				i = key2index(line[j], line[j+1]);
				if(i < 0) break;
				j += 2;
				if(!remove_whitespace(line, &j)) break;
				if(sndres[i]) Mix_FreeChunk(sndres[i]);
				sndres[i] = Mix_LoadWAV(adjust_path(line+j));
				break;
			case 12: /* bmp## */
				if(!remove_whitespace(line, &j)) break;
				if(tempsurf = IMG_Load(adjust_path(line+j))) {
					if(imgres[i]) SDL_FreeSurface(imgres[i]);
					imgres[i] = SDL_DisplayFormat(tempsurf);
					SDL_FreeSurface(tempsurf);
				}
				break;
			case 13: /* bga## */
				/* TODO */
				break;
			case 14: /* stop## */
				i = key2index(line[j], line[j+1]);
				if(i < 0 || !isspace(line[j+2])) break;
				stoptab[i] = atoi(line+j+2);
				break;
			case 15: /* stp## */
				/* TODO */
				break;
			}
			
			for(i=0; i<5; i++)
				if((line[i]-8)/10 != 4) break;
			if(i>4 && line[5]==58 && line[6]) {
				while(line[i++]);
				bmsline = realloc(bmsline, sizeof(char*) * (nbmsline+2));
				bmsline += nbmsline;
				*bmsline = malloc(sizeof(char) * i);
				for(i=0; line[i]; i++)
					(*bmsline)[i] = line[i];
				(*bmsline)[i] = 0;
				bmsline -= nbmsline++;
			}
		}
	}

	qsort(bmsline, nbmsline, sizeof(char*), compare_bmsline);
	for(i=0; i<nbmsline; i++) {
		j = 0;
		for(k=0; k<5; k++)
			j = j * 10 + bmsline[i][j] - 48;
		measure = j / 100; chan = j % 100;
		if(chan == 2) {
			shorten[measure] = atof(bmsline[i]+7);
		} else {
			j = 6;
			remove_whitespace(bmsline[i], &j);
			for(k=j; bmsline[i][k]; k++);
			a = (k - j) / 2;
			for(k=0; k<a; k++,j+=2) {
				b = key2index(bmsline[i][j], bmsline[i][j+1]);
				time = measure + 1. * k / a;
				if(chan == 1) {
					add_note(BGM, time, 0, b);
				} else if(chan == 4) {
					add_note(BGA, time, 0, b);
				} else if(chan == 6) {
					add_note(BGA, time, 1, b);
				} else if(chan == 7) {
					add_note(BGA, time, 2, b);
				} else {
					/* TODO */
				}
			}
		}
	}
}

/* vim: set ts=4 sw=4: */
