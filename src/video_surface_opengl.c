/*
	Video surface
	OpenGL backend

	Copyright (C) 2007	Patrice Mandin

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

#include <stdlib.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"
#include "video_surface.h"
#include "video_surface_opengl.h"

/*--- Functions prototypes ---*/

static void createTexture(video_surface_gl_t *this);
static void findTextureSize(video_surface_gl_t *this, int *width, int *height);
static void uploadTexture(video_surface_gl_t *this);

static void resize(video_surface_t *this, int w, int h);
static SDL_Surface *getSurface(video_surface_t *this);

/*--- Functions ---*/

video_surface_t *video_surface_gl_create(int w, int h, int bpp)
{
	video_surface_t *parent = video_surface_create(w,h,bpp);
	if (!parent) {
		return NULL;
	}

	video_surface_gl_t *this = (video_surface_gl_t *) calloc(1, sizeof(video_surface_gl_t));
	if (!this) {
		video_surface_destroy(parent);
		return NULL;
	}

	memcpy(&this->surf_soft, parent, sizeof(video_surface_t));
	free(parent);

	createTexture(this);

	this->surf_soft.getSurface = getSurface;
	/*printf("ogl_surf: create from size\n");*/
	return (video_surface_t *) this;
}

video_surface_t *video_surface_gl_create_pf(int w, int h, SDL_PixelFormat *pixelFormat)
{
	video_surface_t *parent = video_surface_create_pf(w,h,pixelFormat);
	if (!parent) {
		return NULL;
	}

	video_surface_gl_t *this = (video_surface_gl_t *) calloc(1, sizeof(video_surface_gl_t));
	if (!this) {
		video_surface_destroy(parent);
		return NULL;
	}

	memcpy(&this->surf_soft, parent, sizeof(video_surface_t));
	free(parent);

	createTexture(this);

	this->surf_soft.getSurface = getSurface;
	/*printf("ogl_surf: create from format\n");*/
	return (video_surface_t *) this;
}

video_surface_t *video_surface_gl_create_su(SDL_Surface *surface)
{
	video_surface_t *parent = video_surface_create_su(surface);
	if (!parent) {
		return NULL;
	}

	video_surface_gl_t *this = (video_surface_gl_t *) calloc(1, sizeof(video_surface_gl_t));
	if (!this) {
		video_surface_destroy(parent);
		return NULL;
	}

	memcpy(&this->surf_soft, parent, sizeof(video_surface_t));
	free(parent);

	createTexture(this);

	SDL_BlitSurface(surface, NULL, this->surf_soft.sdl_surf, NULL);

	this->surf_soft.getSurface = getSurface;
	/*printf("ogl_surf: create from surface\n");*/
	return (video_surface_t *) this;
}

void video_surface_gl_destroy(video_surface_t *this)
{
	video_surface_gl_t *this_gl = (video_surface_gl_t *) this;

	if (this_gl) {
		gl.DeleteTextures(1, &this_gl->textureObject);

		/* parent will do free() */
		video_surface_destroy(this);
	}
}

/*--- Private functions ---*/

static void createTexture(video_surface_gl_t *this)
{
	int sw = this->surf_soft.sdl_surf->w;
	int sh = this->surf_soft.sdl_surf->h;
	char *extensions;

	this->can_palette = this->use_palette = 0;

	resize(&this->surf_soft, sw, sh);

	gl.GenTextures(1, &this->textureObject);

#if defined(GL_EXT_paletted_texture)
	extensions = (char *) gl.GetString(GL_EXTENSIONS);

	if (strstr(extensions, "GL_EXT_paletted_texture")
	  && (this->surf_soft.sdl_surf->format->BitsPerPixel == 8)
	  && this->can_palette)
	{
		this->use_palette = 1;
	}
#endif
}

/* Find right size for texture, given extensions */

