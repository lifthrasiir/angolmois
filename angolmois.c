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
 * phase 8 (2005-07-14)
 * fixed some bugs (especially bga position), optimized common routines
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

char *v = "TokigunStudio Angolmois version 0.1 final (phase 8)";

typedef struct { double t; int m,i; } b_;
typedef int i_[22];

Uint8 *bH[]={"title", "genre", "artist", "stagefile", "player", "playlevel", "rank", "lntype", "bpm", "lnobj", "wav", "bmp", "bga", "stop", "stp", "random", "if", "else", "endif"},
	*bP, bR[512], **bl, *bm[4], *bs[1296], *bi[1296], *tS[5]={"MISS", "BAD", "GOOD", "GREAT", "COOL"}, Fr[16][96], (*Fz[3])[96]={Fr};
double bb=130, bB[1296], T[2005], ln, ps=1, pS, po=-1, ph=1, Sf;
int bn, bC, (*bc)[8], bt[1296], pT, pt, pp, pa, xf, xn, xs, xl, gr, PB, Pv=1, Sc, Sn[5], So, SO, ST, SM, Sg=256, t, t1, t2, ta, tC[5][2]={{0xff4040, 0xffc0c0}, {0xff40ff, 0xffc0ff}, {0xffff40, 0xffffc0}, {0x40ff40, 0xc0ffc0}, {0x4040ff, 0xc0c0ff}};
i_ bv={1,0,2,1}, _n, N, P, Pr, Pc, Pu, Pb={-1,-1}, o={0,1,1}, tk, tl, tw, tc, tK={122,115,120,100,99,304,308,102,118,109,107,44,108,46,303,307,59,47};
Mix_Chunk *bS[1296];
SDL_Surface *bI[1296], *s, *S, *ss;
SMPEG *bM;
b_ *C[22];
SDL_Event V;
SDL_Rect gR[2];
SDL_AudioSpec A={44100, MIX_DEFAULT_FORMAT, 2};

/******************************************************************************/
/* general functions */

int is(int n){return!(n-9&&n-10&&n-13&&n-32);}
int rs(char*s,int*i){while(is(s[*i]))++*i;if(!s[*i])return 0;for(s+=*i;*s;s++);while(is(*--s));*++s=0;return 1;}
char*sc(char*s){char*t;int i=0;while(s[i++]);t=malloc(i);for(i=0;t[i]=s[i];i++);return t;}
sd(char*s,char*t){int i=0,j=0;for(;*s;s++)if(*s>98)for(j=*s++-97;--j;t++)*t=t[34-*s];else if(*s>34){i|=(*s-35)<<j*6%8;if(j++&3){*t++=i&255;i>>=8;}}}

int sl(int c){return((c|32)-19)/26-3?c:c|32;}
int s_(char*a,char*b){while(*a&&*b&&sl(*a)==sl(*b))a++,b++;return*a==*b;}

/******************************************************************************/
/* system dependent functions */

char*jP(char*);

#ifdef WIN32
#include <windows.h>
char*(*_j)(char*)=jP,__=92,_O[512];
_o(char*s){OPENFILENAME o={76,0,0,_O,0,0,0,s,512,0,0,0,"Choose a file to play",OFN_HIDEREADONLY,0,0,0,0,0,0};sd("$T)>C+7<PW7@VHY;C/X>X,Z;H$E4LT9<CCE-Q+9>VP&f(H$g(OH%#s5##}]iQ#7'AW8I>G8)<C#|NH$h}H$#6R\\Y<Q`)@H$}NO$hzO$33OT)geV$ffM#fdM###",_O);GetOpenFileName(&o);}

_i(){}_f(){}
#else
#include <dirent.h>
char*_F[2592],__=47;int _N;
_o(char*s){}

_i(){DIR*d;struct dirent*e;int i=0,j=0;for(;bP[i];j=bP[i++]-__?j:i);bP[j-1]*=j<0;if(d=opendir(j?bP:".")){while(e=readdir(d))_F[_N++]=sc(e->d_name);closedir(d);}if(j>0)bP[j-1]=__;}
_f(){while(_N--)free(_F[_N]);}
char*_j(char*p){int i=0;while(i<_N&&!s_(_F[i++],p));return i<_N?jP(_F[i]):0;}
#endif

char*jP(char*p){int i=0,j=0;if(*p-47&&*p-92)for(;bP[i];i++)if((bR[i]=bP[i])==__)j=i+1;for(;*p;p++)bR[j++]=*p-47&&*p-92?*p:__;bR[j]=0;return bR;}

/******************************************************************************/
/* bms parser */

int ki(char*s){char a[3]={0,};int b;*a=*s;a[1]=s[1];b=strtol(a,&s,36);return*s?-1:b;}
int BL(const void*a,const void*b){int i=0,j;for(;i<6;i++)if(j=i[*(char**)a]-i[*(char**)b])return j;return 0;}
int BN(const void*a,const void*b){b_*A=(b_*)a,*B=(b_*)b;return A->t>B->t?1:A->t<B->t?-1:A->m-B->m;}
B(int i,double t,int m,int j){b_ x={t,m,j};C[i]=realloc(C[i],sizeof(b_)*(N[i]+1));C[i][N[i]++]=x;}
Br(int i,int j){if(i<18&&C[i][j].i)B(18,C[i][j].t,0,C[i][j].i);C[i][j].m=-1;}

