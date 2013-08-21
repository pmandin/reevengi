/*
	Draw 2D bitmaps (background, font, etc)
	OpenGL backend

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_OPENGL

#include <SDL.h>
#include <SDL_opengl.h>

#include "../video.h"
#include "../render.h"
#include "../r_common/render_bitmap.h"

#include "dyngl.h"
#include "render_texture_opengl.h"

/*--- Functions prototypes ---*/

static void drawImage(void);

/*--- Functions ---*/

void render_bitmap_opengl_init(render_bitmap_t *render_bitmap)
{
	render_bitmap_init(render_bitmap);

	render.bitmap.drawImage = drawImage;
}

static void drawImage(void)
{
	render_texture_t *tex = render.texture;
	render_texture_gl_t *gl_tex;
	float bitmap_depth;

	if (!tex)
		return;

	gl_tex = (render_texture_gl_t *) tex;

	/* Clip in source texture */
	if (render.bitmap.srcRect.x<0) {
		render.bitmap.srcRect.w += render.bitmap.srcRect.x;
		render.bitmap.dstRect.x -= (render.bitmap.srcRect.x*render.bitmap.dstWidth)/render.bitmap.srcWidth;
		render.bitmap.srcRect.x = 0;
	} else if (render.bitmap.srcRect.x+render.bitmap.srcRect.w>tex->w) {
		render.bitmap.srcRect.w = render.texture->w - render.bitmap.srcRect.x;
		render.bitmap.dstRect.w = (render.bitmap.srcRect.w*render.bitmap.dstWidth)/render.bitmap.srcWidth;
	}
	if (render.bitmap.srcRect.y<0) {
		render.bitmap.srcRect.h += render.bitmap.srcRect.y;
		render.bitmap.dstRect.y -= (render.bitmap.srcRect.y*render.bitmap.dstHeight)/render.bitmap.srcHeight;
		render.bitmap.srcRect.y = 0;
	} else if (render.bitmap.srcRect.y+render.bitmap.srcRect.h>tex->h) {
		render.bitmap.srcRect.h = render.texture->h - render.bitmap.srcRect.y;
		render.bitmap.dstRect.h = (render.bitmap.srcRect.h*render.bitmap.dstHeight)/render.bitmap.srcHeight;
	}

	/* Clip in dest screen */
	if (render.bitmap.dstRect.x<video.viewport.x) {
		render.bitmap.dstRect.w += render.bitmap.dstRect.x;
		render.bitmap.srcRect.x -= (render.bitmap.dstRect.x*render.bitmap.srcWidth)/render.bitmap.dstWidth;
		render.bitmap.dstRect.x = 0;
	} else if (render.bitmap.dstRect.x+render.bitmap.dstRect.w>video.viewport.x+video.viewport.w) {
		render.bitmap.dstRect.w = video.viewport.w - render.bitmap.dstRect.x;
		render.bitmap.srcRect.w = (render.bitmap.dstRect.w*render.bitmap.srcWidth)/render.bitmap.dstWidth;
	}
	if (render.bitmap.dstRect.y<video.viewport.y) {
		render.bitmap.dstRect.h += render.bitmap.dstRect.y;
		render.bitmap.srcRect.y -= (render.bitmap.dstRect.y*render.bitmap.srcHeight)/render.bitmap.dstHeight;
		render.bitmap.dstRect.y = 0;
	} else if (render.bitmap.dstRect.y+render.bitmap.dstRect.h>video.viewport.y+video.viewport.h) {
		render.bitmap.dstRect.h = video.viewport.h - render.bitmap.dstRect.y;
		render.bitmap.srcRect.h = (render.bitmap.dstRect.h*render.bitmap.srcHeight)/render.bitmap.dstHeight;
	}

	/* Clipping for out of bounds in source */
	if ((render.bitmap.srcRect.x>=tex->w)
	   || (render.bitmap.srcRect.y>=tex->h)
	   || (render.bitmap.srcRect.x+render.bitmap.srcRect.w<0)
	   || (render.bitmap.srcRect.y+render.bitmap.srcRect.h<0))
	{
		return;
	}

	/* Clipping for out of bounds in dest */
	if ((render.bitmap.dstRect.x>=video.viewport.x+video.viewport.w)
	   || (render.bitmap.dstRect.y>=video.viewport.y+video.viewport.h)
	   || (render.bitmap.dstRect.x+render.bitmap.dstRect.w<video.viewport.x)
	   || (render.bitmap.dstRect.y+render.bitmap.dstRect.h<video.viewport.y))
	{
		return;
	}

	render.bitmap.dstRect.x -= video.viewport.x;
	render.bitmap.dstRect.y -= video.viewport.y;

	gl.Enable(gl_tex->textureTarget);
	if (render.bitmap.depth_test) {
		gl.Enable(GL_DEPTH_TEST);
/*
f/(f-n) * 1-n/z
*/
		bitmap_depth = 1.0f - (RENDER_Z_NEAR / render.bitmap.depth);
		bitmap_depth *= RENDER_Z_FAR / (RENDER_Z_FAR - RENDER_Z_NEAR);

		if (render.bitmap.masking) {
			gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}
	} else {
		gl.Disable(GL_DEPTH_TEST);
		bitmap_depth = 0.5f;
	}

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0f, video.viewport.w, video.viewport.h, 0.0f, 0.0f, 1.0f);

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	if (gl_tex->textureTarget != GL_TEXTURE_2D) {
		/* Rescale to width/height range */
		gl.Translatef((float) render.bitmap.srcRect.x,
			(float) render.bitmap.srcRect.y, 0.0f);
		gl.Scalef((float) render.bitmap.srcRect.w,
			(float) render.bitmap.srcRect.h, 1.0f);
	} else {
		/* Rescale to 0-1 range */
		gl.Translatef((float) render.bitmap.srcRect.x / tex->pitchw,
			(float) render.bitmap.srcRect.y / tex->pitchh, 0.0f);
		gl.Scalef((float) render.bitmap.srcRect.w / tex->pitchw,
			(float) render.bitmap.srcRect.h / tex->pitchh, 1.0f);
	}

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Translatef((float) render.bitmap.dstRect.x,
		(float) render.bitmap.dstRect.y,
		-bitmap_depth);
	gl.Scalef((float) render.bitmap.dstRect.w,
		(float) render.bitmap.dstRect.h,
		1.0f);

	gl.Begin(GL_QUADS);
		gl.TexCoord2f(0.0f, 0.0f);
		gl.Vertex2f(0.0f, 0.0f);

		gl.TexCoord2f(1.0f, 0.0f);
		gl.Vertex2f(1.0f, 0.0f);

		gl.TexCoord2f(1.0f, 1.0f);
		gl.Vertex2f(1.0f, 1.0f);

		gl.TexCoord2f(0.0f, 1.0f);
		gl.Vertex2f(0.0f, 1.0f);
	gl.End();

	gl.Disable(gl_tex->textureTarget);
	gl.Enable(GL_DEPTH_TEST);
	gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

#endif /* ENABLE_OPENGL */
