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

#include "r_misc.h"
#include "render.h"
#include "render_texture.h"
#include "render_texture_list.h"

#include "../r_soft/dither.h"

/*--- Functions prototypes ---*/

static void shutdown(render_texture_t *this);

static void upload(render_texture_t *this, int num_pal);
static void download(render_texture_t *this);

static void prepare_resize(render_texture_t *this, int *w, int *h);
static void resize(render_texture_t *this, int w, int h);

static void load_from_tim(render_texture_t *this, void *tim_ptr);
static void read_rgba(Uint16 color, int *r, int *g, int *b, int *a);

static void load_from_surf(render_texture_t *this, SDL_Surface *surf);

/* Keep texture in surface format */
static void copy_tex_palette(render_texture_t *this, SDL_Surface *surf);
static void copy_surf_to_tex(render_texture_t *this, SDL_Surface *surf);

/* Convert texture from surface to video format */
static void convert_tex_palette(render_texture_t *this, SDL_Surface *surf);
static void convert_surf_to_tex(render_texture_t *this, SDL_Surface *surf);

static void copy_pixels(render_texture_t *this, SDL_Surface *surf);

/*--- Functions ---*/

render_texture_t *render_texture_create(int flags)
{
	render_texture_t *tex;

	tex = calloc(1, sizeof(render_texture_t));
	if (!tex) {
		fprintf(stderr, "Can not allocate memory for render_texture\n");
		return NULL;
	}

	tex->shutdown = shutdown;
	tex->upload = upload;
	tex->download = download;
	tex->prepare_resize = prepare_resize;
	tex->resize = resize;
	tex->load_from_tim = load_from_tim;
	tex->load_from_surf = load_from_surf;
/*	tex->mark_trans = mark_trans;*/

	tex->must_pot = flags & RENDER_TEXTURE_MUST_POT;
	tex->cacheable = flags & RENDER_TEXTURE_CACHEABLE;
	tex->keep_palette = flags & RENDER_TEXTURE_KEEPPALETTE;

	tex->bpp = video.screen->format->BytesPerPixel;
	/* FIXME: copy palette from format elsewhere */
	memcpy(&(tex->format), video.screen->format, sizeof(SDL_PixelFormat));
	tex->format.palette = NULL;

	memset(tex->palettes, 0, sizeof(tex->palettes));

	list_render_texture_add(tex);

	/* Dummy texture */
	tex->bpp = 2;
	tex->resize(tex, 16,16);

	return tex;
}

static void shutdown(render_texture_t *this)
{
	if (!this) {
		return;
	}

	if (this->scaled) {
		SDL_FreeSurface(this->scaled);
		this->scaled = NULL;
	}
	if (this->pixels) {
		free(this->pixels);
		this->pixels = NULL;
	}

	list_render_texture_remove(this);
	free(this);
}

static void upload(render_texture_t *this, int num_pal)
{
}

static void download(render_texture_t *this)
{
}

static void prepare_resize(render_texture_t *this, int *w, int *h)
{
	int new_bound_w = *w;
	int new_bound_h = *h;

	if (this->must_pot) {
		int potw = logbase2(new_bound_w);
		int poth = logbase2(new_bound_h);
		if (new_bound_w != (1<<potw)) {
			new_bound_w = 1<<(potw+1);
		}
		if (new_bound_h != (1<<poth)) {
			new_bound_h = 1<<(poth+1);
		}
	}

	*w = new_bound_w;
	*h = new_bound_h;
}

