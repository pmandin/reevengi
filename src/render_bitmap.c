/*
	Draw 2D bitmaps (background, font, etc)
	Software backend

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

#include "video.h"
#include "render.h"

/*--- Defines ---*/

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/*--- Functions prototypes ---*/

static void bitmapUnscaled(video_t *video, int x, int y);
static void bitmapScaled(video_t *video, int x, int y, int w, int h);

static void refresh_scaled_version(video_t *video, render_texture_t *texture, int new_w, int new_h);

static void create_scaled_version(video_t *video, render_texture_t *src, render_texture_t *dst);
static void create_scaled_version8(render_texture_t *src, render_texture_t *dst);
static void create_scaled_version16(render_texture_t *src, render_texture_t *dst);
static void create_scaled_version24(render_texture_t *src, render_texture_t *dst);
static void create_scaled_version32(render_texture_t *src, render_texture_t *dst);

/*--- Functions ---*/

void render_bitmap_soft_init(render_t *render)
{
	render->bitmapUnscaled = bitmapUnscaled;
	render->bitmapScaled = bitmapScaled;
}

static void bitmapUnscaled(video_t *video, int x, int y)
{
	render_texture_t *tex = render.texture;

	if (!tex)
		return;

	bitmapScaled(video,x,y,tex->w,tex->h);
}

static void bitmapScaled(video_t *video, int x, int y, int w, int h)
{
	render_texture_t *tex = render.texture;
	int x1,y1, w1,h1;

	if (!tex)
		return;

	refresh_scaled_version(video, tex, w,h);

	/* Clip position to viewport */
	x1 = MAX(x,video->viewport.x);
	y1 = MAX(y,video->viewport.y);
	w1 = MIN(x+w,video->viewport.x+video->viewport.w) - x1;
	h1 = MIN(y+h,video->viewport.y+video->viewport.h) - y1;

	/* Use scaled version if available, to update screen */
	if (tex->scaled) {
		tex = tex->scaled;
	}
}

static void refresh_scaled_version(video_t *video, render_texture_t *texture, int new_w, int new_h)
{
	int create_scaled = 0, fill_scaled = 0;

	/* Generate a cached version of texture scaled/dithered or both */
	if (texture->scaled) {
		/* Recreate if different target size */		
		if ((texture->scaled->w != new_w) || (texture->scaled->h != new_h)) {
			/*texture->scaled->resize(new_w,new_h);*/
			fill_scaled = 1;
		}
	} else {
		if ((texture->w != new_w) || (texture->h != new_h)) {
			create_scaled = 1;
		}
		if (render.dithering) {
			create_scaled = 1;
		}
	}

	/* Create new render_texture, for scaled size */
	if (create_scaled) {
		/* texture->scaled = render_texture_create_copy(texture) */
		/* texture->scaled->resize(new_w,new_h) */
		fill_scaled = 1;	
	}

	/* Then redraw a scaled version */
	if (fill_scaled) {
		create_scaled_version(video, texture, texture->scaled);
		if (render.dithering) {
			/* Dither scaled version */
		}
	}
}

static void create_scaled_version(video_t *video, render_texture_t *src, render_texture_t *dst)
{
	switch(video->bpp) {
		case 8:
			create_scaled_version8(src, dst);
			break;
		case 15:
		case 16:
			create_scaled_version16(src, dst);
			break;
		case 24:
			create_scaled_version24(src, dst);
			break;
		case 32:
			create_scaled_version32(src, dst);
			break;
	}
}

static void create_scaled_version8(render_texture_t *src, render_texture_t *dst)
{
	int x,y;
	Uint8 *dst_line = dst->pixels;

	for(y=0; y<dst->h; y++) {
		Uint8 *dst_col = dst_line;
		Uint8 *src_col = (Uint8 *) src->pixels;
		int y1 = (y * src->h) / dst->h;

		src_col += y1 * src->pitch;
		for(x=0; x<dst->w; x++) {
			int x1 = (x * src->w) / dst->w;

			*dst_col++ = src_col[x1];
		}
		dst_line += dst->pitch;
	}
}

static void create_scaled_version16(render_texture_t *src, render_texture_t *dst)
{
	int x,y;
	Uint16 *dst_line = (Uint16 *) dst->pixels;

	for(y=0; y<dst->h; y++) {
		Uint16 *dst_col = dst_line;
		Uint16 *src_col = (Uint16 *) src->pixels;
		int y1 = (y * src->h) / dst->h;

		src_col += y1 * (src->pitch>>1);
		for(x=0; x<dst->w; x++) {
			int x1 = (x * src->w) / dst->w;

			*dst_col++ = src_col[x1];
		}
		dst_line += dst->pitch>>1;
	}
}

static void create_scaled_version24(render_texture_t *src, render_texture_t *dst)
{
	int x,y;
	Uint8 *dst_line = (Uint8 *) dst->pixels;

	for(y=0; y<dst->h; y++) {
		Uint8 *dst_col = dst_line;
		Uint8 *src_col = (Uint8 *) src->pixels;
		int y1 = (y * src->h) / dst->h;

		src_col += y1 * src->pitch;
		for(x=0; x<dst->w; x++) {
			int x1 = (x * src->w) / dst->w;
			int src_pos = x1*3;

			*dst_col++ = src_col[src_pos];
			*dst_col++ = src_col[src_pos+1];
			*dst_col++ = src_col[src_pos+2];
		}
		dst_line += dst->pitch;
	}
}

static void create_scaled_version32(render_texture_t *src, render_texture_t *dst)
{
	int x,y;
	Uint32 *dst_line = (Uint32 *) dst->pixels;

	for(y=0; y<dst->h; y++) {
		Uint32 *dst_col = dst_line;
		Uint32 *src_col = (Uint32 *) src->pixels;
		int y1 = (y * src->h) / dst->h;

		src_col += y1 * (src->pitch>>2);
		for(x=0; x<dst->w; x++) {
			int x1 = (x * src->w) / dst->w;

			*dst_col++ = src_col[x1];
		}
		dst_line += dst->pitch>>2;
	}
}
