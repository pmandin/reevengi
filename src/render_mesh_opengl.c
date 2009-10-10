/*
	3D mesh
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
#include "render_mesh_opengl.h"
#include "video.h"
#include "render.h"
#include "log.h"

/*--- Functions prototypes ---*/

static void upload(render_mesh_t *this);
static void download(render_mesh_t *this);

static void draw(render_mesh_t *this);

/*--- Functions ---*/

render_mesh_t *render_mesh_gl_create(render_texture_t *texture)
{
	render_mesh_t *mesh;
	render_mesh_gl_t *gl_mesh;

	mesh = render_mesh_create(texture);
	if (!mesh) {
		return NULL;
	}

	gl_mesh = realloc(mesh, sizeof(render_mesh_gl_t));
	if (!gl_mesh) {
		fprintf(stderr, "Can not allocate memory for mesh\n");
		return NULL;
	}

	mesh = (render_mesh_t *) gl_mesh;

	mesh->upload = upload;
	mesh->download = download;

	mesh->draw = draw;

	gl_mesh->num_list = 0xFFFFFFFFUL;

	return mesh;
}

static void upload(render_mesh_t *this)
{
	render_mesh_gl_t *gl_mesh = (render_mesh_gl_t *) this;

	gl_mesh->num_list = gl.GenLists(1);	
}

static void download(render_mesh_t *this)
{
	render_mesh_gl_t *gl_mesh = (render_mesh_gl_t *) this;

	if (gl_mesh->num_list == 0xFFFFFFFFUL) {
		return;
	}

	gl.DeleteLists(gl_mesh->num_list, 1);
	gl_mesh->num_list = 0xFFFFFFFFUL;
}

static void draw(render_mesh_t *this)
{
	render_mesh_gl_t *gl_mesh = (render_mesh_gl_t *) this;

	if (!gl_mesh) {
		return;
	}
	if (gl_mesh->num_list == 0xFFFFFFFFUL) {
		this->upload(this);
	}
}

#endif /* ENABLE_OPENGL */