static void findTextureSize(video_surface_gl_t *this, int *width, int *height)
{
	int w = *width, h = *height;
	char *extensions;

	/* Minimal size */
	if (w<64) {
		w = 64;
	}
	if (h<64) {
		h = 64;
	}

	/* Align on 16 pixels boundary */
	if (w & 15) {
		w = (w | 15)+1;
	}
	if (h & 15) {
		h = (h | 15)+1;
	}

	extensions = (char *) gl.GetString(GL_EXTENSIONS);

	if (strstr(extensions, "GL_ARB_texture_non_power_of_two")) {
		this->textureTarget = GL_TEXTURE_2D;
		this->can_palette = 1;
	}
#if defined(GL_ARB_texture_rectangle)
	else if (strstr(extensions, "GL_ARB_texture_rectangle")) {
		this->textureTarget = GL_TEXTURE_RECTANGLE_ARB;
		this->can_palette = 0;
	}
#endif
#if defined(GL_EXT_texture_rectangle)
	else if (strstr(extensions, "GL_EXT_texture_rectangle")) {
		this->textureTarget = GL_TEXTURE_RECTANGLE_EXT;
		this->can_palette = 0;
	}
#endif
#if defined(GL_NV_texture_rectangle)
	else if (strstr(extensions, "GL_NV_texture_rectangle")) {
		this->textureTarget = GL_TEXTURE_RECTANGLE_NV;
		this->can_palette = 0;
	}
#endif
	else {
		/* Calc smallest power of two size needed */
		int w1=64, h1=64;
		while (w>w1) {
			w1<<=1;
		}
		while (h>h1) {
			h1<<=1;
		}
		w = w1;
		h = h1;

		this->textureTarget = GL_TEXTURE_2D;
		this->can_palette = 1;
	}

	/* FIXME: what to do if hw do not support asked size ? */
	/*printf("ogl_surf: needed %dx%d, got %dx%d\n", *width, *height, w,h);*/

	*width = w;
	*height = h;
}

/* Upload texture data */

static void uploadTexture(video_surface_gl_t *this)
{
	GLfloat mapR[256], mapG[256], mapB[256], mapA[256];
	GLenum internalFormat = GL_RGBA;
	GLenum pixelType = GL_UNSIGNED_INT;
	SDL_Surface *surface = this->surf_soft.sdl_surf;

	/*printf("ogl_surf: upload texture, %d %d\n",
		this->textureTarget, this->textureObject);*/

	gl.BindTexture(this->textureTarget, this->textureObject);

	/*printf("ogl_surf: %dx%d, %d\n", surface->w, surface->h,
		surface->format->BitsPerPixel);*/

	switch (surface->format->BitsPerPixel) {
		case 8:
			{
				SDL_Color *palette = surface->format->palette->colors;
				int i;

#ifdef GL_EXT_paletted_texture
				if (this->use_palette) {
					Uint8 mapP[256*3];
					Uint8 *pMap = mapP;

					internalFormat = GL_COLOR_INDEX8_EXT;
					for (i=0;i<surface->format->palette->ncolors;i++) {
						*pMap++ = palette[i].r;
						*pMap++ = palette[i].g;
						*pMap++ = palette[i].b;
					}
					gl.ColorTableEXT(this->textureTarget, GL_RGB, 256, 
						GL_RGB, GL_UNSIGNED_BYTE, mapP);
				} else
#endif
				{
					memset(mapR, 0, sizeof(mapR));
					memset(mapG, 0, sizeof(mapG));
					memset(mapB, 0, sizeof(mapB));
					memset(mapA, 0, sizeof(mapA));
					for (i=0;i<surface->format->palette->ncolors;i++) {
						mapR[i] = palette[i].r / 255.0;
						mapG[i] = palette[i].g / 255.0;
						mapB[i] = palette[i].b / 255.0;
						mapA[i] = 1.0;
					}
					gl.PixelTransferi(GL_MAP_COLOR, GL_TRUE);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, mapR);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, mapG);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, mapB);
					gl.PixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, mapA);
				}
			}
			break;
		case 16:
			pixelType = GL_UNSIGNED_SHORT_5_6_5;
			if (surface->format->Rmask == 31) {
				pixelType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			}
			/* FIXME: care about endianness ? */
			break;
		case 24:
			/* FIXME: care about endianness ? */
			break;
		case 32:
			/* FIXME: care about endianness ? */
			break;
	}

	gl.TexImage2D(this->textureTarget,0, internalFormat,
		surface->w, surface->h, 0,
		GL_RGBA, pixelType, surface->pixels
	);

	switch (surface->format->BitsPerPixel) {
		case 8:
			if (!this->use_palette) {
				gl.PixelTransferi(GL_MAP_COLOR, GL_FALSE);
			}
			break;
		case 16:
			/* FIXME: care about endianness ? */
			break;
		case 24:
			/* FIXME: care about endianness ? */
			break;
		case 32:
			/* FIXME: care about endianness ? */
			break;
	}
}

/* Resize a surface and texture */

static void resize(video_surface_t *this, int w, int h)
{
	video_surface_gl_t *gl_this = (video_surface_gl_t *) this;

	/*printf("ogl_surf: resize to %dx%d\n", w,h);*/
	findTextureSize(gl_this, &w, &h);
	this->resize(this, w, h);
	gl_this->need_upload = 1;
}

static SDL_Surface *getSurface(video_surface_t *this)
{
	video_surface_gl_t *gl_this = (video_surface_gl_t *) this;

	if (gl_this->need_upload) {
		uploadTexture(gl_this);
		gl_this->need_upload = 0;
	}
	return gl_this->surf_soft.sdl_surf;
}

#endif /* ENABLE_OPENGL */
