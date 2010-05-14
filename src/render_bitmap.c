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
#include "parameters.h"
#include "log.h"
#include "dither.h"
#include "render_bitmap.h"

/*--- Defines ---*/

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/*#define BILINEAR_FILTER(start,end,coef)	\
	(start) + ((((end)-(start)) * ((coef) & 65535)) >> 16)*/

/*--- Functions prototypes ---*/

static void shutdown(render_bitmap_t *this);

static void bitmapScaledRtNodirty(video_t *video, SDL_Rect *src_rect, SDL_Rect *dst_rect);
static void bitmapScaledScDirty(video_t *this, SDL_Rect *src_rect, SDL_Rect *dst_rect);
static void bitmapScaledRtDirty(video_t *this, SDL_Rect *src_rect, SDL_Rect *dst_rect);

static void refresh_scaled_version(video_t *video, render_texture_t *texture, int new_w, int new_h);
static void rescale_nearest(render_texture_t *src, SDL_Surface *dst);
static void rescale_linear(render_texture_t *src, SDL_Surface *dst);

static void clipSource(int x, int y, int w, int h);
static void clipDest(int x, int y, int w, int h);
static void setScaler(int srcw, int srch, int dstw, int dsth);
static void setDepth(int enabled, float depth);
static void setMasking(int enabled);
static void drawImage(video_t *video);

static void drawImageDepth(void);

/*--- Functions ---*/

void render_bitmap_soft_init(render_bitmap_t *render_bitmap)
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
	if (this->scalex_src2dst) {
		free(this->scalex_src2dst);
		this->scalex_src2dst=NULL;
	}
	if (this->scalex_src2dst) {
		free(this->scaley_src2dst);
		this->scaley_src2dst=NULL;
	}
	if (this->scalex_dst2src) {
		free(this->scalex_dst2src);
		this->scalex_dst2src=NULL;
	}
	if (this->scaley_dst2src) {
		free(this->scaley_dst2src);
		this->scaley_dst2src=NULL;
	}
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
	int i;
	render_bitmap_t *this = &render.bitmap;

	this->srcWidth = srcw;
	this->srcHeight = srch;
	this->dstWidth = dstw;
	this->dstHeight = dsth;

	if (srcw>this->sizex_src2dst) {
		this->scalex_src2dst = (Uint16 *) realloc(this->scalex_src2dst, sizeof(Uint16) * srcw);
		this->sizex_src2dst = srcw;
	}
	if (srch>this->sizey_src2dst) {
		this->scaley_src2dst = (Uint16 *) realloc(this->scaley_src2dst, sizeof(Uint16) * srch);
		this->sizey_src2dst = srch;
	}
	if (dstw>this->sizex_dst2src) {
		this->scalex_dst2src = (Uint16 *) realloc(this->scalex_dst2src, sizeof(Uint16) * dstw);
		this->sizex_dst2src = dstw;
	}
	if (dsth>this->sizey_dst2src) {
		this->scaley_dst2src = (Uint16 *) realloc(this->scaley_dst2src, sizeof(Uint16) * dsth);
		this->sizey_dst2src = dsth;
	}

	if ((srcw!=this->numx_src2dst) || (dstw!=this->numx_dst2src)) {
		for (i=0; i<srcw; i++) {
			this->scalex_src2dst[i] = (i*dstw)/srcw;
		}
		this->numx_src2dst = srcw;

		for (i=0; i<dstw; i++) {
			this->scalex_dst2src[i] = (i*srcw)/dstw;
		}
		this->numx_dst2src = dstw;
	}
	if ((srch!=this->numy_src2dst) || (dsth!=this->numy_dst2src)) {
		for (i=0; i<srch; i++) {
			this->scaley_src2dst[i] = (i*dsth)/srch;
		}
		this->numy_src2dst = srch;

		for (i=0; i<dsth; i++) {
			this->scaley_dst2src[i] = (i*srch)/dsth;
		}
		this->numy_dst2src = dsth;
	}

	refresh_scaled_version(&video, render.texture, dstw,dsth);
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

