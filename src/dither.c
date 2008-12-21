/*
	Dithering

	Copyright (C) 2008	Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <SDL.h>

/*--- Constants ---*/

/* Map insensity for 216 color palette */
static const int map_color[6] = {
	0,	51,	102,	153,	204,	255
};

/*--- Variables ---*/

static int inited = 0;
static Uint8 approxR1[256], approxG1[256], approxB1[256];
static Uint8 approxR2[216], approxG2[216], approxB2[216];
static Uint16 fact1[512], fact2[512], fact3[512], fact4[512];
static Uint8 sat[768];

/*--- Functions prototypes ---*/

static void dither_init(void);

/*--- Functions ---*/

void dither_setpalette(SDL_Surface *src)
{
	int r,g,b;
	SDL_Color palette[216];

	for (r=0; r<6; r++) {
		for (g=0; g<6; g++) {
			for (b=0; b<6; b++) {
				int i = r*36+g*6+b;

				palette[i].r = map_color[r];
				palette[i].g = map_color[g];
				palette[i].b = map_color[b];
			}
		}
	}		

	SDL_SetPalette(src, SDL_LOGPAL|SDL_PHYSPAL, palette, 16, 216);
}

void dither(SDL_Surface *src, SDL_Surface *dest)
{
	if (!src || !dest) {
		return;
	}
	if (dest->format->BytesPerPixel!=1) {
		return;
	}

	if (!inited) {
		dither_init();
		inited = 1;
	}
}

static void dither_init(void)
{
	int r,g,b,i,j;

	for (i=0; i<256; i++) {
		int j = (i*6)/256;
		approxR1[i] = j;
		approxG1[i] = j*6;
		approxB1[i] = j*36;
	}

	for (r=0; r<6; r++) {
		for (g=0; g<6; g++) {
			for (b=0; b<6; b++) {
				int i = r*36+g*6+b;

				approxR2[i] = map_color[r];
				approxG2[i] = map_color[g];
				approxB2[i] = map_color[b];
			}
		}
	}		

	for (i=-255, j=0; i<256; i++,j++) {
		fact1[j] = (i*7)/16;
		fact2[j] = (i*3)/16;
		fact3[j] = (i*5)/16;
		fact4[j] = (i*1)/16;
	}

	for (i=0; i<256; i++) {
		sat[i] = 0;
		sat[i+256] = i;
		sat[i+512] = 255;
	}
}
