/*
	Load EMD model

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

#include <SDL.h>

#include "filesystem.h"
#include "video.h"
#include "render.h"

/*--- Defines ---*/

#define EMD_MESHES 7

/*--- Variables ---*/

static void *emd_file = NULL;
static void *tim_file = NULL;

/*--- Types ---*/

typedef struct {
	Uint32 offset;
	Uint32 length;
} emd_header_t;

typedef struct {
	Sint16 x,y,z,w;
} emd_vertex_t;

typedef struct {
	Uint16	n0,v0;
	Uint16	n1,v1;
	Uint16	n2,v2;
} emd_triangle_t;

typedef struct {
	unsigned char u0,v0;
	unsigned short clutid;
	unsigned char u1,v1;
	unsigned short page;
	unsigned char u2,v2;
	unsigned short dummy;
} emd_triangle_tex_t;

typedef struct {
	Uint16	n0,v0;
	Uint16	n1,v1;
	Uint16	n2,v2;
	Uint16	n3,v3;
} emd_quad_t;

typedef struct {
	unsigned char u0,v0;
	unsigned short clutid;
	unsigned char u1,v1;
	unsigned short page;
	unsigned char u2,v2;
	unsigned short dummy0;
	unsigned char u3,v3;
	unsigned short dummy1;
} emd_quad_tex_t;

typedef struct {
	Uint32	vtx_offset;
	Uint32	vtx_count;
	Uint32	nor_offset;
	Uint32	nor_count;
	Uint32	mesh_offset;
	Uint32	mesh_count;
	Uint32	tex_offset;
} emd_mesh_t;

typedef struct {
	Uint32 length;
	Uint32 dummy;
	Uint32 num_objects;
} emd_mesh_header_t;

typedef struct {
	emd_mesh_t triangles;
	emd_mesh_t quads;
} emd_mesh_object_t;

/*--- Functions prototypes ---*/

/*--- Functions ---*/

int model_emd_load(const char *filename)
{
	PHYSFS_sint64 length;
	int retval = 0;
	char *tim_filename;
	
	emd_file = FS_Load(filename, &length);
	if (!emd_file) {
		fprintf(stderr, "emd: Can not load %s\n", filename);
		return 0;
	}

	tim_filename = calloc(1, strlen(filename)+1);
	if (!tim_filename) {
		fprintf(stderr, "emd: Can not allocate memory for filename\n");
		return 0;
	}
	strncpy(tim_filename, filename, strlen(filename)-4);
	strcat(tim_filename, ".tim");

	tim_file = FS_Load(tim_filename, &length);
	if (!tim_file) {
		fprintf(stderr, "emd: Can not load %s\n", tim_filename);
	} else {
		retval = 1;
	}
	free(tim_filename);

	return retval;
}

void model_emd_close(void)
{
	if (emd_file) {
		free(emd_file);
		emd_file = NULL;
	}

	if (tim_file) {
		free(tim_file);
		tim_file = NULL;
	}
}

void model_emd_draw(void)
{
	emd_header_t *emd_header;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;
	Uint32 *hdr_offsets;
	int num_objects, i, j;
	/*static int firsttime = 1;*/

	if (!emd_file) {
		return;
	}

	/*if (!firsttime) {
		return;
	}
	firsttime=0;*/

	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[SDL_SwapLE32(emd_header->offset)]);

	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[SDL_SwapLE32(hdr_offsets[EMD_MESHES])]);
	num_objects = SDL_SwapLE32(emd_mesh_header->num_objects)/2;
	/*printf("mesh: %d objects\n", num_objects);*/

	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[SDL_SwapLE32(hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t))]);

	for (i=0; i<num_objects; i++) {
		int num_tri = SDL_SwapLE32(emd_mesh_object->triangles.mesh_count);
		emd_vertex_t *emd_tri_vtx = (emd_vertex_t *)
			(&((char *) emd_file)
			[SDL_SwapLE32(hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t))+emd_mesh_object->triangles.vtx_offset]);		
		emd_triangle_t *emd_tri_idx = (emd_triangle_t *)
			(&((char *) emd_file)
			[SDL_SwapLE32(hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t))+emd_mesh_object->triangles.mesh_offset]);

		int num_quads = SDL_SwapLE32(emd_mesh_object->quads.mesh_count);
		emd_vertex_t *emd_quad_vtx = (emd_vertex_t *)
			(&((char *) emd_file)
			[SDL_SwapLE32(hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t))+emd_mesh_object->quads.vtx_offset]);
		emd_quad_t *emd_quad_idx = (emd_quad_t *)
			(&((char *) emd_file)
			[SDL_SwapLE32(hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t))+emd_mesh_object->quads.mesh_offset]);

		/*printf("mesh: object %d: %d triangles, %d quads\n", i, num_tri, num_quads);*/

		render.push_matrix();
		render.translate((i % 5)*1000, ((i/5)*2000) - 2000, 0);

		/* Draw triangles */
		for (j=0; j<num_tri; j++) {
			int v0 = emd_tri_idx[j].v0;
			int v1 = emd_tri_idx[j].v1;
			int v2 = emd_tri_idx[j].v2;

			/*printf(" triangle %d: %d,%d,%d\n", j,v0,v1,v2);*/

			render.line(
				emd_tri_vtx[v0].x, emd_tri_vtx[v0].y, emd_tri_vtx[v0].z,
				emd_tri_vtx[v1].x, emd_tri_vtx[v1].y, emd_tri_vtx[v1].z
			);
			render.line(
				emd_tri_vtx[v1].x, emd_tri_vtx[v1].y, emd_tri_vtx[v1].z,
				emd_tri_vtx[v2].x, emd_tri_vtx[v2].y, emd_tri_vtx[v2].z
			);
			render.line(
				emd_tri_vtx[v2].x, emd_tri_vtx[v2].y, emd_tri_vtx[v2].z,
				emd_tri_vtx[v0].x, emd_tri_vtx[v0].y, emd_tri_vtx[v0].z
			);
		}

		/* Draw quads */
		for (j=0; j<num_quads; j++) {
			int v0 = emd_quad_idx[j].v0;
			int v1 = emd_quad_idx[j].v1;
			int v2 = emd_quad_idx[j].v2;
			int v3 = emd_quad_idx[j].v3;

			/*printf(" quad %d: %d,%d,%d\n", j,v0,v1,v2,v3);*/

			render.line(
				emd_tri_vtx[v0].x, emd_tri_vtx[v0].y, emd_tri_vtx[v0].z,
				emd_tri_vtx[v1].x, emd_tri_vtx[v1].y, emd_tri_vtx[v1].z
			);
			render.line(
				emd_tri_vtx[v1].x, emd_tri_vtx[v1].y, emd_tri_vtx[v1].z,
				emd_tri_vtx[v3].x, emd_tri_vtx[v3].y, emd_tri_vtx[v3].z
			);
			render.line(
				emd_tri_vtx[v3].x, emd_tri_vtx[v3].y, emd_tri_vtx[v3].z,
				emd_tri_vtx[v2].x, emd_tri_vtx[v2].y, emd_tri_vtx[v2].z
			);
			render.line(
				emd_tri_vtx[v2].x, emd_tri_vtx[v2].y, emd_tri_vtx[v2].z,
				emd_tri_vtx[v0].x, emd_tri_vtx[v0].y, emd_tri_vtx[v0].z
			);
		}

		render.pop_matrix();

		emd_mesh_object++;
	}
}
