/*
	3D skeleton, composed of meshes
	OpenGL renderer

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

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"

#include "render_texture.h"
#include "render_texture_opengl.h"
#include "render_mesh.h"
#include "render_skel_opengl.h"
#include "video.h"
#include "render.h"
#include "log.h"
#include "render_skel_list.h"

/*--- Functions prototypes ---*/

static void draw(render_skel_t *this, render_skel_mesh_t *parent);

/*--- Functions ---*/

render_skel_t *render_skel_gl_create(render_texture_t *texture)
{
	render_skel_t *skel;
	render_skel_gl_t *gl_skel;

	skel = render_skel_create(texture);
	if (!skel) {
		return NULL;
	}

	gl_skel = realloc(skel, sizeof(render_skel_gl_t));
	if (!gl_skel) {
		fprintf(stderr, "Can not allocate memory for skel\n");
		return NULL;
	}

	list_render_skel_remove(skel);

	skel = (render_skel_t *) gl_skel;

	gl_skel->softDraw = skel->draw;
	skel->draw = draw;

	logMsg(2, "render_skel_gl: created\n");

	list_render_skel_add((render_skel_t *) gl_skel);

	return skel;
}

static void draw(render_skel_t *this, render_skel_mesh_t *parent)
{
	render_skel_gl_t *gl_skel = (render_skel_gl_t *) this;

	if (parent == NULL) {
		/* Init OpenGL rendering */
		gl.Enable(GL_DEPTH_TEST);

		gl.Enable(GL_CULL_FACE);
		gl.CullFace(GL_FRONT);

		switch(render.render_mode) {
			case RENDER_WIREFRAME:
				gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;
			case RENDER_FILLED:
				gl.ShadeModel(GL_FLAT);
				break;
			case RENDER_GOURAUD:
				gl.ShadeModel(GL_SMOOTH);
				break;
			case RENDER_TEXTURED:
				{
					render_texture_gl_t *gl_tex = (render_texture_gl_t *) this->texture;

					gl.MatrixMode(GL_TEXTURE);
					gl.LoadIdentity();
					if (gl_tex->textureTarget == GL_TEXTURE_2D) {
						gl.Scalef(1.0f / this->texture->pitchw, 1.0f / this->texture->pitchh, 1.0f);
					}

					gl.MatrixMode(GL_MODELVIEW);

					gl.Enable(gl_tex->textureTarget);
				}
				break;
		}
	}

	gl_skel->softDraw(this, parent);

	if (parent == NULL) {
		switch(render.render_mode) {
			case RENDER_WIREFRAME:
				gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case RENDER_TEXTURED:
				{
					render_texture_gl_t *gl_tex = (render_texture_gl_t *) this->texture;

					gl.Disable(gl_tex->textureTarget);
				}
				break;
		}
		gl.Disable(GL_CULL_FACE);

		gl.Disable(GL_DEPTH_TEST);
	}
}

#endif /* ENABLE_OPENGL */