static void resize(render_texture_t *this, int w, int h)
{
	Uint8 *new_pixels;
	int new_bound_w, new_bound_h;
	int new_pitch;

	if (!this) {
		return;
	}

	/* We have enough room ? */
	if ((w <= this->pitchw) && (h <= this->pitchh)) {
		this->w = w;
		this->h = h;

		/* Update pitch */
		this->pitch = this->w * this->bpp;
		return;
	}

	new_bound_w = w;
	new_bound_h = h;
	this->prepare_resize(this, &new_bound_w, &new_bound_h);

	logMsg(2, "texture: resize %dx%d to %dx%d\n",
		this->pitchw,this->pitchh, new_bound_w,new_bound_h);

	new_pitch = new_bound_w * this->bpp;

	new_pixels = (Uint8 *) malloc(new_pitch * new_bound_h);
	if (!new_pixels) {
		fprintf(stderr, "Can not allocate %d for render_texture pixels\n", new_pitch * new_bound_h);
		return;
	}

	/* Copy old data, if any */
	if (this->pixels) {
		Uint8 *src = this->pixels;
		Uint8 *dst = new_pixels;
		int y;

		for (y=0; y<this->h; y++) {
			memcpy(dst, src, this->pitch);
			src += this->pitch;
			dst += new_pitch;
		}

		free(this->pixels);
	}

	this->pixels = new_pixels;
	this->w = w;
	this->h = h;

	this->pitch = new_pitch;
	this->pitchw = new_bound_w;
	this->pitchh = new_bound_h;

	this->download(this);
}

