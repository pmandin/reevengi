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

#include <SDL.h>

#include "video.h"
#include "render.h"

/*--- Defines ---*/

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/*--- Functions prototypes ---*/

static void bitmapUnscaled(video_t *video, int x, int y);
static void bitmapScaled(video_t *video, int x, int y, int w, int h);

static void bitmapScaledRtNodirty(video_t *video, SDL_Rect *src_rect, SDL_Rect *dst_rect);
static void bitmapScaledScDirty(video_t *this, SDL_Rect *src_rect, SDL_Rect *dst_rect);
static void bitmapScaledRtDirty(video_t *this, SDL_Rect *src_rect, SDL_Rect *dst_rect);

static void refresh_scaled_version(video_t *video, render_texture_t *texture, int new_w, int new_h);

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
	int src_x=0, src_y=0, dst_x=x, dst_y=y;
	SDL_Rect src_rect, dst_rect;

	/* Clipping for out of bounds */
	if ((x>=video->viewport.w) || (y>=video->viewport.h) || (x+w<0) || (y+h<0)) {
		return;
	}

	if (x<0) {
		dst_x = 0;
		src_x -= x;
		w -= x;
	}
	if (y<0) {
		dst_y = 0;
		src_y -= y;
		h -= y;
	}
	if (x+w>=video->viewport.w) {
		w = video->viewport.w - x;
	}
	if (y+h>=video->viewport.h) {
		h = video->viewport.h - y;
	}
	if ((w<=0) || (h<=0) || (src_x>=w) || (src_y>=h)) {
		return;
	}

	dst_x += video->viewport.x;
	dst_y += video->viewport.y;

	if (!render.texture)
		return;

	/* Use scaled version if available, to update screen */
	refresh_scaled_version(video, render.texture, w,h);

	src_rect.x = src_x;
	src_rect.y = src_y;
	dst_rect.x = dst_x;
	dst_rect.y = dst_y;
	src_rect.w = dst_rect.w = w;
	src_rect.h = dst_rect.h = h;

	if (render.texture->scaled) {
		if (render.useDirtyRects) {
			bitmapScaledScDirty(video, &src_rect, &dst_rect);
		} else {
			SDL_BlitSurface(render.texture->scaled, &src_rect, video->screen, &dst_rect);
		}
	} else {
		if (SDL_MUSTLOCK(video->screen)) {
			SDL_LockSurface(video->screen);
		}

		if (render.useDirtyRects) {
			bitmapScaledRtDirty(video, &src_rect, &dst_rect);
		} else {
			bitmapScaledRtNodirty(video, &src_rect, &dst_rect);
		}

		if (SDL_MUSTLOCK(video->screen)) {
			SDL_UnlockSurface(video->screen);
		}
	}
}

static void bitmapScaledRtNodirty(video_t *video, SDL_Rect *src_rect, SDL_Rect *dst_rect)
{
	render_texture_t *tex = render.texture;
	SDL_Surface *surf = video->screen;
	int j;

	switch(video->bpp) {
		case 8:
			{
				Uint8 *src = tex->pixels;
				src += src_rect->y * tex->pitch;
				src += src_rect->x;
				Uint8 *dst = surf->pixels;
				dst += dst_rect->y * surf->pitch;
				dst += dst_rect->x;

				for (j=0; j<src_rect->h; j++) {
					memcpy(dst, src, src_rect->w);
					src += tex->pitch;
					dst += surf->pitch;
				}
			}
			break;
		case 15:
		case 16:
			{
				Uint16 *src = (Uint16 *) tex->pixels;
				src += src_rect->y * (tex->pitch>>1);
				src += src_rect->x;
				Uint16 *dst = (Uint16 *) surf->pixels;
				dst += dst_rect->y * (surf->pitch>>1);
				dst += dst_rect->x;

				for (j=0; j<src_rect->h; j++) {
					memcpy(dst, src, src_rect->w<<1);
					src += tex->pitch>>1;
					dst += surf->pitch>>1;
				}
			}
			break;
		case 24:
			{
				Uint8 *src = tex->pixels;
				src += src_rect->y * tex->pitch;
				src += src_rect->x * 3;
				Uint8 *dst = surf->pixels;
				dst += dst_rect->y * surf->pitch;
				dst += dst_rect->x * 3;

				for (j=0; j<src_rect->h; j++) {
					memcpy(dst, src, src_rect->w*3);
					src += tex->pitch;
					dst += surf->pitch;
				}
			}
			break;
		case 32:
			{
				Uint32 *src = (Uint32 *) tex->pixels;
				src += src_rect->y * (tex->pitch>>2);
				src += src_rect->x;
				Uint32 *dst = (Uint32 *) surf->pixels;
				dst += dst_rect->y * (surf->pitch>>2);
				dst += dst_rect->x;

				for (j=0; j<src_rect->h; j++) {
					memcpy(dst, src, src_rect->w<<2);
					src += tex->pitch>>2;
					dst += surf->pitch>>2;
				}
			}
			break;
	}
}

