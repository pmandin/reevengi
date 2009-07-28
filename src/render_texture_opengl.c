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

static void shutdown(render_texture_t *texture);

static void upload(render_texture_t *texture, int num_pal);
static void download(render_texture_t *texture);

/*--- Functions ---*/

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

/* Load texture from a TIM image file as pointer */
render_texture_t *render_texture_gl_load_from_tim(void *tim_ptr)
{
	tim_header_t *tim_header;
	Uint16 *pal_header;
	int num_colors, num_palettes, i,j, paletted, img_offset;
	int w,h, wpot,hpot, tim_type;
	render_texture_t *tex;
	render_texture_gl_t *texgl;
	tim_size_t *tim_size;
	SDL_PixelFormat *fmt = video.screen->format;
	int bytes_per_pixel;

	/* Read dimensions */
	tim_header = (tim_header_t *) tim_ptr;
	if (SDL_SwapLE32(tim_header->magic) != MAGIC_TIM) {
		fprintf(stderr, "Not a TIM file\n");
		return NULL;
	}

	num_palettes = SDL_SwapLE16(tim_header->nb_palettes);
	if (num_palettes>MAX_TEX_PALETTE) {
		fprintf(stderr, "Does not support %d palettes per texture\n", num_palettes);
		return NULL;
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
		return NULL;
	}

	logMsg(2, "texture: %dx%d, %d palettes * %d colors\n", w,h, num_palettes, num_colors);

	/* Align on POT size */
	wpot = 2;
	while (wpot<w) {
		wpot <<= 1;
	}	
	hpot = 2;
	while (hpot<h) {
		hpot <<= 1;
	}

	/* Allocate memory */
	tex = calloc(1, sizeof(render_texture_gl_t) + wpot*hpot*bytes_per_pixel);
	if (!tex) {
		fprintf(stderr, "Can not allocate memory for texture\n");
		return NULL;
	}

	texgl = (render_texture_gl_t *) tex;
	for (i=0; i<MAX_TEX_PALETTE; i++) {
		texgl->texture_id[i] = 0xFFFFFFFFUL;
	}

	tex->paletted = paletted;
	tex->num_palettes = paletted ? num_palettes : 0;
	tex->pitch = wpot*bytes_per_pixel;
	tex->w = w;
	tex->pitchw = wpot;
	tex->h = h;
	tex->pitchh = hpot;
	tex->pixels = &((Uint8 *)tex)[sizeof(render_texture_gl_t)];

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
					tex->palettes[i][j] = (a<<24)|(r<<16)|(g<<8)|b;
				} else {
					tex->palettes[i][j] = SDL_MapRGBA(fmt, r,g,b,a);
				}
			}
		}
	}

	/* Copy data */
	switch(tim_type) {
		case TIM_TYPE_4:
			{
				Uint8 *src_pixels = &((Uint8 *) tim_ptr)[img_offset];
				Uint8 *tex_pixels = tex->pixels;
				for (i=0; i<h; i++) {
					Uint8 *tex_line = tex_pixels;
					for (j=0; j<w>>1; j++) {
						Uint8 color = *src_pixels++;
						*tex_line++ = color & 15;
						*tex_line++ = (color>>4) & 15;
					}
					tex_pixels += tex->pitch;
				}
			}
			break;
		case TIM_TYPE_8:
			{
				Uint8 *src_pixels = &((Uint8 *) tim_ptr)[img_offset];
				Uint8 *tex_pixels = tex->pixels;
				for (i=0; i<h; i++) {
					memcpy(tex_pixels, src_pixels, w);
					src_pixels += w;
					tex_pixels += tex->pitch;
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
							Uint16 *tex_pixels = (Uint16 *) tex->pixels;
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
								tex_pixels += tex->pitch>>1;
							}
						}
						break;
					case 3:
					case 4:
						{
							Uint32 *tex_pixels = (Uint32 *) tex->pixels;
							for (i=0; i<h; i++) {
								Uint32 *tex_line = tex_pixels;
								for (j=0; j<w; j++) {
									color = *src_pixels++;
									color = SDL_SwapLE16(color);

									read_rgba(color, &r,&g,&b,&a);
									
									*tex_line++ = SDL_MapRGBA(fmt, r,g,b,a);
								}
								tex_pixels += tex->pitch>>1;
							}
						}
						break;
				}
				
			}
			break;
	}

	tex->shutdown = shutdown;
	tex->upload = upload;
	tex->download = download;

	return tex;
}

static void shutdown(render_texture_t *texture)
{
	if (texture) {
		texture->download(texture);

		free((render_texture_gl_t *) texture);
	}
}

static void upload(render_texture_t *texture, int num_pal)
{
	render_texture_gl_t *texgl = (render_texture_gl_t *) texture;
	int i = texture->paletted ? num_pal : 0;
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
	if (texture->paletted) {
		surfaceFormat = GL_COLOR_INDEX;

#if defined(GL_EXT_paletted_texture)
		if (video.has_gl_ext_paletted_texture) {
			Uint8 mapP[256*4];
			Uint8 *pMap = mapP;

			internalFormat = GL_COLOR_INDEX8_EXT;
			for (i=0; i<256; i++) {
				Uint32 color = texture->palettes[num_pal][i];

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
				Uint32 color = texture->palettes[num_pal][i];

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
		texture->pitchw, texture->pitchh, 0,
		surfaceFormat, pixelType, texture->pixels
	);

	if (texture->paletted) {
		if (!video.has_gl_ext_paletted_texture) {
			gl.PixelTransferi(GL_MAP_COLOR, GL_FALSE);
		}
	}
}

static void download(render_texture_t *texture)
{
	int i;
	render_texture_gl_t *texgl = (render_texture_gl_t *) texture;

	for (i=0; i<MAX_TEX_PALETTE; i++) {
		if (texgl->texture_id[i] != 0xFFFFFFFFUL) {
			gl.DeleteTextures(1, &texgl->texture_id[i]);
			texgl->texture_id[i] = 0xFFFFFFFFUL;
		}
	}	
}

#endif /* ENABLE_OPENGL */