static void load_from_tim(render_texture_t *this, void *tim_ptr)
{
#ifdef MAGIC_TIM
	tim_header_t *tim_header;
	Uint16 *pal_header;
	int num_colors, num_palettes, i,j, paletted, img_offset;
	int w,h, tim_type;
	tim_size_t *tim_size;
	SDL_PixelFormat *fmt = video.screen->format /*&(this->format)*/;
/*	int bytes_per_pixel;*/

	if (!this || !tim_ptr) {
		return;
	}

	/* Read dimensions */
	tim_header = (tim_header_t *) tim_ptr;
	if (SDL_SwapLE32(tim_header->magic) != MAGIC_TIM) {
		fprintf(stderr, "Not a TIM file\n");
		return;
	}

	num_palettes = SDL_SwapLE16(tim_header->nb_palettes);
	if (num_palettes>MAX_TEX_PALETTE) {
		fprintf(stderr, "Does not support %d palettes per texture\n", num_palettes);
		return;
	}

	num_colors = SDL_SwapLE16(tim_header->palette_colors);

	img_offset = SDL_SwapLE32(tim_header->offset) + 20;

	tim_size = (tim_size_t *) (&((Uint8 *) tim_ptr)[img_offset-4]);
	w = SDL_SwapLE16(tim_size->width);
	h = SDL_SwapLE16(tim_size->height);

	tim_type = SDL_SwapLE32(tim_header->type);
/*	bytes_per_pixel = 1;*/
	paletted = 1;
	switch(tim_type) {
		case TIM_TYPE_4:
			logMsg(3, "texture: 4 bits source\n");
			w <<= 2;
			break;
		case TIM_TYPE_8:
			logMsg(3, "texture: 8 bits source\n");
			w <<= 1;
			break;
		case TIM_TYPE_16:
			logMsg(3, "texture: 16 bits source\n");
			paletted = 0;
/*			bytes_per_pixel=2;
			if (fmt->BytesPerPixel>2) {
				bytes_per_pixel=4;
			}*/
			break;
	}
	if ((w==0) || (h==0)) {
		fprintf(stderr, "Can not read image dimension\n");
		return;
	}

	logMsg(2, "texture: %dx%d, %d palettes * %d colors\n", w,h, num_palettes, num_colors);

	/* Fill palettes */
	this->paletted = paletted;
	this->num_palettes = paletted ? num_palettes : 0;

	if (paletted) {
		pal_header = & ((Uint16 *) tim_ptr)[sizeof(tim_header_t)/2];
		for (i=0; i<num_palettes; i++) {
			for (j=0; j<num_colors; j++) {
				int r,g,b,a;

				Uint16 color = *pal_header++;
				color = SDL_SwapLE16(color);

				read_rgba(color, &r,&g,&b,&a);

				if (params.use_opengl) {
					this->palettes[i][j] = (a<<24)|(r<<16)|(g<<8)|b;
				} else {
					this->palettes[i][j] = SDL_MapRGBA(fmt, r,g,b,a);
				}
				this->alpha_palettes[i][j] = a;
			}
		}
	}

	/* Set bpp from texture, before resize */
	switch(tim_type) {
		case TIM_TYPE_4:
		case TIM_TYPE_8:
			this->bpp = 1;
			break;
		case TIM_TYPE_16:
			if (params.use_opengl) {
				this->bpp = 2;
			}
			break;
	}
	this->resize(this, w,h);

	logMsg(2, "texture: R=0x%08x, G=0x%08x, B=0x%08x, A=0x%08x\n",
		this->format.Rmask, this->format.Gmask,
		this->format.Bmask, this->format.Amask);

	/* Copy data */
	switch(tim_type) {
		case TIM_TYPE_4:
			{
				Uint8 *src_pixels = &((Uint8 *) tim_ptr)[img_offset];
				Uint8 *tex_pixels = this->pixels;
				for (i=0; i<h; i++) {
					Uint8 *tex_line = tex_pixels;
					for (j=0; j<w>>1; j++) {
						Uint8 color = *src_pixels++;

						*tex_line++ = color & 15;
						*tex_line++ = (color>>4) & 15;
					}
					tex_pixels += this->pitch;
				}
			}
			break;
		case TIM_TYPE_8:
			{
				Uint8 *src_pixels = &((Uint8 *) tim_ptr)[img_offset];
				Uint8 *tex_pixels = this->pixels;
				for (i=0; i<h; i++) {
					Uint8 *tex_line = tex_pixels;
					for (j=0; j<w; j++) {
						*tex_line++ = *src_pixels++;
					}
					tex_pixels += this->pitch;
				}
			}
			break;
		case TIM_TYPE_16:
			{
				int bytesPerPixel = fmt->BytesPerPixel;
				int r,g,b,a, color;
				Uint16 *src_pixels = (Uint16 *) (&((Uint8 *) tim_ptr)[img_offset]);

				/* With OpenGL, we can keep source in its proper format */
				if (params.use_opengl) {
					bytesPerPixel = 2;
				}

				switch(bytesPerPixel) {
					case 2:
						{
							Uint16 *tex_pixels = (Uint16 *) this->pixels;
							for (i=0; i<h; i++) {
								Uint16 *tex_line = tex_pixels;
								for (j=0; j<w; j++) {
									color = *src_pixels++;
									color = SDL_SwapLE16(color);

									read_rgba(color, &r,&g,&b,&a);

									if (params.use_opengl) {
										Uint16 c = (r<<8) & (31<<11);
										c |= (g<<3) & (63<<5);
										c |= (b>>3) & 31;
										*tex_line++ = c;
									} else {
										*tex_line++ = SDL_MapRGBA(fmt, r,g,b,a);
									}
								}
								tex_pixels += this->pitch>>1;
							}
						}
						break;
					case 3:
					case 4:
						{
							Uint32 *tex_pixels = (Uint32 *) this->pixels;
							for (i=0; i<h; i++) {
								Uint32 *tex_line = tex_pixels;
								for (j=0; j<w; j++) {
									color = *src_pixels++;
									color = SDL_SwapLE16(color);

									read_rgba(color, &r,&g,&b,&a);
									
									*tex_line++ = SDL_MapRGBA(fmt, r,g,b,a);
								}
								tex_pixels += this->pitch>>1;
							}
						}
						break;
				}
				
			}
			break;
	}

	/* Single palette, 8 bit image ? Mark transparent zones now */
	/*if ((this->bpp == 1) && (this->paletted) && (this->num_palettes == 1)) {
		mark_trans(this, 0, 0,0, this->w,this->h);
	}*/

	this->download(this);
#endif
}

static void read_rgba(Uint16 color, int *r, int *g, int *b, int *a)
{
	int r1,g1,b1,a1;

	r1 = color & 31;
	r1 = (r1<<3)|(r1>>2);

	g1 = (color>>5) & 31;
	g1 = (g1<<3)|(g1>>2);

	b1 = (color>>10) & 31;
	b1 = (b1<<3)|(b1>>2);

	a1 = (color>>15) & 1;
	if (r1+g1+b1 == 0) {
		a1 = (a1 ? 0xff : 0);
	} else {
		a1 = 0xff;
	}

	*r = r1;
	*g = g1;
	*b = b1;
	*a = a1;
}

