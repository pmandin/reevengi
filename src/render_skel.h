/*
	3D skeleton, composed of meshes

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

#ifndef RENDER_SKEL_H
#define RENDER_SKEL_H 1

typedef struct render_skel_s render_skel_t;

struct render_skel_s {
	void (*shutdown)(render_skel_t *this);

	/* Send/remove skeleton from video card */
	void (*upload)(render_skel_t *this);
	void (*download)(render_skel_t *this);

	void (*addMesh)(render_skel_t *this, render_mesh_t *mesh);

	void (*drawSkel)(render_skel_t *this);

	int num_meshes;
	render_mesh_t **meshes;

	render_texture_t *texture;
};

/*--- Functions prototypes ---*/

/* Create a skeleton */
render_skel_t *render_skel_create(render_texture_t *texture);

#endif /* RENDER_SKEL_H */
