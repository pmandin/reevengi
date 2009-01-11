/*
	Rescale and display background

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

#include "video_surface_opengl.h"
#include "video.h"
#include "render_background_opengl.h"

/*--- Variables ---*/

static video_surface_t *backgroundSurf = NULL;

/*--- Functions ---*/

void render_background_init_opengl(video_t *this, video_surface_t *source)
{
	video_surface_gl_t *gl_surf = (video_surface_gl_t *) backgroundSurf;

	backgroundSurf = source;

	/* Force reupload texture (needed on Win32), for a screen resize */
	if (gl_surf) {
		gl_surf->need_upload = 1;
	}
}

void render_background_opengl(video_t *this)
{
	video_surface_gl_t *gl_surf = (video_surface_gl_t *) backgroundSurf;
	GLenum textureTarget, textureObject;
	SDL_Surface *sdl_surf;

	if (!this || !backgroundSurf) {
		return;
	}

	sdl_surf = backgroundSurf->getSurface(backgroundSurf);	/* Update texture from sdl surface */

	textureTarget = gl_surf->textureTarget;
	textureObject = gl_surf->textureObject;
	
	gl.Enable(textureTarget);
	gl.BindTexture(textureTarget, textureObject);

 	gl.TexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);

 	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	/*gl.Enable(GL_DITHER);*/

	/*gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE, GL_ZERO);*/

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0f, this->width, this->height, 0.0f, -1.0f, 1.0f);

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.Scalef(backgroundSurf->width, backgroundSurf->height, 1.0f);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	/*gl.Translatef(0.375f, 0.375f, 0.0f);*/
	gl.Scalef(this->width, this->height, 1.0f);

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

	/*gl.Disable(GL_DITHER);
	gl.Disable(GL_BLEND);*/
	gl.Disable(GL_TEXTURE_RECTANGLE_ARB);
}

#endif /* ENABLE_OPENGL */
