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
char bmspath[512]="D:\\Works\\angolmois\\project\\endlessdream\\MB_end7an.bme";
char respath[512];
char **bmsline=0;
int nbmsline=0;

char *metadata[4]={0,};
double bpm=130;
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

int parse_bms() {
	FILE* fp;
	int i, j, k, a, b, c;
	int prev[20]={0,};
	int rnd=1, ignore=0;
	int measure, chan;
	double ctime;
	char *line=malloc(1024);
	SDL_Surface *tempsurf;
	
	fp = fopen(bmspath, "r");
	if(!fp) return 1;

	srand(time(0));
	while(fgets(line, 1024, fp)) {
		if(line[0] != 35) continue;
		line++;
		printf("%s",line);

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
				bmsline = realloc(bmsline, sizeof(char*) * (nbmsline+1));
					/* sometimes returns 0 */
				bmsline += nbmsline;
				*bmsline = malloc(sizeof(char) * i);
				for(i=0; (*bmsline)[i] = line[i]; i++);
				bmsline -= nbmsline++;
			}
		}
		line--;
	}
	free(line);
	fclose(fp);

	qsort(bmsline, nbmsline, sizeof(char*), compare_bmsline);
	return 0;
	
	/* not tested */
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
	/* /not tested */

	for(i=0; i<nbmsline; i++) free(bmsline[i]);
	free(bmsline);
	
	/* TODO: sort and arrange notes */
	return 0;
}

int finalize() {
	int i, j;
	
	for(i=0; i<6; i++) {
		free(metadata[i]);
	}
	for(i=0; i<1296; i++) {
		if(sndres[i]) Mix_FreeChunk(sndres[i]);
		if(imgres[i]) SDL_FreeSurface(imgres[i]);
	}
	for(i=0; i<23; i++) {
		for(j=0; j<nchannel[i]; j++)
			free(channel[i][j]);
		free(channel[i]);
	}
	
	return 0;
}

/******************************************************************************/

SDL_Surface *screen;
SDL_Event event;

/* compressed font data (whitespaces are ignored) */
Uint8 fontmap[]="\0\2\3\6\7\10\13\14\16\20\30\33\34\36$,03678;<>?@AFU]_`acfghkl"
	"nopsvwx{|~\177\201\237\303\333\361";
Uint8 fontinfo[]=">',\37==8==M\\\256\211\255K==========MNM{M==================>"
	"==========K=\26\315&]=]=]=_-=?==]]]``]]=]]]]_]-\20-7";
Uint8 fontdata[]=".::::..$...66662'67;O7;O64));IIH;*III;))CDE'+.4F?'8JJJ9IKFK9+\
    ++.-+.444444.+4.++++++.4.YYT.TYY....T......4T...%'+.4C=;EEGLRNEE;0:3++++++<\
    ;EEG,08MCU;EE&1&&EE;,16FFFFU''UCCCT&&EE;;EECTEEEE;U&&'+.....;EEE;EEEE;;EEEE\
    <&EE;...$$$......$$$...4'+.4C4.+'T$$TC4.+'+.4C;EEE'+.$..;>A@@@B>E;)06EEEUEE\
    ETEEETEEEET15ECCCCE51SFEEEEEEFSUCCCTCCCCUUCCCTCCCCC15ECCGEE51EEEEUEEEEET...\
    .....T&&&&&&EEE;EFJQMMQJFECCCCCCCCCUEPUUIIEEEEEEENRLGEEE06EEEEEE60TEEEETCCC\
    C;EEEEEIL;(&TEEEETEEEE;EEC;&&EE;T.........EEEEEEEEE;EEEEEEE60)EEEEIIIU66>EP\
    ;00;PE>VXF:......U&&(,08MCU:44444444:=C4.+'%:++++++++:)06E>U4.+';&&<EEE<CCC\
    TEEEEET;EECCEE;&&&<EEEEE<;EEEUCE;,//.T.....<EEEE<&EE;CCCTEEEEEEE..$.......&\
	&$$&&&&EEE;CCEFJQQJFE0+++++++++OUIIIIEETEEEEEEE;EEEEEE;TEEEEEETCCC<EEEEEE<&\
	&&TEECCCCC;EC;&&E;..T...///,EEEEEEE;EEEEE60)EEEIIIU6>E6006E>EEEE51+.4CU(,08\
	MCU,....M....,................M....,....MMZW,";
