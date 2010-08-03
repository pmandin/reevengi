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

#include "render_mesh.h"

/*--- Types ---*/

typedef struct render_skel_mesh_s render_skel_mesh_t;

struct render_skel_mesh_s {
	Sint16 x,y,z;	/* Relative mesh position */
	render_mesh_t *mesh;
};

typedef struct render_skel_s render_skel_t;

struct render_skel_s {
	void (*shutdown)(render_skel_t *this);

	void *emd_file;	/* EMD file for model */
	Uint32 emd_length;

	/* Send/remove skeleton from video card */
	void (*upload)(render_skel_t *this);
	void (*download)(render_skel_t *this);

	void (*addMesh)(render_skel_t *this, render_mesh_t *mesh,
		Sint16 x, Sint16 y, Sint16 z);

	void (*draw)(render_skel_t *this, int num_parent);
	void (*drawBones)(render_skel_t *this, int num_parent);

	int num_meshes;
	render_skel_mesh_t *meshes;

	render_texture_t *texture;

	/*--- Hierarchy ---*/

	/* Returns num_child-th child mesh of num_parent mesh, -1 if end of list */
	int (*getChild)(render_skel_t *this, int num_parent, int num_child);

	/*--- For animation ---*/
	int num_anim;
	int num_frame;

	int (*getNumAnims)(render_skel_t *this);

	/* Set anim frame, returns 1 if OK, 0 if wrong (animation stopped) */
	int (*setAnimFrame)(render_skel_t *this, int num_anim, int num_frame);

	void (*getAnimPosition)(render_skel_t *this, int *x, int *y, int *z);
	void (*getAnimSpeed)(render_skel_t *this, int *x, int *y, int *z);
	void (*getAnimAngles)(render_skel_t *this, int num_mesh, int *x, int *y, int *z); 
};

/*--- Functions prototypes ---*/

/* Create a skeleton */
render_skel_t *render_skel_create(void *emd_file, Uint32 emd_length, render_texture_t *texture);

#endif /* RENDER_SKEL_H */