static void load_from_surf(render_texture_t *this, SDL_Surface *surf)
{
	if (!this || !surf) {
		return;
	}

	memcpy(&(this->format), surf->format, sizeof(SDL_PixelFormat));

	if (this->cacheable) {
		logMsg(2, "keep texture in original format\n");
		copy_tex_palette(this, surf);
		copy_surf_to_tex(this, surf);
	} else {
		logMsg(2, "convert texture to video format\n");
		if (this->keep_palette) {
			copy_tex_palette(this, surf);
		} else {
			convert_tex_palette(this, surf);
		}
		convert_surf_to_tex(this, surf);
	}

	this->download(this);
}

/* Copy surface data to texture in original format */

static void copy_tex_palette(render_texture_t *this, SDL_Surface *surf)
{
	int i, r,g,b,a;
	SDL_Palette *surf_palette;

	this->num_palettes = this->paletted = ((surf->format->BitsPerPixel==8) && surf->format->palette);

	if (!this->paletted)
		return;

	surf_palette = surf->format->palette;

	for (i=0; i<surf->format->palette->ncolors; i++) {
		SDL_Color *color = &(surf_palette->colors[i]);

		r = color->r;
		g = color->g;
		b = color->b;
		a = 0xff;

		if (params.use_opengl) {
			this->palettes[0][i] = (a<<24)|(r<<16)|(g<<8)|b;
		} else {
			this->palettes[0][i] = SDL_MapRGBA(surf->format, r,g,b,a);
		}
		this->alpha_palettes[0][i] = a;
	}
}

static void copy_surf_to_tex(render_texture_t *this, SDL_Surface *surf)
{
	/* Set bpp from texture, before resize */
	this->bpp = surf->format->BytesPerPixel;
	this->resize(this, surf->w,surf->h);

	copy_pixels(this, surf);
}

/* Convert surface data to texture in video format
	video		surf		operation
	paletted	paletted	convert color indexes
	non paletted	paletted	convert palette, done
	paletted	non paletted	convert to dithered surface
	non paletted	non paletted	convert surface */

static void convert_tex_palette(render_texture_t *this, SDL_Surface *surf)
{
	int i, r,g,b,a;
	SDL_Palette *surf_palette;
	SDL_PixelFormat *fmt = video.screen->format;

	this->num_palettes = this->paletted = ((surf->format->BitsPerPixel==8) && surf->format->palette);

	if (!this->paletted)
		return;

	surf_palette = surf->format->palette;

	for (i=0; i<surf->format->palette->ncolors; i++) {
		SDL_Color *color = &(surf_palette->colors[i]);

		r = color->r;
		g = color->g;
		b = color->b;
		a = 0xff;

		if (params.use_opengl) {
			this->palettes[0][i] = (a<<24)|(r<<16)|(g<<8)|b;
		} else {
			this->palettes[0][i] = SDL_MapRGBA(fmt, r,g,b,a);
		}
		this->alpha_palettes[0][i] = a;
	}
}

