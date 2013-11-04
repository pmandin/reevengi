/*
	Floyd-Steinberg dithering

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

/*--- Defines ---*/

#define FIND_COLOR_INDEX(r,g,b) \
	(approxR1[r]+approxG1[g]+approxB1[b])

#define FIND_APPROX(i, r,g,b) \
	r = approxR2[i]; \
	g = approxG2[i]; \
	b = approxB2[i];

/*--- Constants ---*/

/* Map insensity for 216 color palette */
static const int map_color[6] = {
	0,	51,	102,	153,	204,	255
};

/*--- Variables ---*/

static Uint8 approxR1[256], approxG1[256], approxB1[256];
static Uint8 approxR2[216], approxG2[216], approxB2[216];
static Sint16 fact1[512], fact2[512], fact3[512], fact4[512];
static Uint8 sat[768];

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

int dither_nearest_index(int r, int g, int b)
{
	return 16+FIND_COLOR_INDEX(r,g,b);
}

void dither(SDL_Surface *src, SDL_Surface *dest)
{
	void *errbuffer;
	int x,y,line;
	Uint8 r,g,b, r1,g1,b1, r2,g2,b2, idx;
	Sint16 dr,dg,db;
	Uint8 *dst_line;
	Sint16 *err_line[2];

	if (!src || !dest) {
		return;
	}
	if ((src->format->BytesPerPixel==1) || (dest->format->BytesPerPixel!=1) || (src->h<2)) {
		return;
	}

	errbuffer = calloc(2, (dest->w+2) * sizeof(Uint32)*2);
	if (!errbuffer) {
		fprintf(stderr, "Can not allocate memory for error buffer\n");
		return;
	}

	err_line[0] = &((Sint16 *) errbuffer)[4+(dest->w+2)*0];
	err_line[1] = &((Sint16 *) errbuffer)[4+(dest->w+2)*1];
	line = 0;

	switch(src->format->BytesPerPixel) {
		case 2:
			{
				Uint16 *src_line, *src_col;
				Uint8 *dst_col;
				Sint16 *err_col, *err_next;

				src_line = (Uint16 *) src->pixels;
				dst_line = dest->pixels;
				for (y=0; y<dest->h; y++) {
					src_col = src_line;
					dst_col = dst_line;
					err_col = err_line[line];
					err_next = err_line[line ^ 1];
					err_next[0] = err_next[1] = err_next[2] = 0;
					for (x=0; x<dest->w; x++) {
						SDL_GetRGB(*src_col++, src->format, &r, &g, &b);

						r1 = sat[256 + r + err_col[0]];
						g1 = sat[256 + g + err_col[1]];
						b1 = sat[256 + b + err_col[2]];

						idx = FIND_COLOR_INDEX(r1,g1,b1);

						FIND_APPROX(idx, r2,g2,b2);

						dr = r1-r2;
						dg = g1-g2;
						db = b1-b2;

						*dst_col++ = 16+idx;

						err_col[4+0] += fact1[255+dr];
						err_col[4+1] += fact1[255+dg];
						err_col[4+2] += fact1[255+db];

						err_next[-4+0] += fact2[255+dr];
						err_next[-4+1] += fact2[255+dg];
						err_next[-4+2] += fact2[255+db];

						err_next[0] += fact3[255+dr];
						err_next[1] += fact3[255+dg];
						err_next[2] += fact3[255+db];

						err_next[4+0] = fact4[255+dr];
						err_next[4+1] = fact4[255+dg];
						err_next[4+2] = fact4[255+db];

						err_col += 4;
						err_next += 4;
					}

					/* Next line */
					src_line += src->pitch>>1;
					dst_line += dest->pitch;
					line ^= 1;
				}
			}
			break;
		case 3:
			{
				Uint8 *src_line, *src_col;
				Uint8 *dst_col;
				Sint16 *err_col, *err_next;

				src_line = (Uint8 *) src->pixels;
				dst_line = dest->pixels;
				for (y=0; y<dest->h; y++) {
					src_col = src_line;
					dst_col = dst_line;
					err_col = err_line[line];
					err_next = err_line[line ^ 1];
					err_next[0] = err_next[1] = err_next[2] = 0;
					for (x=0; x<dest->w; x++) {
						Uint32 color;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
						color = (src_col[0]<<16)|(src_col[1]<<8)|src_col[2];
#else
						color = (src_col[2]<<16)|(src_col[1]<<8)|src_col[0];
#endif
						SDL_GetRGB(color, src->format, &r, &g, &b);
						src_col+=3;

						r1 = sat[255 + r + err_col[0]];
						g1 = sat[255 + g + err_col[1]];
						b1 = sat[255 + b + err_col[2]];

						idx = FIND_COLOR_INDEX(r1,g1,b1);

						FIND_APPROX(idx, r2,g2,b2);

						dr = r1-r2;
						dg = g1-g2;
						db = b1-b2;

						*dst_col++ = 16+idx;

						err_col[4+0] += fact1[255+dr];
						err_col[4+1] += fact1[255+dg];
						err_col[4+2] += fact1[255+db];

						err_next[-4+0] += fact2[255+dr];
						err_next[-4+1] += fact2[255+dg];
						err_next[-4+2] += fact2[255+db];

						err_next[0] += fact3[255+dr];
						err_next[1] += fact3[255+dg];
						err_next[2] += fact3[255+db];

						err_next[4+0] = fact4[255+dr];
						err_next[4+1] = fact4[255+dg];
						err_next[4+2] = fact4[255+db];

						err_col += 4;
						err_next += 4;
					}

					/* Next line */
					src_line += src->pitch;
					dst_line += dest->pitch;
					line ^= 1;
				}
			}
			break;
		case 4:
			{
				Uint32 *src_line, *src_col;
				Uint8 *dst_col;
				Sint16 *err_col, *err_next;

				src_line = (Uint32 *) src->pixels;
				dst_line = dest->pixels;
				for (y=0; y<dest->h; y++) {
					src_col = src_line;
					dst_col = dst_line;
					err_col = err_line[line];
					err_next = err_line[line ^ 1];
					err_next[0] = err_next[1] = err_next[2] = 0;
					for (x=0; x<dest->w; x++) {
						SDL_GetRGB(*src_col++, src->format, &r, &g, &b);

						r1 = sat[255 + r + err_col[0]];
						g1 = sat[255 + g + err_col[1]];
						b1 = sat[255 + b + err_col[2]];

						idx = FIND_COLOR_INDEX(r1,g1,b1);

						FIND_APPROX(idx, r2,g2,b2);

						dr = r1-r2;
						dg = g1-g2;
						db = b1-b2;

						*dst_col++ = 16+idx;

						err_col[4+0] += fact1[255+dr];
						err_col[4+1] += fact1[255+dg];
						err_col[4+2] += fact1[255+db];

						err_next[-4+0] += fact2[255+dr];
						err_next[-4+1] += fact2[255+dg];
						err_next[-4+2] += fact2[255+db];

						err_next[0] += fact3[255+dr];
						err_next[1] += fact3[255+dg];
						err_next[2] += fact3[255+db];

						err_next[4+0] = fact4[255+dr];
						err_next[4+1] = fact4[255+dg];
						err_next[4+2] = fact4[255+db];

						err_col += 4;
						err_next += 4;
					}

					/* Next line */
					src_line += src->pitch>>2;
					dst_line += dest->pitch;
					line ^= 1;
				}
			}
			break;
		default:
			break;
	}

	free(errbuffer);
}

