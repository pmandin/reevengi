/*
	3D mesh

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

#ifndef RENDER_MESH_H
#define RENDER_MESH_H 1

/*--- Defines ---*/

enum {
	RENDER_ARRAY_BYTE=0,
	RENDER_ARRAY_SHORT,
};

enum {
	RENDER_ARRAY_VERTEX=0,
	RENDER_ARRAY_NORMAL,
	RENDER_ARRAY_TEXCOORD
};

/*--- Types ---*/

typedef struct {
	void *data;

	int stride;	/* n */
	int size;	/* 2,3,4 */
	int type;	/* byte, short */
} render_mesh_array_t;

typedef struct {
	int v[3];
	int n[3];
	int tx[3];
	int txpal;
} render_mesh_tri_t;

typedef struct {
	int v[4];
	int n[4];
	int tx[4];
	int txpal;
} render_mesh_quad_t;

typedef struct render_mesh_s render_mesh_t;

struct render_mesh_s {
	void (*shutdown)(render_mesh_t *this);

	/* Send/remove mesh from video card */
	void (*upload)(render_mesh_t *this);
	void (*download)(render_mesh_t *this);

	void (*setArray)(render_mesh_t *this, int array_type, int size, int type,
		int stride, void *data);
	void (*addTriangle)(render_mesh_t *this, render_mesh_tri_t *tri);
	void (*addQuad)(render_mesh_t *this, render_mesh_quad_t *quad);

	void (*drawMesh)(render_mesh_t *this);

	render_mesh_array_t *vertex;
	render_mesh_array_t *normal;
	render_mesh_array_t *texcoord;

	int num_tris;
	render_mesh_tri_t *triangles;

	int num_quads;
	render_mesh_quad_t *quads;

	render_texture_t *texture;
};

/*--- Functions prototypes ---*/

/* Create a mesh */
render_mesh_t *render_mesh_create(render_texture_t *texture);

#endif /* RENDER_MESH_H */