static void convert_surf_to_tex(render_texture_t *this, SDL_Surface *surf)
{
	SDL_PixelFormat *fmt = video.screen->format;
	SDL_Surface *tmp_surf;
	int i,j;

	/* Set bpp from texture, before resize */
	this->bpp = surf->format->BytesPerPixel;
	if (this->bpp>1) { 
		this->bpp = (fmt->BytesPerPixel==3 ? 4 : fmt->BytesPerPixel);
	}
	this->resize(this, surf->w,surf->h);

	if (video.bpp==8) {
		if (surf->format->BytesPerPixel==1) {
			if (this->keep_palette) {
				/* Keep texture palette index as is */
				logMsg(2, "texture: do not convert color indices\n");

				copy_pixels(this, surf);
			} else {
				/* Convert color indexes */
				SDL_Palette *surf_palette = surf->format->palette;
				Uint8 *src_pixels = surf->pixels;
				Uint8 *tex_pixels = this->pixels;

				logMsg(2, "texture: convert color indices\n");

				for (i=0; i<this->h; i++) {
					Uint8 *src_line = src_pixels;
					Uint8 *tex_line = tex_pixels;

					for (j=0; j<this->w; j++) {
						int r,g,b;
						Uint8 color;

						color= *src_line++;

						r = surf_palette->colors[color].r;
						g = surf_palette->colors[color].g;
						b = surf_palette->colors[color].b;

						*tex_line++ = dither_nearest_index(r,g,b);
					}

					src_pixels += surf->pitch;
					tex_pixels += this->pitch;
				}
			}
		} else {
			/* Convert to dithered surface */
			tmp_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, surf->w,surf->h,8, 0,0,0,0);
			if (tmp_surf) {
				dither_setpalette(tmp_surf);
				if (render.dithering) {
					dither(surf, tmp_surf);
				} else {
					dither_copy(surf, tmp_surf);
				}

				logMsg(2, "texture: converted to 8bits dither palette\n");

				this->format.BitsPerPixel = 8;
				this->bpp = 1;
				this->resize(this, tmp_surf->w,tmp_surf->h);	/* recalc pitch */

				copy_pixels(this, tmp_surf);

				convert_tex_palette(this, tmp_surf);

				SDL_FreeSurface(tmp_surf);
			}
		}
	} else {
		if (surf->format->BytesPerPixel==1) {
			logMsg(2, "texture: keep as paletted texture, screen format\n");

			copy_pixels(this, surf);
		} else {
			SDL_PixelFormat tmpFmt;

			memcpy(&tmpFmt, fmt, sizeof(SDL_PixelFormat));
			if (tmpFmt.BytesPerPixel == 3) {
				/* Convert textures to 32bits, for 24bits video mode */
				tmpFmt.BytesPerPixel = 4;
				tmpFmt.BitsPerPixel = 32;
			}

			if (params.use_opengl) {
				/* Convert to some compatible OpenGL texture format */
				Uint32 rmask=0, gmask=0, bmask=0, amask=0;
				int bpp=2;

				switch(surf->format->BitsPerPixel) {
					case 15:
						rmask = 31<<11;
						gmask = 31<<6;
						bmask = 31<<1;
						amask = 1;
						break;
					case 16:
						rmask = 31<<11;
						gmask = 63<<5;
						bmask = 31;
						break;
					case 24:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
						rmask = 255;
						gmask = 255<<8;
						bmask = 255<<16;
#else
						rmask = 255<<16;
						gmask = 255<<8;
						bmask = 255;
#endif
						bpp=3;
						break;
					case 32:
						rmask = 255;
						gmask = 255<<8;
						bmask = 255<<16;
						amask = 255<<24;
						bpp=4;
						break;
				}

				this->format.Rmask = tmpFmt.Rmask = rmask;
				this->format.Gmask = tmpFmt.Gmask = gmask;
				this->format.Bmask = tmpFmt.Bmask = bmask;
				this->format.Amask = tmpFmt.Amask = amask;
				this->format.BitsPerPixel = tmpFmt.BitsPerPixel = surf->format->BitsPerPixel;
				this->bpp = tmpFmt.BytesPerPixel = this->format.BytesPerPixel = bpp;
			}

			tmp_surf = SDL_ConvertSurface(surf, &tmpFmt, SDL_SWSURFACE);
			logMsg(2, "texture: converted to video format\n");

			this->resize(this, tmp_surf->w,tmp_surf->h);	/* recalc pitch */

			copy_pixels(this, tmp_surf);

			SDL_FreeSurface(tmp_surf);
		}
	}
}

static void copy_pixels(render_texture_t *this, SDL_Surface *surf)
{
	Uint8 *src, *dst;
	int y;

	src = (Uint8 *) surf->pixels;
	dst = (Uint8 *) this->pixels;

	for (y=0; y<this->h; y++) {
		memcpy(dst, src, this->w * this->bpp);

		src += surf->pitch;
		dst += this->pitch;
	}
}
