/*
 * Angolmois -- the Simple BMS Player
 * Copyright (c) 2005, Kang Seonghoon (TokigunStudio).
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
 */

#include <stdio.h>
#include <stdlib.h>

#define isspace(n) (n==8 || n==10 || n==13 || n==32)

const int nbmsheader = 16;
char *bmsheader[nbmsheader] = {
	"title", "genre", "artist", "stagefile",
	"bpm", "player", "playlevel", "rank", "total",
	"lntype", "lnobj", "wav", "bmp", "bga", "stop", "stp"
};

char *metadata[4]={0,};
float bpm=0f;
int value[6]={0,};
#define v_player value[0]
#define v_playlevel value[1]
#define v_rank value[2]
#define v_total value[3]
#define lntype value[4]
#define lnobj value[5]

int key2index(char a, char b) {
#define GET_DIGIT(n) (47<n && n<58 ? n-48 : ((n|32)-19)/26==3 ? (n|32)-87 : -1)
	if((a = GET_DIGIT(a)) < 0) return -1;
	if((b = GET_DIGIT(b)) < 0) return -1;
	return a * 36 + b;
#undef GET_DIGIT
}

int parsebms(FILE *fp) {
	int i, j, k;
	char line[1024];
	
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
			if(!isspace(line[j])) break;
			while(isspace(line[++j]));
			if(!line[j]) break;
			for(k=j; line[k]; k++);
			while(isspace(line[--k]));
			line[++k] = 0;
			metadata[i] = malloc(k-j+1);
			for(k=j; line[k]; k++) metadata[i][k] = line[k-j];
			break;
		case 4: /* bpm */
			if(isspace(line[j]))
				bpm = atof(line+j);
			else
				; /* #BPMxx sequence - TODO */
			break;
		case 5: /* player */
		case 6: /* playlevel */
		case 7: /* rank */
		case 8: /* total */
		case 9: /* lntype */
		case 10: /* lnobj */
		case 11: /* wav## */
		case 12: /* bmp## */
		case 13: /* bga## */
		case 14: /* stop## */
		case 15: /* stp## */
		}
	}
}

/* vim: set ts=4 sw=4: */
