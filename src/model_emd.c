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
	static int firsttime = 1;

	if (!emd_file) {
		return;
	}

	if (!firsttime) {
		return;
	}
	firsttime=0;

	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[SDL_SwapLE32(emd_header->offset)]);

	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[SDL_SwapLE32(hdr_offsets[EMD_MESHES])]);
	num_objects = SDL_SwapLE32(emd_mesh_header->num_objects)/2;
	printf("mesh: %d objects\n", num_objects);

	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[SDL_SwapLE32(hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t))]);

	for (i=0; i<num_objects; i++) {
		int num_tri = SDL_SwapLE32(emd_mesh_object->triangles.mesh_count);
		int num_quads = SDL_SwapLE32(emd_mesh_object->quads.mesh_count);

		printf("mesh: object %d: %d triangles, %d quads\n", i, num_tri, num_quads);

		/* Draw triangles */
		for (j=0; j<num_tri; j++) {
		}

		/* Draw quads */
		for (j=0; j<num_quads; j++) {
		}

		emd_mesh_object++;
	}
}