int fontindex[96]={0};

void putpixel(int x, int y, int c) { ((Uint32*)screen->pixels)[x+y*800]=c; }
void drawhline(int x1, int x2, int y, int c) { while(x1<x2) putpixel(x1++, y, c); }
void drawvline(int x, int y1, int y2, int c) { while(y1<y2) putpixel(x, y1++, c); }
int blend(int x, int y, int a, int b) { int i=0;for(;i<24;i+=8)y+=((x>>i&255)-(y>>i&255))*a/b<<i;return y; }
void putblendedpixel(int x, int y, int c, int o) { putpixel(x, y, blend(((Uint32*)screen->pixels)[x+y*800], c, o, 255)); }

void fontprocess() {
	int i,j=0;
	for(i=0; i<95; i++) {
		fontinfo[i]--;
		fontindex[i+1] = fontindex[i] + fontinfo[i]%16 - fontinfo[i]/16 + 1;
	}
	for(i=0; i<fontindex[95]; i++) {
		if(fontdata[i+j] == 32) i--,j++;
		else fontdata[i] = fontmap[fontdata[i+j]-36];
	}
}

void printchar(int x, int y, char c) {
	int i, j, k;
	if(c < 33 || c > 126) return;
	for(i=fontindex[c-33],j=fontinfo[c-33]>>4; j<=(fontinfo[c-33]&15); i++,j++) {
		for(k=0; k<8; k++) {
			if(fontdata[i]&(1<<(7-k)))
				putpixel(x+k, y+j, blend(0xFF80FF, 0xFFFFFF, j, 15));
		}
	}
}

void printstr(int x, int y, char *s) {
	while(*s) {
		printchar(x, y, *s++);
		x += 8;
	}
}

int test() {
	int i, j;
	/* test section */
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)<0) return 2;
	/*
	parse_bms();
	for(i=0; i<23; i++) {
		for(j=0; j<nchannel[i]; j++) {
			printf("[%d.%d] %8.4lf (%d; %d)\n", i, j, channel[i][j]->time, channel[i][j]->type, channel[i][j]->index);
		}
	}
	finalize();

	{
		Mix_Chunk *music;
		music = Mix_LoadWAV("E:\\Program\\Games\\rdm\\1st\\girl\\gtc3.wav");
		for(i=0; i<8; i++) {
			printf("%d", i);
			Mix_PlayChannel(-1, music, 0);
			SDL_Delay(200);
		}
		while(j=Mix_Playing(-1));
		Mix_FreeChunk(music);
	}
	*/
	Mix_CloseAudio();
	getchar();

	return 0;
}

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
	int i;

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0) return 1;
	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE);
	if(!screen) return 1;

	fontprocess();
	SDL_WM_SetCaption("TokigunStudio Angolmois: development version", "angolmois-dev (2005/03/31)");
	printstr(20, 20, "TokigunStudio Angolmois (alpha version!)");
	printstr(20, 40, "3.1415926535897932384662643383279............");
	for(i=0; i<95; i++)
		printchar(20+i*8, 60, i+33);
	SDL_UpdateRect(screen, 0, 0, 800, 600);
	
	i = 1;
	while(i) {
		/*
		if(!SDL_MUSTLOCK(screen) || SDL_LockSurface(screen)>=0) {
			drawhline(0, 800, 300, (SDL_GetTicks() & 0xFF) * 0x10101);
			SDL_UnlockSurface(screen);
		}
		SDL_UpdateRect(screen, 0, 0, 800, 600);
		*/
		while(SDL_PollEvent(&event)) {
			if(event.type==SDL_QUIT || (event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE)) i = 0;
		}
	}
	
	return 0;
}

/* vim: set ts=4 sw=4: */
