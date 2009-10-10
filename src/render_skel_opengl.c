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
#include "render_mesh.h"
#include "render_skel_opengl.h"
#include "video.h"
#include "render.h"
#include "log.h"
#include "render_skel_list.h"

/*--- Functions prototypes ---*/

static void draw(render_skel_t *this);

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

static void draw(render_skel_t *this)
{
	render_skel_gl_t *gl_skel = (render_skel_gl_t *) this;

	/* Init OpenGL rendering */

	gl_skel->softDraw(this);
}

#endif /* ENABLE_OPENGL */
