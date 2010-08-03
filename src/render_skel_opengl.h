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

#ifndef RENDER_SKEL_OPENGL_H
#define RENDER_SKEL_OPENGL_H 1

#include "render_skel.h"

/*--- Types ---*/

typedef struct render_skel_gl_s render_skel_gl_t;

struct render_skel_gl_s {
	render_skel_t render_skel;

#if 0
	void (*softDraw)(render_skel_t *this, render_skel_mesh_t *parent);
#else
	void (*softDraw)(render_skel_t *this, int num_parent);
#endif
};

/*--- Functions prototypes ---*/

/* Create a skeleton */
render_skel_t *render_skel_gl_create(void *emd_file, Uint32 emd_length, render_texture_t *texture);

#endif /* RENDER_SKEL_OPENGL_H */
