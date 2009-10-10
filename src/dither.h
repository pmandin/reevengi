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

#ifndef DITHER_H
#define DITHER_H 1

/*--- Functions prototypes ---*/

/* Set 216 color palette */
void dither_setpalette(SDL_Surface *src);

/* Find nearest color in 216 color palette */
int dither_nearest_index(int r, int g, int b);

/* Dither image */
void dither(SDL_Surface *src, SDL_Surface *dest);

/* Copy image, no dithering */
void dither_copy(SDL_Surface *src, SDL_Surface *dest);

#endif /* DITHER_H */