static void bitmapScaledScDirty(video_t *this, SDL_Rect *src_rect, SDL_Rect *dst_rect)
{
	SDL_Rect blt_src_rect, blt_dst_rect;
	int x,y;

	for (y=0; y<this->dirty_rects[this->numfb]->height; y++) {
		int num_rows = 16;

		if (((y+1)<<4) > dst_rect->y+dst_rect->h) {
			num_rows = dst_rect->y+dst_rect->h - (y<<4);
		}

		blt_src_rect.y = src_rect->y + (y<<4);
		blt_dst_rect.y = dst_rect->y + (y<<4);
		blt_src_rect.h = blt_dst_rect.h = num_rows;

		for (x=0; x<this->dirty_rects[this->numfb]->width; x++) {
			int num_cols = 16;

			if (this->dirty_rects[this->numfb]->markers[y*this->dirty_rects[this->numfb]->width + x] == 0) {
				continue;
			}

			if (((x+1)<<4) > dst_rect->x+dst_rect->w) {
				num_cols = dst_rect->x+dst_rect->w - (x<<4);
			}

			blt_src_rect.x = src_rect->x + (x<<4);
			blt_dst_rect.x = dst_rect->x + (x<<4);
			blt_src_rect.w = blt_dst_rect.w = num_cols;

			SDL_BlitSurface(render.texture->scaled, &blt_src_rect, this->screen, &blt_dst_rect);

			this->upload_rects[this->numfb]->setDirty(this->upload_rects[this->numfb],
				blt_dst_rect.x,blt_dst_rect.y,
				blt_dst_rect.w,blt_dst_rect.h);
		}
	}
}

static void bitmapScaledRtDirty(video_t *this, SDL_Rect *src_rect, SDL_Rect *dst_rect)
{
	SDL_Rect blt_src_rect, blt_dst_rect;
	int x,y, j;
	render_texture_t *tex = render.texture;
	SDL_Surface *surf = this->screen;

	for (y=0; y<this->dirty_rects[this->numfb]->height; y++) {
		int num_rows = 16;

		if (((y+1)<<4) > dst_rect->y+dst_rect->h) {
			num_rows = dst_rect->y+dst_rect->h - (y<<4);
		}

		blt_src_rect.y = src_rect->y + (y<<4);
		blt_dst_rect.y = dst_rect->y + (y<<4);
		blt_src_rect.h = blt_dst_rect.h = num_rows;

		for (x=0; x<this->dirty_rects[this->numfb]->width; x++) {
			int num_cols = 16;

			if (this->dirty_rects[this->numfb]->markers[y*this->dirty_rects[this->numfb]->width + x] == 0) {
				continue;
			}

			if (((x+1)<<4) > dst_rect->x+dst_rect->w) {
				num_cols = dst_rect->x+dst_rect->w - (x<<4);
			}

			blt_src_rect.x = src_rect->x + (x<<4);
			blt_dst_rect.x = dst_rect->x + (x<<4);
			blt_src_rect.w = blt_dst_rect.w = num_cols;

			switch(this->bpp) {
				case 8:
					{
						Uint8 *src = tex->pixels;
						src += blt_src_rect.y * tex->pitch;
						src += blt_src_rect.x;
						Uint8 *dst = surf->pixels;
						dst += blt_dst_rect.y * surf->pitch;
						dst += blt_dst_rect.x;

						for (j=0; j<blt_src_rect.h; j++) {
							memcpy(dst, src, blt_src_rect.w);
							src += tex->pitch;
							dst += surf->pitch;
						}
					}
					break;
				case 15:
				case 16:
					{
						Uint16 *src = (Uint16 *) tex->pixels;
						src += blt_src_rect.y * (tex->pitch>>1);
						src += blt_src_rect.x;
						Uint16 *dst = (Uint16 *) surf->pixels;
						dst += blt_dst_rect.y * (surf->pitch>>1);
						dst += blt_dst_rect.x;

						for (j=0; j<blt_src_rect.h; j++) {
							memcpy(dst, src, blt_src_rect.w<<1);
							src += tex->pitch>>1;
							dst += surf->pitch>>1;
						}
					}
					break;
				case 24:
					{
						Uint8 *src = tex->pixels;
						src += blt_src_rect.y * tex->pitch;
						src += blt_src_rect.x * 3;
						Uint8 *dst = surf->pixels;
						dst += blt_dst_rect.y * surf->pitch;
						dst += blt_dst_rect.x * 3;

						for (j=0; j<blt_src_rect.h; j++) {
							memcpy(dst, src, blt_src_rect.w*3);
							src += tex->pitch;
							dst += surf->pitch;
						}
					}
					break;
				case 32:
					{
						Uint32 *src = (Uint32 *) tex->pixels;
						src += blt_src_rect.y * (tex->pitch>>2);
						src += blt_src_rect.x;
						Uint32 *dst = (Uint32 *) surf->pixels;
						dst += blt_dst_rect.y * (surf->pitch>>2);
						dst += blt_dst_rect.x;

						for (j=0; j<blt_src_rect.h; j++) {
							memcpy(dst, src, blt_src_rect.w<<2);
							src += tex->pitch>>2;
							dst += surf->pitch>>2;
						}
					}
					break;
			}

			this->upload_rects[this->numfb]->setDirty(this->upload_rects[this->numfb],
				blt_dst_rect.x,blt_dst_rect.y,
				blt_dst_rect.w,blt_dst_rect.h);
		}
	}
}

