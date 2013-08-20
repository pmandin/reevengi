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

#ifndef RENDER_MESH_OPENGL_H
#define RENDER_MESH_OPENGL_H 1

/*#include "render_mesh.h"*/

/*--- External types ---*/

typedef struct render_mesh_s render_mesh_t;

/*--- Types ---*/

typedef struct render_mesh_gl_s render_mesh_gl_t;

struct render_mesh_gl_s {
	render_mesh_t render_mesh;

	GLuint	num_list;
};

/*--- Functions prototypes ---*/

/* Create a mesh */
render_mesh_t *render_mesh_gl_create(render_texture_t *texture);

#endif /* RENDER_MESH_OPENGL_H */
