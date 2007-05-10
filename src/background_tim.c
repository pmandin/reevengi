/*
	Load background from tim file

	Copyright (C) 2007	Patrice Mandin

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

#include <stdlib.h>
#include <SDL.h>

#include "state.h"
#include "background_tim.h"

/*--- Defines ---*/

#define MAGIC_TIM	0x10
#define TIM_TYPE_4	8
#define TIM_TYPE_8	9
#define TIM_TYPE_16	2

/*--- Types ---*/

typedef struct {
	Uint32	magic;
	Uint32	type;
	Uint32	offset;
	Uint16	dummy0;
	Uint16	dummy1;
	Uint16	palette_colors;
	Uint16	nb_palettes;
} tim_header_t;

typedef struct {
	Uint16	width;
	Uint16	height;
} tim_size_t;

/*--- Variables ---*/

/*--- Functions prototypes ---*/

/*--- Functions ---*/

SDL_Surface *background_tim_load(SDL_RWops *src)
{
	int start, scale_width, palette_size, i, j, w, h, dblpitch, bpp;
	const char *error = NULL;
	SDL_Surface *surface = NULL;
	SDL_Color palette[256*4];
	int num_palettes;
	tim_header_t tim_header;
	tim_size_t tim_size;
	int rmask, gmask, bmask;
	Uint8 *tim_image = NULL;
	int x,y, image_offset;

	if ( !SDL_RWread( src, &tim_header, sizeof(tim_header), 1 ) ) {
		error = "Can not load header";
		goto done;
	}
	tim_header.magic = SDL_SwapLE32(tim_header.magic);	
	if (tim_header.magic != MAGIC_TIM) {
		error = "Unknown header";
		goto done;
	}
	tim_header.type = SDL_SwapLE32(tim_header.type);	
	tim_header.offset = SDL_SwapLE32(tim_header.offset);	
	tim_header.nb_palettes = SDL_SwapLE16(tim_header.nb_palettes);

	scale_width = 1;
	palette_size = 0;
	bpp = 15;
	bmask = 31<<10;
	gmask = 31<<5;
	rmask = 31;
	dblpitch = 4;
	image_offset = 16;
	switch (tim_header.type) {
		case TIM_TYPE_4:
			scale_width = 4;
			palette_size = 16;
			bpp = 8;
			rmask = gmask = bmask = 0;
			dblpitch = 1;
			image_offset = tim_header.offset + 16;
			break;
		case TIM_TYPE_8:
			scale_width = 2;
			palette_size = 256;
			bpp = 8;
			rmask = gmask = bmask = 0;
			dblpitch = 2;
			image_offset = tim_header.offset + 16;
			break;
		case TIM_TYPE_16:
		default:
			break;
	}

	/* Read palette */
	if (palette_size>0) {
		Uint16 tim_palette[256];
		for (i=0; i<tim_header.nb_palettes; i++) {
			if ( !SDL_RWread( src, &tim_palette, sizeof(Uint16)*palette_size, 1 ) ) {
				error = "Can not load palette";
				goto done;
			}

			/*printf("Load palette %d\n", i);*/
			for (j=0; j<palette_size; j++) {
				int c = SDL_SwapLE16(tim_palette[j]), r,g,b;

				r=(c<<3) & 0xF8;
				g=(c>>2) & 0xF8;
				b=(c>>7) & 0xF8;

				palette[i*256+j]. r = r | (r>>5);
				palette[i*256+j]. g = g | (g>>5);
				palette[i*256+j]. b = b | (b>>5);
			}
		}
	}
	
	/* Read image */
	SDL_RWseek(src, image_offset, SEEK_SET);
	if ( !SDL_RWread( src, &tim_size, sizeof(tim_size), 1 ) ) {
		error = "Can not load image data";
		goto done;
	}
	tim_size.width = SDL_SwapLE16(tim_size.width);
	tim_size.height = SDL_SwapLE16(tim_size.height);
	w = tim_size.width * scale_width;
	h = tim_size.height;
	dblpitch = (w*dblpitch)>>1;

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE,w,h,bpp,rmask,gmask,bmask,0);
	if (!surface) {
		error = "Can not create surface for image";
		goto done;
	}

	tim_image = (Uint8 *) malloc(w * h * 2);
	if (!tim_image) {
		error = "Can not allocate memory for image";
		goto done;
	}

	if ( !SDL_RWread( src, tim_image, dblpitch*h, 1 ) ) {
		error = "Can not read image data";
		goto done;
	}

	/* Set palette */
	if (bpp<=8) {
		SDL_Color *colors = surface->format->palette->colors;
		for (i=0; i<palette_size; i++) {
			colors[i].r = palette[i].r;
			colors[i].g = palette[i].g;
			colors[i].b = palette[i].b;
		}
	}

	switch(tim_header.type) {
		case TIM_TYPE_4:
			{
				Uint8 *src, *dst;
				/*SDL_SetPalette(surface, SDL_PHYSPAL|SDL_LOGPAL, palette, 0, palette_size);*/
				src = tim_image;
				dst = surface->pixels;
				for (y=0;y<h;y++) {
					Uint8 *srcline, *dstline;
					srcline = src;
					dstline = dst;
					for (x=0;x<w>>1;x++) {
						int color = *srcline++;
						*dstline++ = color & 15;
						*dstline++ = (color>>4)&15;
					}
					src += w>>1;
					dst += surface->pitch;
				}
			}
			break;
		case TIM_TYPE_8:
			{
				Uint8 *src, *dst;
				/*SDL_SetPalette(surface, SDL_PHYSPAL|SDL_LOGPAL, &palette[256], 0, palette_size);*/
				src = tim_image;
				dst = surface->pixels;
				for (y=0;y<h;y++) {
					memcpy(dst, src, w);
					src += w;
					dst += surface->pitch;
				}
			}
			break;
		case TIM_TYPE_16:
			{
				Uint16 *src, *dst;
				src = (Uint16 *) tim_image;
				dst = (Uint16 *) surface->pixels;
				for (y=0;y<h;y++) {
					Uint16 *srcline, *dstline;
					srcline = src;
					dstline = dst;
					for (x=0;x<w;x++) {
						Uint16 color = *srcline++;
						*dstline++ = SDL_SwapLE16(color);
					}
					src += w;
					dst += (surface->pitch)>>1;
				}
			}
			break;
	}

done:
	if (tim_image) {
		free(tim_image);
	}

	if ( error ) {
		SDL_RWseek(src, start, SEEK_SET);
		if ( surface ) {
			SDL_FreeSurface(surface);
			surface = NULL;
		}
		fprintf(stderr, "tim: %s\n", error);
	}

	return surface;
}

