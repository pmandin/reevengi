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

#include "background_tim.h"
#include "render_texture.h"
#include "video.h"
#include "render.h"
#include "r_soft/dither.h"
#include "parameters.h"
#include "log.h"
#include "render_texture_list.h"

/*--- Defines ---*/

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/*--- Functions prototypes ---*/

static void shutdown(render_texture_t *this);

static void upload(render_texture_t *this, int num_pal);
static void download(render_texture_t *this);

static void prepare_resize(render_texture_t *this, int *w, int *h);
static void resize(render_texture_t *this, int w, int h);

static void load_from_tim(render_texture_t *this, void *tim_ptr);
static void load_from_surf(render_texture_t *this, SDL_Surface *surf);

static void read_rgba(Uint16 color, int *r, int *g, int *b, int *a);
static int logbase2(int n);

static void mark_trans(render_texture_t *this, int num_pal, int x1,int y1, int x2,int y2);

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
	tim_header_t *tim_header;
	Uint16 *pal_header;
	int num_colors, num_palettes, i,j, paletted, img_offset;
	int w,h, tim_type;
	tim_size_t *tim_size;
	SDL_PixelFormat *fmt = video.screen->format /*&(this->format)*/;
	int bytes_per_pixel;

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
	bytes_per_pixel = 1;
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
			bytes_per_pixel=2;
			if (fmt->BytesPerPixel>2) {
				bytes_per_pixel=4;
			}
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
}

static void load_from_surf(render_texture_t *this, SDL_Surface *surf)
{
	SDL_Surface *tmp_surf = NULL;
	int free_tmp_surf = 1, y;

	if (!this || !surf) {
		return;
	}

	/* Init palette */
	this->num_palettes = this->paletted = 0;

	if ((surf->format->BitsPerPixel==8) && surf->format->palette) {
		int i;
		SDL_Palette *surf_palette = surf->format->palette;

		this->num_palettes = this->paletted = 1;

		for (i=0; i<surf->format->palette->ncolors; i++) {
			int r,g,b,a;

			r = surf_palette->colors[i].r;
			g = surf_palette->colors[i].g;
			b = surf_palette->colors[i].b;
			a = 0xff;

			if (params.use_opengl) {
				this->palettes[0][i] = (a<<24)|(r<<16)|(g<<8)|b;
			} else {
				this->palettes[0][i] = SDL_MapRGBA(surf->format, r,g,b,a);
			}
			this->alpha_palettes[0][i] = a;
		}
	}

	/* Copy data */
	if (this->cacheable) {
		tmp_surf = surf;
		free_tmp_surf = 0;
		logMsg(2, "texture: keep texture in original format\n");
	} else {
		if (video.bpp == 8) {
			tmp_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, surf->w,surf->h,8, 0,0,0,0);
			if (tmp_surf) {
				dither_setpalette(tmp_surf);
				dither_copy(surf, tmp_surf);
			}
			logMsg(2, "texture: converted to 8bits dither palette\n");
		} else {
			if (params.use_opengl) {
				/* Convert to some compatible OpenGL texture format */
				Uint32 rmask=0, gmask=0, bmask=0, amask=0;

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
						break;
					case 32:
						rmask = 255;
						gmask = 255<<8;
						bmask = 255<<16;
						amask = 255<<24;
						break;
				}

				tmp_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, surf->w,surf->h,surf->format->BitsPerPixel,
					rmask,gmask,bmask,amask);
				if (tmp_surf) {
					SDL_BlitSurface(surf,NULL,tmp_surf,NULL);
				}
				logMsg(2, "texture: convert to OpenGL texture format\n");
			} else {
				/* Convert to video format */
				tmp_surf = SDL_DisplayFormat(surf);
				logMsg(2, "texture: convert to screen format\n");
			}
		}
	}

	if (!tmp_surf) {
		fprintf(stderr, "texture: no data uploaded\n");
		return;
	}

	this->bpp = tmp_surf->format->BytesPerPixel;
	memcpy(&(this->format), tmp_surf->format, sizeof(SDL_PixelFormat));
	this->format.palette = NULL;

	this->resize(this, tmp_surf->w,tmp_surf->h);

	logMsg(2, "texture: %dx%d, %d bpp, %d palettes\n",
		this->w,this->h, tmp_surf->format->BitsPerPixel, this->num_palettes);

	logMsg(2, "texture: R=0x%08x, G=0x%08x, B=0x%08x, A=0x%08x\n",
		this->format.Rmask, this->format.Gmask,
		this->format.Bmask, this->format.Amask);

	switch(this->bpp) {
		case 1:
			{
				Uint8 *src = tmp_surf->pixels;
				Uint8 *dst = this->pixels;
				for (y=0; y<this->h; y++) {
					memcpy(dst, src, this->w);
					src += tmp_surf->pitch;
					dst += this->pitch;
				}
			}
			break;
		case 2:
			{
				Uint16 *src = (Uint16 *) tmp_surf->pixels;
				Uint16 *dst = (Uint16 *) this->pixels;
				for (y=0; y<this->h; y++) {
					memcpy(dst, src, this->w<<1);
					src += tmp_surf->pitch>>1;
					dst += this->pitch>>1;
				}
			}
			break;
		case 3:
			{
				Uint8 *src = tmp_surf->pixels;
				Uint8 *dst = this->pixels;
				for (y=0; y<this->h; y++) {
					memcpy(dst, src, this->w *3);
					src += tmp_surf->pitch;
					dst += this->pitch;
				}
			}
			break;
		case 4:
			{
				Uint32 *src = (Uint32 *) tmp_surf->pixels;
				Uint32 *dst = (Uint32 *) this->pixels;
				for (y=0; y<this->h; y++) {
					memcpy(dst, src, this->w<<2);
					src += tmp_surf->pitch>>2;
					dst += this->pitch>>2;
				}
			}
			break;
	}

	if (free_tmp_surf) {
		SDL_FreeSurface(tmp_surf);	
	}

	this->download(this);
}

static int logbase2(int n)
{
	int r = 0;

	while (n>>=1) {
		++r;
	}

	return r;
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

	/*if (!a1) {
		r1 = b1 = 0xff;
		g1 = 0;
	}*/

	*r = r1;
	*g = g1;
	*b = b1;
	*a = a1;
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
