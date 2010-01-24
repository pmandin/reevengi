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

#include "dyngl.h"

#include "video.h"
#include "render.h"
#include "render_texture_opengl.h"

/*--- Functions prototypes ---*/

static void bitmapSetSrcPos(video_t *video, int srcx, int srcy);
static void bitmapUnscaled(video_t *video, int x, int y);
static void bitmapScaled(video_t *video, int x, int y, int w, int h);

/*--- Functions ---*/

void render_bitmap_opengl_init(render_t *render)
{
	render->bitmapUnscaled = bitmapUnscaled;
	render->bitmapScaled = bitmapScaled;
	render->bitmapSetSrcPos = bitmapSetSrcPos;
}

static void bitmapSetSrcPos(video_t *video, int srcx, int srcy)
{
	render.bitmapSrcX = srcx;
	render.bitmapSrcY = srcy;
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
	render_texture_t *tex = render.texture;
	render_texture_gl_t *gl_tex;

	if (!tex)
		return;

	gl_tex = (render_texture_gl_t *) tex;

	gl.Enable(gl_tex->textureTarget);
	gl.Disable(GL_DEPTH_TEST);

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0f, video->width, video->height, 0.0f, -1.0f, 1.0f);

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	if (gl_tex->textureTarget != GL_TEXTURE_2D) {
		/* Rescale to width/height range */
		gl.Translatef((float) render.bitmapSrcX,
			(float) render.bitmapSrcY, 0.0f);
		gl.Scalef((float) tex->w - render.bitmapSrcX,
			(float) tex->h - render.bitmapSrcY, 1.0f);
	} else {
		/* Rescale to 0-1 range */
		gl.Translatef((float) render.bitmapSrcX / tex->pitchw,
			(float) render.bitmapSrcY / tex->pitchh, 0.0f);
		gl.Scalef((float) (tex->w-render.bitmapSrcX) / tex->pitchw,
			(float) (tex->h-render.bitmapSrcY) / tex->pitchh, 1.0f);
	}

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	/*gl.Translatef(0.375f, 0.375f, 0.0f);*/
	gl.Scalef((float) video->width, (float) video->height, 1.0f);

	gl.Begin(GL_QUADS);
		gl.TexCoord2f(0.0f, 0.0f);
		gl.Vertex2f(0.0f, 0.0f);

		gl.TexCoord2f(1.0f, 0.0f);
		gl.Vertex2f((float) (tex->pitchw - render.bitmapSrcX) / tex->pitchw, 0.0f);

		gl.TexCoord2f(1.0f, 1.0f);
		gl.Vertex2f((float) (tex->pitchw - render.bitmapSrcX) / tex->pitchw,
			(float) (tex->pitchh - render.bitmapSrcY) / tex->pitchh);

		gl.TexCoord2f(0.0f, 1.0f);
		gl.Vertex2f(0.0f, (float) (tex->pitchh - render.bitmapSrcY) / tex->pitchh);
	gl.End();

	gl.Disable(gl_tex->textureTarget);
	gl.Enable(GL_DEPTH_TEST);
}

#endif /* ENABLE_OPENGL */
