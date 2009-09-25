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

static void upload(render_texture_t *this, int num_pal);
static void download(render_texture_t *this);

static void prepare_resize(render_texture_t *this, int *w, int *h);

static int logbase2(int n);

/*--- Functions ---*/

render_texture_t *render_texture_gl_create(int flags)
{
	render_texture_gl_t *gl_tex;
	render_texture_t *tex;
	int i;

	tex = render_texture_create(flags);
	if (!tex) {
		return NULL;
	}

	gl_tex = realloc(tex, sizeof(render_texture_gl_t));
	if (!gl_tex) {
		fprintf(stderr, "Can not allocate memory for render_texture\n");
		return NULL;
	}

	tex = (render_texture_t *) gl_tex;

	tex->upload = upload;
	tex->download = download;
	tex->prepare_resize = prepare_resize;

	tex->must_pot = tex->cacheable = 0;

	gl_tex->textureTarget = GL_TEXTURE_2D;
	for (i=0; i<MAX_TEX_PALETTE; i++) {
		gl_tex->texture_id[i] = 0xffffffffUL;
	}

	list_render_texture_remove(tex);
	list_render_texture_add((render_texture_t *) gl_tex);

	return tex;
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
		gl.BindTexture(texgl->textureTarget, texgl->texture_id[i]);
		return;
	}

	/* Create new texture object, and upload texture */
	gl.GenTextures(1, &texgl->texture_id[i]);

	gl.BindTexture(texgl->textureTarget, texgl->texture_id[i]);

	/* Upload new palette */
	switch (this->bpp) {
		case 1:
			surfaceFormat = GL_COLOR_INDEX;

#if defined(GL_EXT_paletted_texture)
			if (video.has_gl_ext_paletted_texture && (texgl->textureTarget==GL_TEXTURE_2D)) {
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
				gl.ColorTableEXT(texgl->textureTarget, GL_RGBA, 256, 
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

					mapR[i] = ((color>>16) & 0xff) / 255.0f;
					mapG[i] = ((color>>8) & 0xff) / 255.0f;
					mapB[i] = (color & 0xff) / 255.0f;
					mapA[i] = ((color>>24) & 0xff) / 255.0f;
				}
				gl.PixelTransferi(GL_MAP_COLOR, GL_TRUE);
				gl.PixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, mapR);
				gl.PixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, mapG);
				gl.PixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, mapB);
				gl.PixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, mapA);
			}
			break;
		case 2:
			pixelType = GL_UNSIGNED_SHORT_5_5_5_1;
			break;
		case 3:
			surfaceFormat = GL_RGB;
			break;
		case 4:
			break;
	}

	gl.TexImage2D(texgl->textureTarget,0, internalFormat,
		this->pitchw, this->pitchh, 0,
		surfaceFormat, pixelType, this->pixels
	);

	if (this->bpp == 1) {
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

static void prepare_resize(render_texture_t *this, int *w, int *h)
{
	int new_bound_w = *w;
	int new_bound_h = *h;
	int potw = logbase2(new_bound_w);
	int poth = logbase2(new_bound_h);
	int must_pot = 0;
	render_texture_gl_t *gl_this = (render_texture_gl_t *) this;

	if ((new_bound_w == (1<<potw)) && (new_bound_h == (1<<poth))) {
		gl_this->textureTarget = GL_TEXTURE_2D;
		logMsg(2, "texture: pot\n");
		return;
	}

	if (video.has_gl_arb_texture_non_power_of_two) {
		gl_this->textureTarget = GL_TEXTURE_2D;
		logMsg(2, "texture: arb_npot\n");
	}
#if defined(GL_ARB_texture_rectangle)
	else if (video.has_gl_arb_texture_rectangle) {
		gl_this->textureTarget = GL_TEXTURE_RECTANGLE_ARB;
		/*this->can_palette = 0;*/
		logMsg(2, "texture: arb_rect\n");
	}
#endif
#if defined(GL_EXT_texture_rectangle)
	else if (video.has_gl_ext_texture_rectangle) {
		gl_this->textureTarget = GL_TEXTURE_RECTANGLE_EXT;
		/*this->can_palette = 0;*/
		logMsg(2, "texture: ext_rect\n");
	}
#endif
#if defined(GL_NV_texture_rectangle)
	else if (video.has_gl_nv_texture_rectangle) {
		gl_this->textureTarget = GL_TEXTURE_RECTANGLE_NV;
		/*this->can_palette = 0;*/
		logMsg(2, "texture: nv_rect\n");
	}
#endif
	else {
		must_pot = 1;
		logMsg(2, "texture: must pot\n");
	}

	if (must_pot) {
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

static int logbase2(int n)
{
	int r = 0;

	while (n>>=1) {
		++r;
	}

	return r;
}

#endif /* ENABLE_OPENGL */