static void drawImage(video_t *video)
{
	render_bitmap_t *this = &render.bitmap;

	if (!render.texture)
		return;

	/* Clip in source texture */
	if (render.bitmap.srcRect.x<0) {
		render.bitmap.srcRect.w += render.bitmap.srcRect.x;
		render.bitmap.dstRect.x -= this->scalex_src2dst[render.bitmap.srcRect.x];
		render.bitmap.srcRect.x = 0;
	} else if (render.bitmap.srcRect.x+render.bitmap.srcRect.w>render.texture->w) {
		render.bitmap.srcRect.w = render.texture->w - render.bitmap.srcRect.x;
		render.bitmap.dstRect.w = this->scalex_src2dst[render.bitmap.srcRect.w];
	}
	if (render.bitmap.srcRect.y<0) {
		render.bitmap.srcRect.h += render.bitmap.srcRect.y;
		render.bitmap.dstRect.y -= this->scaley_src2dst[render.bitmap.srcRect.y];
		render.bitmap.srcRect.y = 0;
	} else if (render.bitmap.srcRect.y+render.bitmap.srcRect.h>render.texture->h) {
		render.bitmap.srcRect.h = render.texture->h - render.bitmap.srcRect.y;
		render.bitmap.dstRect.h = this->scaley_src2dst[render.bitmap.srcRect.h];
	}

	/* Clip in dest screen */
	if (render.bitmap.dstRect.x<video->viewport.x) {
		render.bitmap.dstRect.w += render.bitmap.dstRect.x;
		render.bitmap.srcRect.x -= this->scalex_dst2src[render.bitmap.dstRect.x];
		render.bitmap.dstRect.x = 0;
	} else if (render.bitmap.dstRect.x+render.bitmap.dstRect.w>video->viewport.x+video->viewport.w) {
		render.bitmap.dstRect.w = video->viewport.w - render.bitmap.dstRect.x;
		render.bitmap.srcRect.w = this->scalex_dst2src[render.bitmap.dstRect.w];
	}
	if (render.bitmap.dstRect.y<video->viewport.y) {
		render.bitmap.dstRect.h += render.bitmap.dstRect.y;
		render.bitmap.srcRect.y -= this->scaley_dst2src[render.bitmap.dstRect.y];
		render.bitmap.dstRect.y = 0;
	} else if (render.bitmap.dstRect.y+render.bitmap.dstRect.h>video->viewport.y+video->viewport.h) {
		render.bitmap.dstRect.h = video->viewport.h - render.bitmap.dstRect.y;
		render.bitmap.srcRect.h = this->scaley_dst2src[render.bitmap.dstRect.h];
	}

	/* Clipping for out of bounds in source */
	if ((render.bitmap.srcRect.x>=render.texture->w)
	   || (render.bitmap.srcRect.y>=render.texture->h)
	   || (render.bitmap.srcRect.x+render.bitmap.srcRect.w<0)
	   || (render.bitmap.srcRect.y+render.bitmap.srcRect.h<0))
	{
		return;
	}

	/* Clipping for out of bounds in dest */
	if ((render.bitmap.dstRect.x>=video->viewport.x+video->viewport.w)
	   || (render.bitmap.dstRect.y>=video->viewport.y+video->viewport.h)
	   || (render.bitmap.dstRect.x+render.bitmap.dstRect.w<video->viewport.x)
	   || (render.bitmap.dstRect.y+render.bitmap.dstRect.h<video->viewport.y))
	{
		return;
	}

	if (render.bitmap.depth_test) {
		render.bitmap.dstRect.x -= video->viewport.x;
		render.bitmap.dstRect.y -= video->viewport.y;

		drawImageDepth();
		return;
	}

	if (render.texture->scaled) {
		SDL_Rect scaled_src;

		scaled_src.x = this->scalex_src2dst[render.bitmap.srcRect.x];
		scaled_src.y = this->scaley_src2dst[render.bitmap.srcRect.y];
		scaled_src.w = this->scalex_src2dst[render.bitmap.srcRect.w];
		scaled_src.h = this->scaley_src2dst[render.bitmap.srcRect.h];

		if (render.useDirtyRects) {
			bitmapScaledScDirty(video, &scaled_src, &render.bitmap.dstRect);
		} else {
			SDL_BlitSurface(render.texture->scaled, &scaled_src,
				video->screen, &render.bitmap.dstRect);
			/*SDL_FillRect(video->screen, &render.bitmap.dstRect, 0xffffffff);*/
		}
	} else {
		if (SDL_MUSTLOCK(video->screen)) {
			SDL_LockSurface(video->screen);
		}

		if (render.useDirtyRects) {
			bitmapScaledRtDirty(video, &render.bitmap.srcRect, &render.bitmap.dstRect);
		} else {
			bitmapScaledRtNodirty(video, &render.bitmap.srcRect, &render.bitmap.dstRect);
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
				Uint8 *dst = surf->pixels;

				src += src_rect->y * tex->pitch;
				src += src_rect->x;
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
				Uint16 *dst = (Uint16 *) surf->pixels;

				src += src_rect->y * (tex->pitch>>1);
				src += src_rect->x;
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
				Uint8 *dst = surf->pixels;

				src += src_rect->y * tex->pitch;
				src += src_rect->x * 3;
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
				Uint32 *dst = (Uint32 *) surf->pixels;

				src += src_rect->y * (tex->pitch>>2);
				src += src_rect->x;
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
	int x,y, dx,dy, num_rows=0, num_cols=0;

	for (y=0; y<dst_rect->h; y+=num_rows) {
		blt_src_rect.y = src_rect->y + y;
		blt_dst_rect.y = dst_rect->y + y;

		dy = blt_dst_rect.y>>4;
		num_rows = MIN(16 - (blt_dst_rect.y & 15), dst_rect->h - y);

		blt_src_rect.h = blt_dst_rect.h = num_rows;

		for (x=0; x<dst_rect->w; x+=num_cols) {
			blt_src_rect.x = src_rect->x + x;
			blt_dst_rect.x = dst_rect->x + x;

			dx = blt_dst_rect.x>>4;
			num_cols = MIN(16 - (blt_dst_rect.x & 15), dst_rect->w - x);

			blt_src_rect.w = blt_dst_rect.w = num_cols;

			if (this->dirty_rects[this->numfb]->markers[dy*this->dirty_rects[this->numfb]->width + dx] == 0) {
				continue;
			}

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
	int x,y, j, num_rows=0, num_cols=0, dx,dy;
	render_texture_t *tex = render.texture;
	SDL_Surface *surf = this->screen;

	for (y=0; y<dst_rect->h; y+=num_rows) {
		blt_src_rect.y = src_rect->y + y;
		blt_dst_rect.y = dst_rect->y + y;

		dy = blt_dst_rect.y>>4;
		num_rows = MIN(16 - (blt_dst_rect.y & 15), dst_rect->h - y);

		blt_src_rect.h = blt_dst_rect.h = num_rows;

		for (x=0; x<dst_rect->w; x+=num_cols) {
			blt_src_rect.x = src_rect->x + x;
			blt_dst_rect.x = dst_rect->x + x;

			dx = blt_dst_rect.x>>4;
			num_cols = MIN(16 - (blt_dst_rect.x & 15), dst_rect->w - x);

			blt_src_rect.w = blt_dst_rect.w = num_cols;

			if (this->dirty_rects[this->numfb]->markers[dy*this->dirty_rects[this->numfb]->width + dx] == 0) {
				continue;
			}

			switch(this->bpp) {
				case 8:
					{
						Uint8 *src = tex->pixels;
						Uint8 *dst = surf->pixels;

						src += blt_src_rect.y * tex->pitch;
						src += blt_src_rect.x;
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
						Uint16 *dst = (Uint16 *) surf->pixels;

						src += blt_src_rect.y * (tex->pitch>>1);
						src += blt_src_rect.x;
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
						Uint8 *dst = surf->pixels;

						src += blt_src_rect.y * tex->pitch;
						src += blt_src_rect.x * 3;
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
						Uint32 *dst = (Uint32 *) surf->pixels;

						src += blt_src_rect.y * (tex->pitch>>2);
						src += blt_src_rect.x;
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
	int create_scaled = 0, new_bpp;

	/* Generate a cached version of texture scaled/dithered or both */
	if (texture->scaled) {
		/* Recreate if different target size */		
		if ((texture->scaled->w != new_w) || (texture->scaled->h != new_h)) {
			create_scaled = 1;
		}
	} else {
		create_scaled = (texture->w != new_w) || (texture->h != new_h)
			|| (video->screen->format->BytesPerPixel != texture->bpp)
			|| (video->screen->format->Rmask != texture->format.Rmask)
			|| (video->screen->format->Gmask != texture->format.Gmask)
			|| (video->screen->format->Bmask != texture->format.Bmask)
			|| (video->screen->format->Amask != texture->format.Amask)
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
		texture->format.Rmask, texture->format.Gmask,
		texture->format.Bmask, texture->format.Amask);

	if (!texture->scaled) {
		fprintf(stderr, "bitmap: could not create scaled texture\n");
		return;
	}

	if (new_bpp == 8) {
		/*dither_setpalette(texture->scaled);*/

		int i;

		SDL_Palette *palette = texture->scaled->format->palette;
		SDL_PixelFormat *fmt = video->screen->format;
		for (i=0; i<256; i++) {
			Uint8 r,g,b;

			SDL_GetRGB(texture->palettes[0][i], fmt, &r,&g,&b);

			palette->colors[i].r = r;
			palette->colors[i].g = g;
			palette->colors[i].b = b;
		}
	}

	if (params.linear) {
		rescale_linear(texture, texture->scaled);
	} else {
		rescale_nearest(texture, texture->scaled);
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

#if 0
	/* Set color key */
	if (texture->num_palettes==1) {
		int i;

		for (i=0; i<256; i++) {
			if (!texture->alpha_palettes[0][i]) {
				SDL_SetColorKey(texture->scaled, SDL_SRCCOLORKEY, texture->palettes[0][i]);
				break;
			}
		}
	}
#endif
}

static void rescale_nearest(render_texture_t *src, SDL_Surface *dst)
{
	int x,y;

	logMsg(2, "bitmap: scale texture from %dx%dx%d to %dx%dx%d\n",
		src->w,src->h,src->bpp*8, dst->w,dst->h,dst->format->BitsPerPixel);

	switch(src->bpp) {
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
}

static Sint32 BILINEAR_FILTER(Sint32 start, Sint32 end, Sint32 coef)
{
	return start+(((end-start)*(coef & 65535))>>16);
}

static void rescale_linear(render_texture_t *src, SDL_Surface *dst)
{
	int x,y, i;
	Sint32 u=0,v=0;
	Sint32 du = (src->w * 65536) / dst->w;
	Sint32 dv = (src->h * 65536) / dst->h;

	logMsg(2, "bitmap: scale texture linearly from %dx%dx%d to %dx%dx%d\n",
		src->w,src->h,src->bpp*8, dst->w,dst->h,dst->format->BitsPerPixel);

	switch(src->bpp) {
		case 1:
			rescale_nearest(src, dst);
			break;
		case 2:
			{
				Uint16 *dst_line = dst->pixels;

				for(y=0; y<dst->h; y++) {
					Uint16 *dst_col = dst_line;
					Uint16 *src_line_r0 = (Uint16 *) src->pixels;
					Uint16 *src_line_r1 = src_line_r0;

					src_line_r0 += (v>>16) * (src->pitch>>1);
					src_line_r1 += MIN(src->h-1,((v>>16)+1)) * (src->pitch>>1);

					u = 0;
					for(x=0; x<dst->w; x++) {
						Uint16 *src_col[4];
						Sint32 r[4], dr, dr_line0, dr_line1;
						Sint32 g[4], dg, dg_line0, dg_line1;
						Sint32 b[4], db, db_line0, db_line1;
						int u1 = MIN(src->w-1, (u>>16)+1);

						src_col[0] = &src_line_r0[u>>16];
						src_col[1] = &src_line_r0[u1];
						src_col[2] = &src_line_r1[(u>>16)];
						src_col[3] = &src_line_r1[u1];

						for (i=0; i<4; i++) {
							Uint8 mr,mg,mb;

							SDL_GetRGB(*(src_col[i]), &(src->format), &mr, &mg, &mb);
							r[i]= mr;
							g[i]= mg;
							b[i]= mb;
						}

						dr_line0 = BILINEAR_FILTER(r[0],r[1],u);
						dr_line1 = BILINEAR_FILTER(r[2],r[3],u);
						dr = BILINEAR_FILTER(dr_line0, dr_line1, v);

						dg_line0 = BILINEAR_FILTER(g[0],g[1],u);
						dg_line1 = BILINEAR_FILTER(g[2],g[3],u);
						dg = BILINEAR_FILTER(dg_line0, dg_line1, v);

						db_line0 = BILINEAR_FILTER(b[0],b[1],u);
						db_line1 = BILINEAR_FILTER(b[2],b[3],u);
						db = BILINEAR_FILTER(db_line0, db_line1, v);

						*dst_col++ = SDL_MapRGB(&(src->format), dr,dg,db);

						u += du;
					}
					dst_line += dst->pitch>>1;
					v += dv;
				}
			}
			break;
		case 3:
			{
				Uint8 *dst_line = dst->pixels;

				for(y=0; y<dst->h; y++) {
					Uint8 *dst_col = dst_line;
					Uint8 *src_line_r0 = (Uint8 *) src->pixels;
					Uint8 *src_line_r1 = src_line_r0;

					src_line_r0 += (v>>16) * src->pitch;
					src_line_r1 += MIN(src->h-1,((v>>16)+1)) * src->pitch;

					u = 0;
					for(x=0; x<dst->w; x++) {
						Uint8 *src_col[4];
						Sint32 r[4], dr, dr_line0, dr_line1;
						Sint32 g[4], dg, dg_line0, dg_line1;
						Sint32 b[4], db, db_line0, db_line1;
						Uint32 color;
						int u1 = MIN(src->w-1, (u>>16)+1);

						src_col[0] = &src_line_r0[(u>>16)*3];
						src_col[1] = &src_line_r0[u1*3];
						src_col[2] = &src_line_r1[(u>>16)*3];
						src_col[3] = &src_line_r1[u1*3];

						for (i=0; i<4; i++) {
							Uint8 mr,mg,mb;
							Uint8 *src_pix = src_col[i];
							color = (src_pix[0]<<16)|(src_pix[1]<<8)|src_pix[2];

							SDL_GetRGB(color, &(src->format), &mr, &mg, &mb);
							r[i]= mr;
							g[i]= mg;
							b[i]= mb;
						}

						dr_line0 = BILINEAR_FILTER(r[0],r[1],u);
						dr_line1 = BILINEAR_FILTER(r[2],r[3],u);
						dr = BILINEAR_FILTER(dr_line0, dr_line1, v);

						dg_line0 = BILINEAR_FILTER(g[0],g[1],u);
						dg_line1 = BILINEAR_FILTER(g[2],g[3],u);
						dg = BILINEAR_FILTER(dg_line0, dg_line1, v);

						db_line0 = BILINEAR_FILTER(b[0],b[1],u);
						db_line1 = BILINEAR_FILTER(b[2],b[3],u);
						db = BILINEAR_FILTER(db_line0, db_line1, v);

						color = SDL_MapRGB(&(src->format), dr,dg,db);
						*dst_col++ = color>>16;
						*dst_col++ = color>>8;
						*dst_col++ = color;

						u += du;
					}
					dst_line += dst->pitch;
					v += dv;
				}
			}
			break;
		case 4:
			{
				Uint32 *dst_line = dst->pixels;

				for(y=0; y<dst->h; y++) {
					Uint32 *dst_col = dst_line;
					Uint32 *src_line_r0 = (Uint32 *) src->pixels;
					Uint32 *src_line_r1 = src_line_r0;

					src_line_r0 += (v>>16) * (src->pitch>>2);
					src_line_r1 += MIN(src->h-1,((v>>16)+1)) * (src->pitch>>2);

					u = 0;
					for(x=0; x<dst->w; x++) {
						Uint32 *src_col[4];
						Sint32 r[4], dr, dr_line0, dr_line1;
						Sint32 g[4], dg, dg_line0, dg_line1;
						Sint32 b[4], db, db_line0, db_line1;
						int u1 = MIN(src->w-1, (u>>16)+1);

						src_col[0] = &src_line_r0[u>>16];
						src_col[1] = &src_line_r0[u1];
						src_col[2] = &src_line_r1[(u>>16)];
						src_col[3] = &src_line_r1[u1];

						for (i=0; i<4; i++) {
							Uint8 mr,mg,mb;

							SDL_GetRGB(*(src_col[i]), &(src->format), &mr, &mg, &mb);
							r[i]= mr;
							g[i]= mg;
							b[i]= mb;
						}

						dr_line0 = BILINEAR_FILTER(r[0],r[1],u);
						dr_line1 = BILINEAR_FILTER(r[2],r[3],u);
						dr = BILINEAR_FILTER(dr_line0, dr_line1, v);

						dg_line0 = BILINEAR_FILTER(g[0],g[1],u);
						dg_line1 = BILINEAR_FILTER(g[2],g[3],u);
						dg = BILINEAR_FILTER(dg_line0, dg_line1, v);

						db_line0 = BILINEAR_FILTER(b[0],b[1],u);
						db_line1 = BILINEAR_FILTER(b[2],b[3],u);
						db = BILINEAR_FILTER(db_line0, db_line1, v);

						*dst_col++ = SDL_MapRGB(&(src->format), dr,dg,db);

						u += du;
					}
					dst_line += dst->pitch>>2;
					v += dv;
				}
			}
			break;
	}
}

static void drawImageDepth(void)
{
	vertexf_t poly[4];
	int i;

	poly[0].pos[0] = render.bitmap.dstRect.x;
	poly[0].pos[1] = render.bitmap.dstRect.y;
	poly[0].tx[0] = render.bitmap.srcRect.x;
	poly[0].tx[1] = render.bitmap.srcRect.y;

	poly[1].pos[0] = render.bitmap.dstRect.x+render.bitmap.dstRect.w;
	poly[1].pos[1] = render.bitmap.dstRect.y;
	poly[1].tx[0] = render.bitmap.srcRect.x+render.bitmap.srcRect.w;
	poly[1].tx[1] = render.bitmap.srcRect.y;

	poly[2].pos[0] = render.bitmap.dstRect.x+render.bitmap.dstRect.w;
	poly[2].pos[1] = render.bitmap.dstRect.y+render.bitmap.dstRect.h;
	poly[2].tx[0] = render.bitmap.srcRect.x+render.bitmap.srcRect.w;
	poly[2].tx[1] = render.bitmap.srcRect.y+render.bitmap.srcRect.h;

	poly[3].pos[0] = render.bitmap.dstRect.x;
	poly[3].pos[1] = render.bitmap.dstRect.y+render.bitmap.dstRect.h;
	poly[3].tx[0] = render.bitmap.srcRect.x;
	poly[3].tx[1] = render.bitmap.srcRect.y+render.bitmap.srcRect.h;

	for (i=0; i<4; i++) {
		poly[i].pos[0] *= render.bitmap.depth;
		poly[i].pos[1] *= render.bitmap.depth;
		poly[i].pos[2] = render.bitmap.depth;
		poly[i].pos[3] = 1.0f;
	}

	render.draw.polyTexture(&render.draw, poly, 4);
}