int Bp() {
	FILE* f;
	int i, j, k, a, b, c, r=1, g=0, x, y;
	i_ p={0,}, q={0,};
	double t;
	char d[4096], *s=d+1, *z;
	
	if(f = fopen(bP, "r")) {
		for(_i(); fgets(d, 4096, f); )
			if(*d == 35) {
				for(i=0; i<19; i++) {
					for(j=0; bH[i][j] && (bH[i][j]|32)==(s[j]|32); j++);
					if(!bH[i][j]) break;
				}
				x = is(s[j]);
				if(i==15 && x && (j=abs(atoi(s+j)))) r = rand()%j+1;
				g = i-16 || !x ? i-17 ? i-18 ? g : 0 : !g : r-atoi(s+j);
				
				if(!g) {
					if(i<4 && rs(s, &j)) {
						if(bm[i]) free(bm[i]);
						bm[i] = sc(s+j);
					}
					if(i/4==1 && x) bv[i-4] = atoi(s+j);
					if(i==8) {
						if(x)
							bb = atof(s+j);
						else if((k=ki(s+j))+1 && is(s[j+2]))
							bB[k] = atof(s+j+2);
					}
					if(i==9 && x) {
						while(is(s[++j]));
						if(s[j]) bv[4] = ki(s+j);
					}
					if(i/2==5 && (k=ki(s+j))+1) {
						j += 2;
						if(rs(s, &j)) {
							if(z = k[i%2 ? bi : bs]) free(z);
							k[i%2 ? bi : bs] = sc(s+j);
						}
					}
					if(i==12) {
						k = bC;
						bc = realloc(bc, sizeof(int) * 8 * (k+1));
						bc[k][0] = ki(s+j);
						if(is(s[j+=2]) && rs(s, &j)) {
							bc[k][1] = ki(s+j);
							z = s + j + 2;
							for(j=2; *z && j<8; j++)
								bc[k][j] = strtol(z, &z, 10);
							if(j < 8) bc[k][0] = -1; else bC++;
						} else {
							bc[k][0] = -1;
						}
					}
					if(i==13 && (k=ki(s+j))+1 && is(s[j+2])) bt[k] = atoi(s+j+2);
					if(i==14 && sscanf(s+j, "%d.%d %d", &i, &j, &k)>2) B(21, i+j/1e3, 1, k);
					
					strtol(s, &z, 10);
					if(s+5==z && s[5]==58 && s[6]) {
						bl = realloc(bl, sizeof(char*) * ++bn);
						bl[bn-1] = sc(s);
					}
				}
			}
		fclose(f);

		qsort(bl, bn, sizeof(char*), BL);
		for(i=0; i<bn; i++) {
			x = atoi(bl[i]); y = x % 100; x /= 100;
			if(y == 2) {
				T[x+1] = atof(bl[i]+6);
			} else {
				j = 6;
				rs(bl[i], &j);
				for(k=j; bl[i][k]; k++);
				a = (k - j) / 2;
				for(k=0; k<a; k++,j+=2) {
					b = ki(bl[i]+j);
					c = 8 + y%10 - y/10%2*9;
					t = x + 1. * k / a;
					if(b) {
						if(y==1) B(18, t, 0, b);
						if(y==3 && b/36<16 && b%36<16) B(20, t, 0, b/36*16+b%36);
						if(y==4) B(19, t, 0, b);
						if(y==6) B(19, t, 2, b);
						if(y==7) B(19, t, 1, b);
						if(y==8) B(20, t, 1, b);
						if(y==9) B(21, t, 0, b);
						if(y%10 && y>9 && y<30) {
							if(bv[4] && b==bv[4]) {
								if(N[c] && !C[c][N[c]-1].m) {
									C[c][N[c]-1].m = 3;
									B(c, t, 2, b);
								}
							} else {
								B(c, t, 0, b);
							}
						}
						if(y%10 && y>29 && y<50) B(c, t, 1, b);
					}
					if(y%10 && y>49 && y<70) {
						if(bv[3] == 1 && b) {
							B(c, t, 2 + !p[c], b *= !p[c]);
							p[c] = b;
						} else if(bv[3] == 2) {
							if(p[c] || p[c]-b) {
								if(p[c]) {
									if(q[c]+1 < x) B(c, q[c]+1, 2, 0);
									else if(p[c] - b) B(c, t, 2, 0);
								}
								if(b && (p[c]-b || q[c]+1<x)) B(c, t, 3, b);
								q[c] = x; p[c] = b;
							}
						}
					}
				}
			}
			free(bl[i]);
		}
		free(bl);
		ln = x + 2;
		for(i=0; i<18; i++)
			if(p[i]) B(i/10*9+i%10, bv[3]==2 && q[i]+1<x ? q[i]+1 : ln-1, 2, 0);
		
		for(i=0; i<22; i++)
			if(C[i]) {
				qsort(C[i], N[i], sizeof(b_), BN);
				if(i != 18 && i < 21) {
					b = 0; t = -1;
					for(j=0; j<=N[i]; j++) {
						if(j == N[i] || C[i][j].t > t) {
							if(t >= 0) {
								c = 0;
								for(; k<j; k++) {
									r = 1<<C[i][k].m;
									if(i<18 ? (c & r) || (b ? !(a&4) || r<4 : r != ((a&12)-8 ? a&1 ? 1 : 2 : 8)) : i-19 ? c : c & r)
										Br(i, k);
									else
										c |= r;
								}
								b = b ? (a&12)!=4 : (a&12)==8;
							}
							if(j == N[i]) break;
							a = 0;
							k = j;
							t = C[i][j].t;
						}
						a |= 1 << C[i][j].m;
					}
					if(i<18 && b) {
						while(j >= 0 && C[i][--j].t<0);
						if(j >= 0 && C[i][j].m == 3) Br(i, j);
					}
				}
				for(j=k=0; j<N[i]; j++)
					if(C[i][j].m<0) k++; else C[i][j-k] = C[i][j];
				N[i] -= k;
			}

		for(i=0; i<4; i++)
			if(!bm[i]) *(bm[i] = malloc(1)) = 0;
		for(i=0; i<2005; i++)
			if(T[i] < .01) T[i] = 1;

		return 0;
	} 
	return 1;
}

