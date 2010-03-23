/*
	Draw 2D bitmaps (background, font, etc)
	from current texture, to the screen

	Copyright (C) 2009	Patrice Mandin

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

#ifndef RENDER_BITMAP_H
#define RENDER_BITMAP_H 1

#include <SDL.h>

#include "video.h"

typedef struct render_bitmap_s render_bitmap_t;

struct render_bitmap_s {
	void (*shutdown)(render_bitmap_t *this);

	/* Select a rectangle in source texture */
	void (*clipSource)(int x, int y, int w, int h);

	/* Select a rectangle in destination screen */
	void (*clipDest)(int x, int y, int w, int h);

	/* Set scaling factor from source w*h to dest w*h */
	void (*setScaler)(int srcw, int srch, int dstw, int dsth);

	/* Set depth and enabled or not */
	void (*setDepth)(int enabled, float depth);

	/* Bitmap is a mask, do not draw it */
	void (*setMasking)(int enabled);

	/* Draw scaled image */
	void (*drawImage)(video_t *video);

	SDL_Rect srcRect, dstRect;
	int srcWidth, srcHeight, dstWidth, dstHeight;

	int depth_test, masking;
	float depth;

	/* Scale X,Y table from src to dest (gives dst pixel for a given src) */
	Uint16 sizex_src2dst, sizey_src2dst;
	Uint16 numx_src2dst, numy_src2dst;
	Uint16 *scalex_src2dst;
	Uint16 *scaley_src2dst;

	/* Scale X,Y table from dest to src (gives src pixel for a given dst) */
	Uint16 sizex_dst2src, sizey_dst2src;
	Uint16 numx_dst2src, numy_dst2src;
	Uint16 *scalex_dst2src;
	Uint16 *scaley_dst2src;
};

void render_bitmap_soft_init(render_bitmap_t *render_bitmap);

void render_bitmap_opengl_init(render_bitmap_t *render_bitmap);

#endif /* RENDER_BITMAP_H */