void dither_init(void)
{
	int r,g,b,i,j;

	for (i=0; i<256; i++) {
		int j = (i*6)/256;
		approxR1[i] = j*36;
		approxG1[i] = j*6;
		approxB1[i] = j;
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

void dither_copy(SDL_Surface *src, SDL_Surface *dest)
{
	int x,y;
	Uint8 r,g,b;
	Uint8 *dst_line;

	if (!src || !dest) {
		return;
	}
	if ((src->format->BytesPerPixel==1) || (dest->format->BytesPerPixel!=1)) {
		return;
	}

	dst_line = dest->pixels;

	switch(src->format->BytesPerPixel) {
		case 1:
			{
				Uint8 *src_line = (Uint8 *) src->pixels;
				for (y=0; y<dest->h; y++) {
					Uint8 *src_col = src_line;
					Uint8 *dst_col = dst_line;
					for (x=0; x<dest->w; x++) {
						SDL_GetRGB(*src_col++, src->format, &r, &g, &b);

						*dst_col++ = dither_nearest_index(r,g,b);
					}

					/* Next line */
					src_line += src->pitch;
					dst_line += dest->pitch;
				}
			}
			break;
		case 2:
			{
				Uint16 *src_line = (Uint16 *) src->pixels;
				for (y=0; y<dest->h; y++) {
					Uint16 *src_col = src_line;
					Uint8 *dst_col = dst_line;
					for (x=0; x<dest->w; x++) {
						SDL_GetRGB(*src_col++, src->format, &r, &g, &b);

						*dst_col++ = dither_nearest_index(r,g,b);
					}

					/* Next line */
					src_line += src->pitch>>1;
					dst_line += dest->pitch;
				}
			}
			break;
		case 3:
			{
				Uint8 *src_line = (Uint8 *) src->pixels;
				for (y=0; y<dest->h; y++) {
					Uint8 *src_col = src_line;
					Uint8 *dst_col = dst_line;
					for (x=0; x<dest->w; x++) {
						Uint32 color;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
						color = (src_col[0]<<16)|(src_col[1]<<8)|src_col[2];
#else
						color = (src_col[2]<<16)|(src_col[1]<<8)|src_col[0];
#endif
						SDL_GetRGB(color, src->format, &r, &g, &b);
						src_col+=3;

						*dst_col++ = dither_nearest_index(r,g,b);
					}

					/* Next line */
					src_line += src->pitch;
					dst_line += dest->pitch;
				}
			}
			break;
		case 4:
			{
				Uint32 *src_line = (Uint32 *) src->pixels;
				for (y=0; y<dest->h; y++) {
					Uint32 *src_col = src_line;
					Uint8 *dst_col = dst_line;
					for (x=0; x<dest->w; x++) {
						SDL_GetRGB(*src_col++, src->format, &r, &g, &b);

						*dst_col++ = dither_nearest_index(r,g,b);
					}

					/* Next line */
					src_line += src->pitch>>2;
					dst_line += dest->pitch;
				}
			}
			break;
	}
}
