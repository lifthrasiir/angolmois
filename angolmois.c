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

#define NBMSHEADER 20
const char *bmsheader[NBMSHEADER] = {
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
double bpm=0;
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
double bpmtab[1296]={0,};

typedef struct { double time; int type, index; } bmsnote;
bmsnote **channel[23]={0,};
double shorten[1000]={0,};
int nchannel[23]={0,};
double length;

#define GET_CHANNEL(player, chan) ((player)*9+(chan)-1)
#define ADD_NOTE(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 0, (index))
#define ADD_INVNOTE(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 1, (index))
#define ADD_LNSTART(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 2, (index))
#define ADD_LNDONE(player, chan, time, index) \
	add_note(GET_CHANNEL(player,chan), (time), 3, (index))
#define ADD_BGM(time, index) add_note(18, (time), 0, (index))
#define ADD_BGA(time, index) add_note(19, (time), 0, (index))
#define ADD_BGA2(time, index) add_note(19, (time), 1, (index))
#define ADD_POORBGA(time, index) add_note(19, (time), 2, (index))
#define ADD_BPM(time, index) add_note(20, (time), 0, (index))
#define ADD_BPM2(time, index) add_note(20, (time), 1, (index))
#define ADD_STOP(time, index) add_note(21, (time), 0, (index))
#define ADD_STP(time, index) add_note(22, (time), 0, (index))

int isspace(char n) { return n==8 || n==10 || n==13 || n==32; }
int getdigit(char n) { return 47<n && n<58 ? n-48 : ((n|32)-19)/26==3 ? (n|32)-87 : -1296; }
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

void add_note(int chan, double time, int type, int index) {
	bmsnote *temp;
	temp = malloc(sizeof(bmsnote));
	temp->time = time; temp->type = type; temp->index = index;
	channel[chan] = realloc(channel[chan], sizeof(bmsnote*) * (nchannel[chan]+1));
	channel[chan][nchannel[chan]++] = temp;
}

int parse_bms(FILE *fp) {
	int i, j, k, a, b, c;
	int prev[20]={0,};
	int rnd=1, ignore=0;
	int measure, chan;
	double ctime;
	char *line=malloc(1024);
	SDL_Surface *tempsurf;
	
	srand(time(0));
	while(fgets(line, 1024, fp)) {
		if(*line++ != '#') continue;

		for(i=0; i<NBMSHEADER; i++) {
			for(j=0; bmsheader[i][j]; j++)
				if((bmsheader[i][j]|32) != (line[j]|32)) break;
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
				for(k=j; line[k]; k++);
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
				ctime = measure + 1. * k / a;
				if(b) {
					if(chan == 1) {
						ADD_BGM(ctime, b);
					} else if(chan == 3) {
						ADD_BPM(ctime, b);
					} else if(chan == 4) {
						ADD_BGA(ctime, b);
					} else if(chan == 6) {
						ADD_POORBGA(ctime, b);
					} else if(chan == 7) {
						ADD_BGA2(ctime, b);
					} else if(chan == 8) {
						ADD_BPM2(ctime, b);
					} else if(chan == 9) {
						ADD_STOP(ctime, b);
					} else if(chan % 10 != 0 && chan > 9 && chan < 30) {
						if(lnobj && b == lnobj) {
							chan = GET_CHANNEL(chan>20, chan%10);
							for(c=nchannel[chan]-1; c>=0 && channel[chan][c]->type!=0; c--);
							if(c >= 0) {
								channel[chan][c]->type = 2;
								ADD_LNDONE(chan>20, chan%10, ctime, b);
							}
						} else {
							ADD_NOTE(chan>20, chan%10, ctime, b);
						}
					} else if(chan % 10 != 0 && chan > 29 && chan < 50) {
						ADD_INVNOTE(chan>40, chan%10, ctime, b);
					}
				}
				if(chan % 10 != 0 && chan > 49 && chan < 70) {
					if(lntype == 1 && b) {
						if(prev[chan-50]) {
							prev[chan-50] = 0;
							ADD_LNDONE(chan>60, chan%10, ctime, 0);
						} else {
							prev[chan-50] = b;
							ADD_LNSTART(chan>60, chan%10, ctime, b);
						}
					} else if(lntype == 2) {
						if(prev[chan-50] != b) {
							prev[chan-50] = b;
							ADD_LNDONE(chan>60, chan%10, ctime, 0);
							if(b) {
								ADD_LNSTART(chan>60, chan%10, ctime, b);
							}
						} else if(!prev[chan-50]) {
							prev[chan-50] = b;
							ADD_LNSTART(chan>60, chan%10, ctime, b);
						}
					}
				}
			}
		}
	}
	
	length = measure + 1;
	for(i=0; i<20; i++) {
		if(prev[i]) {
			ADD_LNDONE(i>10, i%10, length, 0);
		}
	}
	for(i=0; i<nbmsline; i++) free(bmsline[i]);
	free(bmsline);

	/* TODO: sort and arrange notes */
	free(line);
	return 0;
}

/******************************************************************************/

SDL_Surface *screen;
SDL_Event event;

/*
angolmois
	show logo screen
angolmois help
	print help message (text)
angolmois <filename> <options...>
	play <filename> with the following options:
	x<speed> -- playing speed (between 0.1 and 99.0; default 1.0)
	record -- when playing is done, update record file.
	viewer -- work as viewer
	ranking -- show ranking of <filename> (text)
note: ranking file is <filename>.arank
*/
int main(int argc, char **argv) {
	int i, j;

	if(SDL_Init(SDL_INIT_VIDEO)<0) return 1;
	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE);
	if(!screen) return 1;

	while(1) {
		while(SDL_PollEvent(&event)) {
			if(event.type==SDL_QUIT || (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE)) return 0;
		}
		if(SDL_MUSTLOCK(screen) && SDL_LockSurface(screen)<0) continue;
		for(i=0; i<600; i++) for(j=0; j<800; j++)
			((unsigned int*)screen->pixels)[i*800+j] = ((rand()&0xFF)<<16) | (rand()&0xFFFF);
		if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
		SDL_UpdateRect(screen, 0, 0, 800, 600);
	}
	
	return 0;
}

/* vim: set ts=4 sw=4: */
