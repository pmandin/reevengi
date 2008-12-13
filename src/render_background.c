/*
	Rescale and display background

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

#include "log.h"
#include "video.h"
#include "render_background.h"

/*--- Variables ---*/

static int zoomw = 0, zoomh = 0;
static int *zoomx = NULL, *zoomy = NULL;
static SDL_Surface *zoom_surf = NULL;
static video_surface_t *backgroundSurf = NULL;

/*--- Functions prototypes ---*/

static void render_scaled_background(SDL_Surface *source, SDL_Surface *dest);

/*--- Functions ---*/

void render_background_init(video_t *this, video_surface_t *source)
{
	int i, recreate_surface= (zoom_surf==NULL);
	SDL_Surface *src_surf;
	
	if (!this || !source) {
		return;
	}
	backgroundSurf = source;

	/* Mark background places as dirty */
	this->dirty_rects[0]->setDirty(this->dirty_rects[0], 0,0, this->width, this->height);
	this->dirty_rects[1]->setDirty(this->dirty_rects[1], 0,0, this->width, this->height);

	src_surf = source->getSurface(source);

	if (this->viewport.w != zoomw) {
		zoomw = this->viewport.w;
		zoomx = realloc(zoomx, sizeof(int) * zoomw);
		for (i=0; i<zoomw; i++) {
			zoomx[i] = (i * src_surf->w) / zoomw;
		}
		recreate_surface = 1;
	}

	if (this->viewport.h != zoomh) {
		zoomh = this->viewport.h;
		zoomy = realloc(zoomy, sizeof(int) * zoomh);
		for (i=0; i<zoomh; i++) {
			zoomy[i] = (i * src_surf->h) / zoomh;
		}
		recreate_surface = 1;
	}

	if (recreate_surface) {
		if (zoom_surf) {
			SDL_FreeSurface(zoom_surf);
		}

		/* Create surface with same bpp as source one */
		zoom_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, zoomw, zoomh,
			src_surf->format->BitsPerPixel,
			src_surf->format->Rmask, src_surf->format->Gmask,
			src_surf->format->Bmask, src_surf->format->Amask);

		if (!zoom_surf) {
			return;
		}

		/* Set palette */
		if (zoom_surf->format->BitsPerPixel == 8) {
			SDL_Color palette[256];
			for (i=0;i<256;i++) {
				palette[i].r = src_surf->format->palette->colors[i].r;
				palette[i].g = src_surf->format->palette->colors[i].g;
				palette[i].b = src_surf->format->palette->colors[i].b;
			}
			SDL_SetPalette(zoom_surf, SDL_LOGPAL, palette, 0, 256);
		}
	}

	render_scaled_background(src_surf, zoom_surf);
}

void render_background_shutdown(void)
{
	if (zoomx) {
		free(zoomx);
		zoomx = NULL;
	}
	if (zoomy) {
		free(zoomy);
		zoomy = NULL;
	}
	if (zoom_surf) {
		SDL_FreeSurface(zoom_surf);
		zoom_surf = NULL;
	}
}

void render_background(video_t *this)
{
	int x,y;

	if (!zoom_surf) {
		return;
	}

	/* Now copy back each dirty rectangle from zoom_surf */
	for (y=0; y<this->dirty_rects[this->numfb]->height; y++) {
		int dst_y1, dst_y2;

		/* Target destination of zoomed image */
		dst_y1 = y<<4;
		dst_y2 = (y+1)<<4;

		/* Clip to viewport */
		if (dst_y1<this->viewport.y) {
			dst_y1 = this->viewport.y;
		}
		if (dst_y2>this->viewport.y+this->viewport.h) {
			dst_y2 = this->viewport.y+this->viewport.h;
		}

		if (dst_y1>=dst_y2) {
			continue;
		}

		for (x=0; x<this->dirty_rects[this->numfb]->width; x++) {
			int dst_x1, dst_x2;
			SDL_Rect src_rect, dst_rect;

			if (this->dirty_rects[this->numfb]->markers[y*this->dirty_rects[this->numfb]->width + x] == 0) {
				continue;
			}

			/* 16x16 block */
			dst_x1 = x<<4;
			dst_x2 = (x+1)<<4;

			/* Clip to viewport */
			if (dst_x1<this->viewport.x) {
				dst_x1 = this->viewport.x;
			}
			if (dst_x2>this->viewport.x+this->viewport.w) {
				dst_x2 = this->viewport.x+this->viewport.w;
			}

			if (dst_x1>=dst_x2) {
				continue;
			}

			src_rect.x = dst_x1 - this->viewport.x;
			src_rect.y = dst_y1 - this->viewport.y;
			dst_rect.x = dst_x1;
			dst_rect.y = dst_y1;
			dst_rect.w = src_rect.w = dst_x2-dst_x1;
			dst_rect.h = src_rect.h = dst_y2-dst_y1;
			SDL_BlitSurface(zoom_surf, &src_rect, this->screen, &dst_rect);

			this->upload_rects[this->numfb]->setDirty(this->upload_rects[this->numfb],
				dst_rect.x,dst_rect.y,
				dst_rect.w,dst_rect.h);
		}
	}

	this->dirty_rects[this->numfb]->clear(this->dirty_rects[this->numfb]);
}

/* Redraw scaled image in a target surface */
static void render_scaled_background(SDL_Surface *source, SDL_Surface *dest)
{
	int x,y;
	Uint8 *dst = (Uint8 *) dest->pixels;

	switch(dest->format->BytesPerPixel) {
		case 1:
			{
				Uint8 *dst_line = (Uint8 *) dst;
				for(y=0; y<dest->h; y++) {
					Uint8 *dst_col = dst_line;
					Uint8 *src_col = (Uint8 *) source->pixels;
					src_col += zoomy[y] * source->pitch;
					for(x=0; x<dest->w; x++) {
						*dst_col++ = src_col[zoomx[x]];
					}
					dst_line += dest->pitch;
				}
			}
			break;
		case 2:
			{
				Uint16 *dst_line = (Uint16 *) dst;
				for(y=0; y<dest->h; y++) {
					Uint16 *dst_col = dst_line;
					Uint16 *src_col = (Uint16 *) source->pixels;
					src_col += zoomy[y] * (source->pitch>>1);
					for(x=0; x<dest->w; x++) {
						*dst_col++ = src_col[zoomx[x]];
					}
					dst_line += dest->pitch>>1;
				}
			}
			break;
		case 3:
			{
				Uint8 *dst_line = (Uint8 *) dst;
				for(y=0; y<dest->h; y++) {
					Uint8 *dst_col = dst_line;
					Uint8 *src_col = (Uint8 *) source->pixels;
					src_col += zoomy[y] * source->pitch;
					for(x=0; x<dest->w; x++) {
						int src_pos = zoomx[x]*3;
						*dst_col++ = src_col[src_pos];
						*dst_col++ = src_col[src_pos+1];
						*dst_col++ = src_col[src_pos+2];
					}
					dst_line += dest->pitch;
				}
			}
			break;
		case 4:
			{
				Uint32 *dst_line = (Uint32 *) dst;
				for(y=0; y<dest->h; y++) {
					Uint32 *dst_col = dst_line;
					Uint32 *src_col = (Uint32 *) source->pixels;
					src_col += zoomy[y] * (source->pitch>>2);
					for(x=0; x<dest->w; x++) {
						*dst_col++ = src_col[zoomx[x]];
					}
					dst_line += dest->pitch>>2;
				}
			}
			break;
	}
}
