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

/*--- External types ---*/

struct render_skel_s;
struct render_rexture_s;

/*--- Types ---*/

typedef struct render_skel_gl_s render_skel_gl_t;

struct render_skel_gl_s {
	struct render_skel_s render_skel;

	void (*softDraw)(struct render_skel_s *this, int num_parent);
};

/*--- Functions prototypes ---*/

/* Create a skeleton */
render_skel_t *render_skel_gl_create(void *emd_file, Uint32 emd_length, struct render_texture_s *texture);

#endif /* RENDER_SKEL_OPENGL_H */
