/*
	Draw 2D bitmaps (background, font, etc)
	from current texture, to the screen

	Copyright (C) 2009-2013	Patrice Mandin

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

#include "../r_soft/dirty_rects.h"

#include "render.h"
#include "render_bitmap.h"

/*--- Functions prototypes ---*/

static void shutdown(render_bitmap_t *this);

static void clipSource(int x, int y, int w, int h);
static void clipDest(int x, int y, int w, int h);
static void setScaler(int srcw, int srch, int dstw, int dsth);
static void setDepth(int enabled, float depth);
static void setMasking(int enabled);
static void drawImage(void);

/*--- Functions ---*/

void render_bitmap_init(render_bitmap_t *render_bitmap)
{
	memset(render_bitmap, 0, sizeof(render_bitmap_t));

	render.bitmap.clipSource = clipSource;
	render.bitmap.clipDest = clipDest;
	render.bitmap.setScaler = setScaler;
	render.bitmap.setDepth = setDepth;
	render.bitmap.setMasking = setMasking;
	render.bitmap.drawImage = drawImage;
	render.bitmap.shutdown = shutdown;
}

static void shutdown(render_bitmap_t *this)
{
}

static void clipSource(int x, int y, int w, int h)
{
	render_texture_t *tex = render.texture;

	if (tex) {
		if (!w) {
			w = render.texture->w;
		}
		if (!h) {
			h = render.texture->h;
		}
	}

	render.bitmap.srcRect.x = x;
	render.bitmap.srcRect.y = y;
	render.bitmap.srcRect.w = w;
	render.bitmap.srcRect.h = h;
}

static void clipDest(int x, int y, int w, int h)
{
	if (!w) {
		w = video.viewport.w;
	}
	if (!h) {
		h = video.viewport.h;
	}

	render.bitmap.dstRect.x = x;
	render.bitmap.dstRect.y = y;
	render.bitmap.dstRect.w = w;
	render.bitmap.dstRect.h = h;
}

static void setScaler(int srcw, int srch, int dstw, int dsth)
{
}

static void setDepth(int enabled, float depth)
{
	render.bitmap.depth_test = enabled;
	render.bitmap.depth = depth;
}

static void setMasking(int enabled)
{
	render.bitmap.masking = enabled;
}

static void drawImage(void)
{
}
