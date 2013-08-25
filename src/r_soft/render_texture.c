/*
	Textures for 3D objects

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

#include "../video.h"
#include "../parameters.h"
#include "../log.h"
#include "../background_tim.h"

#include "../r_common/render.h"
#include "../r_common/render_texture.h"
#include "../r_common/r_misc.h"
#include "../r_common/render_texture_list.h"

/*--- Functions prototypes ---*/

static void mark_trans(render_texture_t *this, int num_pal, int x1,int y1, int x2,int y2);

/*--- Functions ---*/

render_texture_t *render_texture_soft_create(int flags)
{
	render_texture_t *tex;

	tex = render_texture_create(flags);
	if (!tex) {
		return NULL;
	}

	tex->mark_trans = mark_trans;

	tex->must_pot = flags & RENDER_TEXTURE_MUST_POT;
	tex->cacheable = flags & RENDER_TEXTURE_CACHEABLE;

	tex->bpp = video.screen->format->BytesPerPixel;
	/* FIXME: copy palette from format elsewhere */
	memcpy(&(tex->format), video.screen->format, sizeof(SDL_PixelFormat));
	tex->format.palette = NULL;

	list_render_texture_add(tex);

	return tex;
}

static void mark_trans(render_texture_t *this, int num_pal, int x1,int y1, int x2,int y2)
{
	Uint8 *src_line;
	Uint8 *alpha_pal;
	int x,y;

	return;

	if (!this) {
		return;
	}
	if (this->bpp != 1) {
		return;
	}
	if (params.use_opengl) {
		return;
	}
	if (num_pal>=this->num_palettes) {
		return;
	}
	x1 = MAX(0, MIN(this->w-1, x1));
	y1 = MAX(0, MIN(this->h-1, y1));
	x2 = MAX(0, MIN(this->w-1, x2));
	y2 = MAX(0, MIN(this->h-1, y2));

	src_line = this->pixels;
	src_line += y1 * this->pitch;
	src_line += x1;
	alpha_pal = this->alpha_palettes[num_pal];
	for (y=y1; y<y2; y++) {
		Uint8 *src_col = src_line;
		for (x=x1; x<x2; x++) {
			Uint8 c = *src_col;

			if (!alpha_pal[c]) {
				*src_col = 0;
			}
			src_col++;
		}
		src_line += this->pitch;
	}
}
