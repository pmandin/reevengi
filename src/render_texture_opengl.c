/*
	Textures for 3D objects
	OpenGL backend

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_OPENGL

#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"

#include "background_tim.h"
#include "render_texture.h"
#include "render_texture_opengl.h"
#include "video.h"
#include "dither.h"
#include "parameters.h"
#include "log.h"

/*--- Functions prototypes ---*/

static void shutdown(render_texture_t *this);

static void upload(render_texture_t *this, int num_pal);
static void download(render_texture_t *this);

static void resize(render_texture_t *this, int w, int h);
static void load_from_tim(render_texture_t *this, void *tim_ptr);

static void read_rgba(Uint16 color, int *r, int *g, int *b, int *a);
static int logbase2(int n);

/*--- Functions ---*/

render_texture_t *render_texture_gl_create(int must_pot)
{
	render_texture_gl_t *gl_tex;
	render_texture_t *tex;
	int i;

	gl_tex = calloc(1, sizeof(render_texture_gl_t));
	if (!gl_tex) {
		fprintf(stderr, "Can not allocate memory for render_texture\n");
		return NULL;
	}

	tex = (render_texture_t *) gl_tex;

	tex->shutdown = shutdown;
	tex->upload = upload;
	tex->download = download;
	tex->resize = resize;
	tex->load_from_tim = load_from_tim;

	tex->must_pot = must_pot;

	tex->bpp = video.screen->format->BytesPerPixel;

	for (i=0; i<MAX_TEX_PALETTE; i++) {
		gl_tex->texture_id[i] = 0xffffffffUL;
	}

	list_render_texture_add(tex);

	return tex;
}

static void shutdown(render_texture_t *this)
{
	if (this) {
		if (this->scaled) {
			this->scaled->shutdown(this->scaled);
			this->scaled = NULL;
		}
		list_render_texture_remove(this);
		free(this);
	}
}

static void upload(render_texture_t *this, int num_pal)
{
	render_texture_gl_t *texgl = (render_texture_gl_t *) this;
	int i = this->paletted ? num_pal : 0;
	GLenum internalFormat = GL_RGBA;
	GLenum pixelType = GL_UNSIGNED_BYTE;
	GLenum surfaceFormat = GL_RGBA;

	/* Already uploaded ? */
	if (texgl->texture_id[i] != 0xFFFFFFFFUL) {
		gl.BindTexture(GL_TEXTURE_2D, texgl->texture_id[i]);
		return;
	}

	/* Create new texture object, and upload texture */
	gl.GenTextures(1, &texgl->texture_id[i]);

	gl.BindTexture(GL_TEXTURE_2D, texgl->texture_id[i]);

	/* Upload new palette */
	if (this->paletted) {
		surfaceFormat = GL_COLOR_INDEX;

#if defined(GL_EXT_paletted_texture)
		if (video.has_gl_ext_paletted_texture) {
			Uint8 mapP[256*4];
			Uint8 *pMap = mapP;

			internalFormat = GL_COLOR_INDEX8_EXT;
			for (i=0; i<256; i++) {
				Uint32 color = this->palettes[num_pal][i];

				*pMap++ = (color>>16) & 0xff;
				*pMap++ = (color>>8) & 0xff;
				*pMap++ = color & 0xff;
				*pMap++ = (color>>24) & 0xff;
			}
			gl.ColorTableEXT(GL_TEXTURE_2D, GL_RGBA, 256, 
				GL_RGBA, GL_UNSIGNED_BYTE, mapP);
		} else
#endif
		{
			GLfloat mapR[256], mapG[256], mapB[256], mapA[256];

			memset(mapR, 0, sizeof(mapR));
			memset(mapG, 0, sizeof(mapG));
			memset(mapB, 0, sizeof(mapB));
			memset(mapA, 0, sizeof(mapA));
			for (i=0; i<256; i++) {
				Uint32 color = this->palettes[num_pal][i];

				mapR[i] = ((color>>16) & 0xff) / 255.0;
				mapG[i] = ((color>>8) & 0xff) / 255.0;
				mapB[i] = (color & 0xff) / 255.0;
				mapA[i] = ((color>>24) & 0xff) / 255.0;
			}
			gl.PixelTransferi(GL_MAP_COLOR, GL_TRUE);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, mapR);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, mapG);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, mapB);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, mapA);
		}
	} else {
		pixelType = GL_UNSIGNED_SHORT_5_5_5_1;
	}

	gl.TexImage2D(GL_TEXTURE_2D,0, internalFormat,
		this->pitchw, this->pitchh, 0,
		surfaceFormat, pixelType, this->pixels
	);

	if (this->paletted) {
		if (!video.has_gl_ext_paletted_texture) {
			gl.PixelTransferi(GL_MAP_COLOR, GL_FALSE);
		}
	}
}

static void download(render_texture_t *this)
{
	int i;
	render_texture_gl_t *texgl = (render_texture_gl_t *) this;

	for (i=0; i<MAX_TEX_PALETTE; i++) {
		if (texgl->texture_id[i] != 0xFFFFFFFFUL) {
			gl.DeleteTextures(1, &texgl->texture_id[i]);
			texgl->texture_id[i] = 0xFFFFFFFFUL;
		}
	}	
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

	/* Calc new bounding size */
	new_bound_w = w;
	new_bound_h = h;
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

		for (y=0; y<h; y++) {
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
	int w,h, wpot,hpot, tim_type;
	render_texture_gl_t *texgl;
	tim_size_t *tim_size;
	SDL_PixelFormat *fmt = video.screen->format;
	int bytes_per_pixel;

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

	/* Copy palettes to video format */
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
			}
		}
	}

	if ((tim_type == TIM_TYPE_16) && params.use_opengl) {
		this->bpp = 2;
	}
	this->resize(this, w,h);

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
					memcpy(tex_pixels, src_pixels, w);
					src_pixels += w;
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

	*r = r1;
	*g = g1;
	*b = b1;
	*a = a1;
}

#endif /* ENABLE_OPENGL */
