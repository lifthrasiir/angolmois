/*
 * Angolmois -- the Simple BMS Player
 * Copyright (c) 2005, Kang Seonghoon (TokigunStudio).
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
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

#define isspace(n) (n==8 || n==10 || n==13 || n==32)

#ifdef WIN32
const char sep = 92;
#else
const char sep = 47;
#endif

const int nbmsheader = 16;
const char *bmsheader[nbmsheader] = {
	"title", "genre", "artist", "stagefile",
	"bpm", "player", "playlevel", "rank", "total",
	"lntype", "lnobj", "wav", "bmp", "bga", "stop", "stp"
};
const char bmspath[512];
char respath[512];

char *metadata[4]={0,};
float bpm=0f, bpmtab[1296]={0,};
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

int key2index(char a, char b) {
#define GET_DIGIT(n) (47<n && n<58 ? n-48 : ((n|32)-19)/26==3 ? (n|32)-87 : -1296)
	return GET_DIGIT(a) * 36 + GET_DIGIT(b);
#undef GET_DIGIT
}

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

int remove_whitespace(char *str, int *i, int *j) {
	if(!isspace(str[*i])) return 0;
	while(isspace(str[++*i]));
	if(!str[*i]) return 0;
	for(*j=*i; str[*j]; *j++);
	while(isspace(str[--*j]));
	str[++*j] = 0;
	return 1;
}

int parse_bms(FILE *fp) {
	int i, j, k;
	char line[1024];
	SDL_Surface *tempsurf;
	
	while(fgets(str, 1024, fp)) {
		if(*str++ != '#') continue;
		for(i=0; i<nbmsheader; i++) {
			for(j=0; bmsheader[i][j]; j++)
				if((bmsheader[i][j]|32) != (str[j]|32)) break;
			if(!bmsheader[i][j]) break;
		}
		switch(i) {
		case 0: /* title */
		case 1: /* genre */
		case 2: /* artist */
		case 3: /* stagefile */
			if(!remove_whitespace(line, &j, &k)) break;
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
			if(!remove_whitespace(line, &j, &k)) break;
			if(sndres[i]) Mix_FreeChunk(sndres[i]);
			sndres[i] = Mix_LoadWAV(adjust_path(line+j));
			break;
		case 12: /* bmp## */
			if(!remove_whitespace(line, &j, &k)) break;
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
		/* TODO: parsing sequence data (#xxxyy:...) */
	}
}

/* vim: set ts=4 sw=4: */