double jp(double t, double u) {
	int i=(int)(t+1), j=(int)(u+1);
	for(t=(u-j+1)*T[j]-(t-i+1)*T[i]; i<j; t+=T[i++]);
	return t;
}

Bs(int x, int y) {
	b_ *s, *r[18]={0,};
	i_ n={0,}, m, p, q={0,}, u={0,}, g, v, w;
	int o, i, j, k, f=1;
	double t;

	for(i=0; i<18; i++)
		m[i] = (!y||i/9+y-2) && (i%9>6 ? N[7]+N[8]+N[16]+N[17] : i%9-6 ? x==3 || x==5 || (i+2)%9<7 : N[6]+N[15]) ? i : -1, p[i] = i;
	for(i=j=0; i<18; i++)
		if(m[i] < 0) j++; else m[i-j] = m[i];
	o = 18 - j;

	if(x < 4)
		for(i=o,j=0; --i>j; j*=x<2) {
			s = C[m[i]]; C[m[i]] = C[m[j = x<2 ? o-i-1 : rand()%i]]; C[m[j]] = s;
			k = N[m[i]]; N[m[i]] = N[m[j]]; N[m[j]] = k;
		}
	else {
		while(f) {
			for(i=o-1; i>0; i--) {
				k = p[i]; p[i] = p[j=rand()%i]; p[j] = k;
			}
			t = 9e9;
			for(f=i=0; i<o; i++)
				if(g[i] = q[m[i]] < N[m[i]]) {
					f = 1;
					if(t > C[m[i]][q[m[i]]].t)
						t = C[m[i]][q[m[i]]].t - 1e-9;
				}
			t += 2e-9;
			for(i=0; i<o; i++)
				g[i] &= C[m[i]][q[m[i]]].t <= t;
			for(i=0; i<o; i++) {
				if(g[i]) {
					k = C[m[i]][q[m[i]]].m;
					if(k == 2)
						u[j = v[i]] = 2;
					else {
						for(j=p[i]; u[j]; j=p[w[j]]);
						if(k == 3) v[i]=j, w[j]=i, u[j]=1;
					}
					r[j] = realloc(r[j], sizeof(b_) * (n[j]+1));
					r[j][n[j]++] = C[m[i]][q[m[i]]++];
				}
			}
			for(i=0; i<o; i++) u[i] *= u[i] != 2;
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

Uint32*gg(SDL_Surface*s,int x,int y){return(Uint32*)s->pixels+x+y*s->pitch/4;}
int gb(int x,int y,int a,int b){int i=0;for(;i<24;i+=8)y+=((x>>i&255)-(y>>i&255))*a/b<<i;return y;}
SDL_Surface*gs(int f,int w,int h){return SDL_CreateRGBSurface(f,w,h,32,255<<16,65280,255,0);}
SDL_Rect*R(int x,int y,int w,int h){SDL_Rect*r=gR+gr++%2;r->x=x;r->y=y;r->w=w;r->h=h;return r;}

int gI(int x, int y) {
	return (x<y ? ((y*y - 2*x*x + x*x/y*x) << 11)/y/y : x<y*2 ?  ((4*y*y - 8*x*y + 5*x*x - x*x/y*x) << 11)/y/y : 0);
}

/******************************************************************************/
/* font functions */

FP(int x, int y, int z, int c, int s) {
	int i=0, j;
	for(; s && i<z; i++)
		for(j=s-2?s-4?0:i+1:z-i; j<(s-3?s-5?z:z-i-1:i); j++)
			Fz[z-1][(y*z+i)*z+j][c] |= 1<<(7-x);
}

F_(SDL_Surface *s, int x, int y, int z, int c, int u, int v) {
	int i=0, j;
	if(!is(c))
		for(c-=c<0?-96:c<33||c>126?c:32; i<16*z; i++)
			for(j=0; j<8*z; j++)
				if(Fz[z-1][i*z+j%z][c]&1<<(7-j/z))
					*gg(s, x+j, y+i) = gb(u, v, i, 16*z-1);
}

F(SDL_Surface *s, int x, int y, int z, char *c, int u, int v)
	{ for(;*c;x+=8*z)F_(s,x,y,z,(Uint8)*c++,u,v); }

/******************************************************************************/
/* main routines */

int xx() {
	int i = 0;
	while(SDL_PollEvent(&V))
		if(V.type==SDL_QUIT || (V.type==SDL_KEYUP && V.key.keysym.sym==SDLK_ESCAPE)) i = 1;
	return i;
}

int mi() {
	int z, i=0, j, k, l, m;
	char t[1536];

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0) return 1;
	atexit(SDL_Quit);
	s = SDL_SetVideoMode(800, 600, 32, o[2] ? SDL_FULLSCREEN : SDL_SWSURFACE|SDL_DOUBLEBUF);
	if(!s || Mix_OpenAudio(44100, A.format, 2, 2048)<0) return 1;
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption(v, 0);

	sd("##k#;C$)i/%;#&;#&;#$l=jBS#lPa$n<>RPB;[ZY>&jbfma$lqeog#;S#hte+g#jq[S)>OPV=Q<I>^#iRe=3#j~#C#%aOY=K\\V%NPY=aC#%fuC(Y;)#evF0W#grY#e#G#kQ##C$Y?F2Y$e%Y#&gqb('m#b$fy;S&e#;C$#;#e&gdC$&)/#ftC$hha/Y;F$e1eBjtf>fFi2eza$g%iBhkkql2S#krfvF\\&hrg.b/#n2C\\*mBb0S#)S#evjr1[D0I$e#b<C$iBC$);a0#obJ\\#*[#*;b$kr&[$qBRPZ?kR?S&./#g#b#hrF0Y;VPZ>J$e)iBY#e4e7YS$hrC$j#b$iRZ`ZBNP)fFhr&#g#eha#irIT)AS$*AO<)irfpb$guhraD$i#a$hrb$e~a$e&C$)hrA/V;C$Y<F0Y/A#hr_<)g_F<)BqRb$ire~f&F$irgbC$)kba'7:8$e#A$G2hr+SD0esb$ewj0$$h{r(b$i9#$)/;SC$%#jG_S#i#_#iI&/S$1S$1S$)iR_#&i#iB$0Y@aS$*a?Z;$$h~$1O<_C$g#htF$h#YS$%l2NPY=b<F0hra$hOrRF0I2iBfcg'hra/Y;C\\V#&#qBC$e#kBeeN`I2*/#hshdkbjDjb?S#j#jBY`Z=e#jR&#ei&#e#jTC$Y;IT)A[TI<hrC$);a$hriR;C$e%g$hr1OT);[*m2##S2fRb#esgre~g3F`&jBa#erb$)h@###;C$IBgBa$mBC$)i`##&)/;#j{jbhtirSHbJ1#qCikaT&)r#eQf61#f;S$jyb@C&?C&?C`*grh=gGjx$0I0?SD0F('j~F$e#V[$&;#&;l2eE+#mBNPY=b<&hb;CDBeu>OT)krgRF0I2jra0Y;C$f#jba/);a/S#oBfUa$jEb#hcb/S#&#", t);
	for(; i<1536; i++)
		Fr[i%16][i/16^14] = t[i];
	for(z=2; z<4; z++) {
		Fz[z-1] = malloc(1536*z*z);
		for(i=0; i<96; i++) {
			for(j=0; j<16*z*z; j++)
				Fz[z-1][j][i] = 0;
			for(j=0; j<16; j++)
				for(k=0; k<8; k++) {
					l = 0;
					for(m=j-1; m<j+2; m++)
						l = (l<<3) | (m>>4?0:Fr[m][i]<<k>>6&7);
					if((i==3 || i==20) && (k+6)&4)
						l |= ((l&146)>>1)*5;
					if(l & 16) {
						FP(k, j, z, i, 1);
					} else {
						FP(k, j, z, i, ((l&218)==10 || (l&63)==11) * 2);
						FP(k, j, z, i, ((l&434)==34 || (l&63)==38) * 3);
						FP(k, j, z, i, ((l&155)==136 || (l&504)==200) * 4);
						FP(k, j, z, i, ((l&182)==160 || (l&504)==416) * 5);
					}
				}
		}
	}
	return 0;
}

mf() {
	int i;
 
	for(i=0; i<4; i++)
		if(bm[i]) free(bm[i]);
	for(i=0; i<1296; i++) {
		if(bs[i]) free(bs[i]);
		if(bi[i]) free(bi[i]);
		if(bS[i]) Mix_FreeChunk(bS[i]);
		if(bI[i]) SDL_FreeSurface(bI[i]);
	}
	for(i=0; i<22; i++)
		if(C[i]) free(C[i]);
	free(bc);

	if(S) SDL_FreeSurface(S);
	if(ss) SDL_FreeSurface(ss);
	if(bM) { SMPEG_stop(bM); SMPEG_delete(bM); }
	Mix_HookMusic(0, 0);
	Mix_CloseAudio();
	for(i=1; i<3; i++)
		if(Fz[i]) free(Fz[i]);
	_f();
}

int mS(char *p) {
	int i;
	for(i=0; p[i]; i++);
	SDL_BlitSurface(ss, R(0,0,800,20), s, R(0,580,800,20));
	F(s, 797-8*i, 582, 1, p, 0x808080, 0xc0c0c0);
	SDL_Flip(s);
	return xx();
}

int ml() {
	SDL_Surface *t;
	int i, j, *r;

	for(i=0; i<1296; i++) {
		if(bs[i]) {
			if(o[1] && mS(bs[i])) return 1;
			bS[i] = Mix_LoadWAV(_j(bs[i]));
			for(j=0; bs[i][j]; j++);
			j = (j>3 ? *(Uint32*)(bs[i]+j-4) : 0) | 0x20202020;
			if(j != 0x33706d2e && j != 0x2e6d7033) {
				free(bs[i]); bs[i] = 0;
			}
		}
		if(bi[i]) {
			if(o[1] && mS(bi[i])) return 1;
			if(t = IMG_Load(_j(bi[i]))) {
				bI[i] = SDL_DisplayFormat(t);
				SDL_FreeSurface(t);
				SDL_SetColorKey(bI[i], SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
			}
			free(bi[i]); bi[i] = 0;
		}
	}
	
	if(o[1] && mS("loading...")) return 1;
	for(i=0; i<bC; i++)
		if(1[r=bc[i]]>=0 && *r>=0 && bI[r[1]]) {
			t = bI[*r];
			if(!t) {
				SDL_FillRect(bI[*r] = t = gs(SDL_SWSURFACE, 256, 256), 0, 0);
				SDL_SetColorKey(t, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
			}
			r[2] *= r[2] > 0; r[3] *= r[3] > 0;
			SDL_BlitSurface(bI[r[1]], R(r[2], r[3], r[4]-r[2], r[5]-r[3]), t, R(r[6], r[7], 0, 0));
		}
	free(bc);

	return 0;
}

int ms() {
	SDL_Surface *d, *e;
	char c[256];
	int i, j, t;

	sprintf(c, "%s: %s - %s", v, bm[2], bm[0]);
	SDL_WM_SetCaption(c, 0);
	F(s, 248, 284, 2, "loading bms file...", 0x202020, 0x808080);
	SDL_Flip(s);

	ss = gs(SDL_SWSURFACE, 800, 20);
	if(bm[3] && (e = IMG_Load(_j(bm[3])))) {
		int x, y, u, v, w, h, i, j, k, l, r, g, b, a, c, p[4], q[4];

		d = SDL_DisplayFormat(e);
		w = s->w - 1; h = s->h - 1;
		for(i=u=x=0; i<=w; i++) {
			v = y = 0;
			for(k=x; k<x+4; k++)
				p[k-x] = gI(abs(k*w+i-w-i*d->w), w);
			for(j=0; j<=h; j++) {
				r = g = b = 0;
				for(l=y; l<y+4; l++)
					q[l-y] = gI(abs(l*h+j-h-j*d->h), h);
				for(k=x; k<x+4; k++)
					for(l=y; l<y+4; l++)
						if(k>0 && k<=d->w && l>0 && l<=d->h) {
							c = *gg(d, k-1, l-1); a = p[k-x] * q[l-y] >> 6;
							r += (c&255) * a; g += (c>>8&255) * a; b += (c>>16&255) * a;
						}
				*gg(s, i, j) =
					(r<0 ? 0 : r>>24 ? 255 : r>>16) |
					(g<0 ? 0 : g>>24 ? 255 : g>>16)<<8 |
					(b<0 ? 0 : b>>24 ? 255 : b>>16)<<16;
				if((v+=d->h-1) > h) { y++; v -= h; }
			}
			if((u+=d->w-1) > w) { x++; u -= w; }
		}
		SDL_FreeSurface(d);
		SDL_FreeSurface(e);
	}
	if(o[1]) {
		for(i=0; i<800; i++)
			for(j=0; j<600; j++)
				if(j<42 || j>579)
					*gg(s, i, j) = 0xc0c0c + ((*gg(s, i, j) & 0xfcfcfc) >> 2);
		F(s, 6, 4, 2, bm[0], 0x808080, 0xffffff);
		for(i=0; bm[1][i]; i++);
		for(j=0; bm[2][j]; j++);
		F(s, 792-8*i, 4, 1, bm[1], 0x808080, 0xffffff);
		F(s, 792-8*j, 20, 1, bm[2], 0x808080, 0xffffff);
		sprintf(c, "Level %d | BPM %.2f%s | %d note%s [%dKEY%s]", bv[1], bb, "?"+!(xf&8), xn, "s"+(xn==1), xf&1 ? 7 : 5, xf&2 ? "-LN" : "");
		F(s, 3, 582, 1, c, 0x808080, 0xffffff);
		SDL_BlitSurface(s, R(0,580,800,20), ss, R(0,0,800,20));
	}
	SDL_Flip(s);

	t = SDL_GetTicks() + 3000;
	if(ml()) return 1;
	while((int)SDL_GetTicks() < t && !xx());
	return 0;
}

mg(int v) {
	Sn[SM = v]++;
	PB = v ? PB : t+600;
	ST = t+700;
	Sg += "+2789"[v] - 55;
	if(v > 2) SO += ++So > SO;
	So *= v > 1;
}

int mP() {
	int i, j, k, l, m, z;
	double x, y, u;
	char c[99];

	if(pa) {
		u = pS - ps;
		if((int)(u * 1e3))
			ps += u * .015;
		else
			for(pa=i=0,ps=pS; i<22; i++) Pr[i] = P[i];
	}

	x = po;
	t = SDL_GetTicks();
	if(t >= pp)
		x += pp ? (pt = t, pp = 0) : (t - pt) * bb / ph / 24e4;
	z = (int)(x + 1);
	if(x > -1 && ph != T[z]) {
		pt += (int)((z - po - 1) * 24e4 * ph / bb);
		po = z - 1;
		ph = T[z];
	}

	y = 1.25 / ps;
	u = i = z;
	if((z - x) * T[z] > y)
		u = x;
	else
		for(y-=(i-x)*T[i]; T[++i]<=y; y-=T[i]) u = i;
	y = u + y / T[i];
	for(i=0; i<22; i++) {
		for(; Pr[i]<N[i] && C[i][Pr[i]].t<=y; Pr[i]++);
		for(; P[i]<N[i] && C[i][P[i]].t<x; P[i]++) {
			j = C[i][P[i]].i;
			if(i == 18)
				if(bS[j]) {
					if((j = Mix_PlayChannel(-1, bS[j], 0)) >= 0)
						Mix_Volume(j, 96), Mix_GroupChannel(j, 1);
				} else if(bs[j] && (!bM || SMPEG_status(bM) != SMPEG_PLAYING)) {
					if(bM) {
						Mix_HookMusic(0, 0);
						SMPEG_delete(bM);
					}
					if(bM = SMPEG_new(_j(bs[j]), 0, 0)) {
						SMPEG_actualSpec(bM, &A);
						Mix_HookMusic(SMPEG_playAudioSDL, bM);
						SMPEG_enableaudio(bM, 1);
						SMPEG_play(bM);
					}
				}
			if(i == 19)
				Pb[C[i][P[i]].m] = j, Pv = 1;
			if(i == 20 && (u = (C[i][P[i]].m ? bB[j] : j)))
				pt = t, po = x, bb = u;
			if(i == 21)
				pp = (t<pp ? pp : t) + (C[i][P[i]].m ? j : (int)(bt[j] * 1250 / bb)), po = x;
			if(i<18 && *o && C[i][P[i]].m-1 && bS[j] && (j=Mix_PlayChannel(-1,bS[j],0))>=0)
				Mix_GroupChannel(j, 1);
		}
		for(; i<18 && !*o && Pc[i]<P[i]; Pc[i]++) {
			j = C[i][Pc[i]].m;
			if(j>=0 && j-1 && (j-2 || Pu[i])) {
				u = C[i][Pc[i]].t;
				if((x - u) * T[(int)(u+1)] / bb * Sf > 6e-4) mg(0); else break;
			}
		}
	}

	while(SDL_PollEvent(&V)) {
		k = V.key.keysym.sym;
		if(V.type == SDL_QUIT) return 2;
		if(V.type == SDL_KEYUP) {
			if(k == SDLK_ESCAPE) return 2;
			for(i=0; !*o && i<18; i++)
				if(tl[i] >= 0 && k == tK[i]) {
					tk[i] = 0;
					if(N[i] && Pu[i]) {
						for(j=P[i]+1; C[i][j].m-2; j--);
						Pu[i] = 0;
						u = (C[i][j].t - x) * T[z] / bb * Sf * 1e4;
						if(-6 < u && u < 6) C[i][j].m ^= -1; else mg(0);
					}
				}
		}
		if(V.type == SDL_KEYDOWN) {
			if(k == SDLK_F3)
				pa=1,pS-=pS>10?5:pS>5?1:pS>1?.5:pS>.201?.2:0;
			if(k == SDLK_F4)
				pa=1,pS+=pS<1?.2:pS<5?.5:pS<10?1:pS<95?5:0;
			for(i=0; !*o && i<18; i++)
				if(tl[i] >= 0 && k == tK[i]) {
					tk[i] = 1;
					if(N[i]) {
						j = P[i] - !(P[i]<1 || (P[i]<N[i] && C[i][P[i]-1].t+C[i][P[i]].t<2*x));
						if(bS[C[i][j].i] && (l = Mix_PlayChannel(-1, bS[C[i][j].i], 0)) >= 0) Mix_GroupChannel(l, 0);
						if(j < P[i]) {
							while(j>=0 && C[i][j].m==1) j--;
							if(j < 0) continue;
						} else {
							while(j<N[i] && C[i][j].m==1) j++;
							if(j == N[i]) continue;
						}
						if(P[i]<N[i] && C[i][P[i]].m==2) mg(0);
						if(C[i][j].m - 2) {
							u = (C[i][j].t - x) * T[z] / bb * Sf * 1e5;
							if(C[i][j].m >= 0 && (u = u<0 ? -u : u) < 60) {
								l && Mix_GroupChannel(l, 1);
								Pu[i] = C[i][j].m == 3;
								C[i][j].m ^= -1;
								mg(u<6 ? 4 : u<20 ? 3 : u<35 ? 2 : 1);
								Sc += (int)((60 - u) * (5 + 5. * So / xn));
							}
						}
					}
				}
		}
	}
	if(x > ln ? (!bM || SMPEG_status(bM) != SMPEG_PLAYING) && Mix_GroupNewer(1)<0 : x < -1) return 1;

	SDL_FillRect(s, R(0,30,t1,490), 0x404040);
	if(t2) SDL_FillRect(s, R(t2,30,800-t2,490), 0x404040);
	for(i=0; i<18; i++)
		if(tl[i] >= 0) {
			SDL_FillRect(s, R(tl[i],30,tw[i],490), 0);
			if(tk[i]) SDL_BlitSurface(S, R(tl[i],140,tw[i],380), s, R(tl[i],140,0,0));
		}
	SDL_SetClipRect(s, R(0,30,800,490));
	for(i=0; i<18; i++)
		if(tl[i] >= 0) {
			for(j=P[i]; j<Pr[i]; j++) {
				k = (int)(525 - 400 * ps * jp(x, C[i][j].t));
				m = C[i][j].m; m ^= -(m<0);
				if(m == 0) l = k + 5;
				if(m == 1) continue;
				if(m == 2) l = 520, k += 5;
				if(m == 3) {
					l = k + 5;
					k = (int)(530 - 400 * ps * jp(x, C[i][++j].t));
					k = k<30 ? 30 : k;
				}
				if(k>0 && l>k)
					SDL_BlitSurface(S, R(800+tl[i%9],0,tw[i%9],l-k), s, R(tl[i],k,0,0));
			}
			if(P[i]==Pr[i] && Pr[i]<N[i] && C[i][Pr[i]].m==2)
				SDL_BlitSurface(S, R(800+tl[i%9],0,tw[i%9],490), s, R(tl[i],30,0,0));
		}
	for(i=z-1; i<y; i++) {
		j = (int)(530 - 400 * ps * jp(x, i));
		SDL_FillRect(s, R(0,j,t1,1), 0xc0c0c0);
		if(t2) SDL_FillRect(s, R(t2,j,800-t2,1), 0xc0c0c0);
	}
	if(o[4]) {
		for(i=P[20]; i<Pr[20]; i++)
			SDL_FillRect(s, R(0,(int)(530-400*ps*jp(x,C[20][i].t)),t1,1), 0xffff00);
		for(i=P[21]; i<Pr[21]; i++)
			SDL_FillRect(s, R(0,(int)(530-400*ps*jp(x,C[21][i].t)),t1,1), 0x00ff00);
	}
	if(t < ST) {
		for(i=0; tS[SM][i]; i++);
		F(s, t1/2-8*i, 292, 2, tS[SM], tC[SM][0], tC[SM][1]);
		if(So > 1) F(s, t1/2-4*sprintf(c, "%d COMBO", So), 320, 1, c, 0x808080, 0xffffff);
		Pv = SM ? Pv : 1;
	}
	SDL_SetClipRect(s, 0);
	if(Pv > 0 || (Pv < 0 && t >= PB)) {
		SDL_FillRect(s, R(ta,172,256,256), 0);
		for(i=0; i<4; i++)
			if(i/2 == (t<PB) && Pb[i] >= 0 && bI[Pb[i]])
				SDL_BlitSurface(bI[Pb[i]], R(0,0,256,256), s, R(ta,172,0,0));
		Pv = -(t < PB);
	}

	i = (t - pT) / 1000; j = xl / 1000;
	sprintf(c, "SCORE %07d%c%4.1fx%c%02d:%02d / %02d:%02d%c@%9.4f%cBPM %6.2f",
			Sc, 0, pS, 0, i/60, i%60, j/60, j%60, 0, x, 0, bb);
	SDL_BlitSurface(S, R(0,0,800,30), s, R(0,0,0,0));
	SDL_BlitSurface(S, R(0,520,800,80), s, R(0,520,0,0));
	F(s, 10, 8, 1, c, 0, 0);
	F(s, 5, 522, 2, c+14, 0, 0);
	F(s, t1-94, 565, 1, c+20, 0, 0x404040);
	F(s, 95, 538, 1, c+34, 0, 0);
	F(s, 95, 522, 1, c+45, 0, 0);
	F_(s, 6+(t-pT<xl ? (t-pT)*t1/xl : t1), 548, 1, -1, 0x404040, 0x404040);
	if(!t2 && !*o) {
		i = (Sg = Sg>512 ? 512 : Sg)<0 ? 0 : (Sg*400>>9) - (int)(160*ph*(1+x)) % 40;
		SDL_FillRect(s, R(4,588,i>360?360:i<5?5:i,8), 0xc00000);
	}

	SDL_Flip(s);
	return 0;
}

/******************************************************************************/
/* entry point */

int mc() {
	SDL_Surface *r;
	char *d[] = {
		"TokigunStudio Angolmois", "\"the Simple BMS Player\"", v + 24,
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

	o[2] = 0;
	if(mi()) return 1;
	r = gs(SDL_SWSURFACE, 800, 2200);
	SDL_FillRect(r, 0, 0x000010);
	SDL_FillRect(r, R(0, 1790, 800, 410), 0);
	for(i=1; i<16; i++)
		SDL_FillRect(r, R(0, 1850-i*5, 800, 5), i);
	for(i=0; i<38; i++) {
		for(j=0; d[i][j]; j++);
		F(r, 400-j*(f[i]%3+1)*4, y[i], f[i]%3+1, d[i], c[f[i]/3-22], 0xffffff);
	}

	SDL_FillRect(s, 0, 0x000010);
	while(++t < 1820 && !xx()) {
		SDL_BlitSurface(r, R(0,t,800,600), s, 0);
		SDL_Flip(s);
		SDL_Delay(20);
	}
	if(t == 1820) {
		SDL_FillRect(s, R(0, 0, 800, 100), 0);
		SDL_Flip(s);
		t = SDL_GetTicks() + 8000;
		while((int)SDL_GetTicks() < t && !xx());
	}
	
	SDL_FreeSurface(r);
	mf();
	return 0;
}

int main(int c, char **v) {
	char b[512]={0,};
	int i, j, t, r, d;
	double p=-1, e, q;

	if(c < 2 || !*v[1]) _o(b);
	ps = c>2 && (e=atof(v[2]))>0 ? e<.1 ? .1 : e>99 ? 99 : e : 1;
	if(c > 3)
		for(j=0; i=v[3][j]; j++) {
			t = i|32; r = i&32;
			*o |= t == 'v';
			if(t == 'i') o[1] = r;
			if(t == 'w') o[2] = !r;
			o[3] = t-'m' ? t-'s' ? t-'r' ? o[3] : 4+!r : 2+!r : 1;
			if(i/3 == 16) bv[3] = i - 48;
			o[4] |= /*j == 42 &&*/ i == 42;
		}
	for(t=r=0; --c>3; )
		if(i = (c<22) * atoi(v[c])) tK[c-4] = i;
	if(o[4]) return mc();

	srand(time(0));
	if(c > 1 || *b) {
		bP = *b ? b : v[1];
		if(mi() || Bp()) { mf(); return 1; }
		for(i=0; *bv==4 && i<9; i++) {
			if(C[i+9]) free(C[i+9]);
			N[i+9] = N[i];
			C[i+9] = malloc(sizeof(b_) * N[i]);
			for(j=0; j<N[i]; j++) C[i+9][j] = C[i][j];
		}
		if(o[3]) {
			Bs(o[3], *bv!=3);
			if(*bv%2==0) Bs(o[3], 2);
		}

		xf = !!(N[7] + N[8] + N[16] + N[17]) + (N[6] + N[15] ? 4 : 0) + (N[20] ? 8 : 0);
		for(i=xn=0; i<18; i++)
			for(j=0; j<N[i]; j++)
				xf |= (C[i][j].m > 1) * 2, xn += C[i][j].m < 3;
		for(i=0; i<xn; i++)
			xs += (int)(300 + 300. * i / xn);

		if(i = !ms()) {
			for(e=bb; ; P[i]++) {
				for(i=-1,j=0; j<22; j++)
					if(P[j] < N[j] && (i < 0 || C[j][P[j]].t < C[i][P[i]].t)) i = j;
				if(i < 0) { t += (int)(jp(p, ln) * 24e7 / e); break; }
				t += (int)(jp(p, C[i][P[i]].t) * 24e7 / e);
				j = C[i][P[i]].i; d = 0;
				if(i<19 && bS[j]) d = (int)(bS[j]->alen / .1764);
				if(i == 20) {
					q = (C[i][P[i]].m ? bB[j] : j);
					if(q > 0) e = q;
					if(q < 0) { t += (int)(jp(-1, p) * 24e7 / e); break; }
				}
				if(i == 21) {
					if(C[i][P[i]].m) t += j;
					else t += (int)(bt[j] * 125e4 * T[(int)(p+1)] / e);
				}
				if(r < t + d) r = t + d;
				p = C[i][P[i]].t;
			}
			xl = (t > r ? t : r) / 1000;

			for(i=0; i<18; i++)
				tl[i] = -1,
				tw[i] = (i+2)%9>6 ? 40 : 25,
				tc[i] = 0x808080 | 255 << 16 >> (i%9-5 ? i%9-6 ? 24-i%9%2*8 : 8 : 0);
			for(i=0; i<5; i++) tl[i] = 41 + 26 * i;
			tl[5] = t2 = 0; t1 = 171;
			if(xf & 1) tl[7] = 171, tl[8] = 197, t1 += 52;
			if(xf & 4) { tl[6] = t1; t1 += 41; }
			if(bv[0] > 1) {
				for(i=0; i<9; i++)
					tl[i+9] = tl[i] < 0 ? -1 : i-5 ? i-6 ? tl[i] + (xf&1 ? 537 : 589) : 760 - tl[6] : 760;
				t2 = 800 - t1;
				if(bv[0] == 3) {
					for(i=9; i<18; i++) tl[i] += t1 - t2;
					t1 += 801 - t2; t2 = 0;
				}
			}
			ta = ((t2 ? t2 : 800) + t1 - 256) / 2;
			
			S = gs(SDL_SWSURFACE, 1200, 600);
			for(i=0; i<18; i++) {
				if(tl[i] >= 0) {
					for(j=140; j<520; j++)
						SDL_FillRect(S, R(tl[i],j,tw[i],1), gb(tc[i], 0, j-140, 1000));
					if(i < 9)
						for(j=0; j*2<tw[i]; j++)
							SDL_FillRect(S, R(800+tl[i]+j,0,tw[i]-2*j,600), gb(tc[i], 0xffffff, tw[i]-j, tw[i]));
				}
			}
			for(j=-244; j<556; j++) {
				for(i=-10; i<20; i++)
					*gg(S, j+244, i+10) = gb(0xc0c0c0, 0x606060, 850 - abs((i*2+j*3+750) % 2000 - 1000), 700);
				for(i=-20; i<60; i++)
					*gg(S, j+244, i+540) = gb(0xc0c0c0, 0x404040, 850 - abs((i*3+j*2+750) % 2000 - 1000), 700);
			}
			SDL_FillRect(S, R(t1+20,0,(t2?t2:820)-t1-40,600), 0);
			for(i=0; i<20; i++)
				for(j=20; j*j+i*i>400; j--) {
					*gg(S, t1+j, i+10) = *gg(S, t1+j, 539-i) = 0;
					if(t2) *gg(S, t2-j-1, i+10) = *gg(S, t2-j-1, 539-i) = 0;
				}
			if(!t2 && !*o)
				SDL_FillRect(S, R(0,584,368,16), 0x404040),
				SDL_FillRect(S, R(4,588,360,8), 0);
			SDL_FillRect(S, R(10,564,t1,1), 0x404040);

			SDL_FillRect(s, 0, 0);
			SDL_BlitSurface(S, R(0,0,800,30), s, R(0,0,0,0));
			SDL_BlitSurface(S, R(0,520,800,80), s, R(0,520,0,0));

			pT = pt = SDL_GetTicks();
			pS = ps;
			Mix_AllocateChannels(128);
			for(i=0; i<22; P[i++]=0);
			Sf = 1.5 - bv[2] / 4.;

			while(!(i = mP()));
		}
		mf();
		if(!*o && i == 1) {
			if(Sg > 150) {
				printf("*** CLEARED! ***\n");
				for(i=4; i>=0; i--)
					printf("%-5s %4d    %s", tS[i], Sn[i], "\n"+(i!=2));
				printf("MAX COMBO %d\nSCORE %07d (max %07d)\n", SO, Sc, xs);
			} else {
				printf("YOU FAILED!\n");
			}
		}
	}
	return 0;
}

/* vim: set ts=4 sw=4: */