static void refresh_scaled_version(video_t *video, render_texture_t *texture, int new_w, int new_h)
{
	int create_scaled = 0, new_bpp, x,y;
	render_texture_t *src;
	SDL_Surface *dst;

	/* Generate a cached version of texture scaled/dithered or both */
	if (texture->scaled) {
		/* Recreate if different target size */		
		if ((texture->scaled->w != new_w) || (texture->scaled->h != new_h)) {
			create_scaled = 1;
		}
	} else {
		create_scaled = (texture->w != new_w) || (texture->h != new_h)
			|| (video->screen->format->BytesPerPixel != texture->bpp)
			|| (video->screen->format->Rmask != texture->rmask)
			|| (video->screen->format->Gmask != texture->gmask)
			|| (video->screen->format->Bmask != texture->bmask)
			|| (video->screen->format->Amask != texture->amask)
			|| ((video->bpp == 8) && render.dithering);
	}

	/* Create new render_texture, for scaled size */
	if (!create_scaled) {
		return;
	}

	/* Free old one */
	if (texture->scaled) {
		SDL_FreeSurface(texture->scaled);
		texture->scaled = NULL;
	}

	logMsg(2, "bitmap: create scaled version of texture\n");

	new_bpp = 8;
	switch(texture->bpp) {
		case 2:
			new_bpp = 16;
			break;
		case 3:
			new_bpp = 24;
			break;
		case 4:
			new_bpp = 32;
			break;
	}

	texture->scaled = SDL_CreateRGBSurface(SDL_SWSURFACE, new_w,new_h,new_bpp,
		texture->rmask,texture->gmask,texture->bmask,texture->amask);

	if (!texture->scaled) {
		fprintf(stderr, "bitmap: could not create scaled texture\n");
		return;
	}

	if (new_bpp == 8) {
		dither_setpalette(texture->scaled);
	}

	src = texture;
	dst = texture->scaled;

	logMsg(2, "bitmap: scale texture from %dx%dx%d to %dx%dx%d\n",
		src->w,src->h,src->bpp*8, dst->w,dst->h,dst->format->BitsPerPixel);

	switch(texture->bpp) {
		case 1:
			{
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
			break;
		case 2:
			{
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
			break;
		case 3:
			{
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
			break;
		case 4:
			{
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
			break;
	}

	/* Dither if needed */
	if (video->bpp == 8) {
		SDL_Surface *dithered_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
			texture->scaled->w,texture->scaled->h,8, 0,0,0,0);
		if (!dithered_surf) {
			fprintf(stderr, "bitmap: can not create dithered texture\n");
			return;
		}
		
		dither_setpalette(dithered_surf);
		if (render.dithering) {
			logMsg(2, "bitmap: creating dithered version of texture\n");
			dither(texture->scaled, dithered_surf);
		} else {
			logMsg(2, "bitmap: creating 8bit version of texture\n");
			dither_copy(texture->scaled, dithered_surf);
		}
		SDL_FreeSurface(texture->scaled);
		texture->scaled = dithered_surf;
	}
}
