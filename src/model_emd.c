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

#define EMD_SKELETON 2
#define EMD_MESHES 7

/*--- Variables ---*/

static void *emd_file = NULL;
static void *tim_file = NULL;

/*--- Types ---*/

typedef struct {
	Uint32 offset;
	Uint32 length;
} emd_header_t;

/*
64 00
b0 00
0f 00
50 00 00 00
ee f8 00 00 e3 ff
ed ff 00 00 01 00
78 00 40 ff 09 00
e2 02 14 00 c9 ff
2f 03 f8 ff 03 00
7d 00 bf 00 0a 00
e2 02 ef ff ca ff
2b 03 08 00 9e ff
40 fd 00 00 c5 ff
4c fd 8f fe f5 ff
e5 01 b0 ff 04 00
c6 01 d3 ff c5 ff
4c fd 71 01 f5 ff
e5 01 50 00 04 00
c6 01 2d 00 00 00
04 00 3c 00
02 00 40 00
01 00 42 00
01 00 43 00
00 00 44 00
01 00 44 00	
01 00 45 00
00 00 46 00
00 00 46 00
01 00 46 00
01 00 47 00
00 00 48 00
01 00 48 00
01 00 49 00
00 00 4a 00
01 08 09 0c 02 05 03 04 06 07 0a 0b 0d 0e 00 00

0 buste
1 tronc
2 cuisse1
3 jambe1
4 pied1
5 cuisse2
6 jambe2
7 pied2
8 tete
9 bras1
10 avbras1
11 main1
12 bras2
13 avbras2
14 main2
*/

typedef struct {
	Sint16	x,y,z;
} emd_skel_relpos_t;

typedef struct {
	Uint16	num_mesh;
	Uint16	offset;
} emd_skel_data_t;

typedef struct {
	Uint16	relpos_offset;
	Uint16	unk_offset;
	Uint16	count;
	Uint16	size;
} emd_skel_header_t;

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

static void emd_convert_endianness(void);
static void emd_convert_endianness_skel(
	int num_skel,
	emd_skel_relpos_t *emd_skel_relpos,
	emd_skel_data_t *emd_skel_data);

static void emd_draw_skel(int num_skel,
	emd_skel_relpos_t *emd_skel_relpos,
	emd_skel_data_t *emd_skel_data);
static void emd_draw_mesh(int num_mesh);

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
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	emd_convert_endianness();
#endif

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
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_skel_data_t *emd_skel_data;
	Uint32 *hdr_offsets;

	if (!emd_file) {
		return;
	}

	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[emd_header->offset]);

	emd_skel_header = (emd_skel_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]]);
	emd_skel_relpos = (emd_skel_relpos_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]+sizeof(emd_skel_header_t)]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]+emd_skel_header->relpos_offset]);

	emd_draw_skel(0, emd_skel_relpos, emd_skel_data);
}

static void emd_draw_skel(int num_skel,
	emd_skel_relpos_t *emd_skel_relpos,
	emd_skel_data_t *emd_skel_data)
{
	Uint8 *emd_skel_mesh = (Uint8 *) emd_skel_data;
	int i;

	render.push_matrix();
	render.translate(
		emd_skel_relpos[num_skel].x,
		emd_skel_relpos[num_skel].y,
		emd_skel_relpos[num_skel].z
	);

	/* Draw current mesh */
	emd_draw_mesh(num_skel);

	/* Draw children meshes */
	for (i=0; i<emd_skel_data[num_skel].num_mesh; i++) {
		int num_mesh = emd_skel_mesh[emd_skel_data[num_skel].offset+i];
		emd_draw_skel(num_mesh, emd_skel_relpos, emd_skel_data);
	}

	render.pop_matrix();
}

static void emd_draw_mesh(int num_mesh)
{
	emd_header_t *emd_header;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;
	Uint32 *hdr_offsets, mesh_offset;
	Sint32 posx,posy,posz;
	int num_objects, i;
	emd_vertex_t *emd_tri_vtx, *emd_quad_vtx;
	emd_triangle_t *emd_tri_idx;
	emd_quad_t *emd_quad_idx;

	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[emd_header->offset]);

	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_MESHES]]);
	num_objects = emd_mesh_header->num_objects/2;

	if ((num_mesh<0) || (num_mesh>=num_objects)) {
		fprintf(stderr, "Invalid mesh %d\n", num_mesh);
		return;
	}

	mesh_offset = hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t);

	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[mesh_offset]);
	emd_mesh_object = &emd_mesh_object[num_mesh];

	/* Draw triangles */
	emd_tri_vtx = (emd_vertex_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->triangles.vtx_offset]);
	emd_tri_idx = (emd_triangle_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->triangles.mesh_offset]);

	for (i=0; i<emd_mesh_object->triangles.mesh_count; i++) {
		render.triangle(
			&emd_tri_vtx[emd_tri_idx[i].v0].x,
			&emd_tri_vtx[emd_tri_idx[i].v1].x,
			&emd_tri_vtx[emd_tri_idx[i].v2].x
		);
	}

	/* Draw quads */
	emd_quad_vtx = (emd_vertex_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->quads.vtx_offset]);
	emd_quad_idx = (emd_quad_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->quads.mesh_offset]);

	for (i=0; i<emd_mesh_object->quads.mesh_count; i++) {
		render.quad(
			&emd_quad_vtx[emd_quad_idx[i].v0].x,
			&emd_quad_vtx[emd_quad_idx[i].v1].x,
			&emd_quad_vtx[emd_quad_idx[i].v3].x,
			&emd_quad_vtx[emd_quad_idx[i].v2].x
		);
	}
}

/*--- Convert EMD file (little endian) to big endian ---*/

static void emd_convert_endianness(void)
{
	emd_header_t *emd_header;
	Uint32 *hdr_offsets, mesh_offset;
	int i;
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_skel_data_t *emd_skel_data;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;

	/* Header */
	emd_header = (emd_header_t *) emd_file;
	emd_header->offset = SDL_SwapLE32(emd_header->offset);
	emd_header->length = SDL_SwapLE32(emd_header->length);

	/* Directory offsets */
	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[emd_header->offset]);
	for (i=0; i<8; i++) {
		hdr_offsets[i] = SDL_SwapLE32(hdr_offsets[i]);
	}

	/* Offset 2: Skeleton */
	emd_skel_header = (emd_skel_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]]);
	emd_skel_header->relpos_offset = SDL_SwapLE16(emd_skel_header->relpos_offset);
	emd_skel_header->unk_offset = SDL_SwapLE16(emd_skel_header->unk_offset);
	emd_skel_header->count = SDL_SwapLE16(emd_skel_header->count);
	emd_skel_header->size = SDL_SwapLE16(emd_skel_header->size);

	emd_skel_relpos = (emd_skel_relpos_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]+sizeof(emd_skel_header_t)]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]+emd_skel_header->relpos_offset]);

	emd_convert_endianness_skel(0, emd_skel_relpos, emd_skel_data);

	/* Offset 7: Mesh data */
	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_MESHES]]);
	emd_mesh_header->length = SDL_SwapLE32(emd_mesh_header->length);
	emd_mesh_header->dummy = SDL_SwapLE32(emd_mesh_header->dummy);
	emd_mesh_header->num_objects = SDL_SwapLE32(emd_mesh_header->num_objects);

	mesh_offset = hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t);
	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[mesh_offset]);
	for (i=0; i<emd_mesh_header->num_objects/2; i++) {
		int j;
		emd_vertex_t *emd_vtx;
		emd_triangle_t *emd_tri_idx;
		emd_quad_t *emd_quad_idx;
		emd_vertex_t **list_vtx_done;

		/* Triangles */
		emd_mesh_object->triangles.vtx_offset = SDL_SwapLE32(emd_mesh_object->triangles.vtx_offset);
		emd_mesh_object->triangles.vtx_count = SDL_SwapLE32(emd_mesh_object->triangles.vtx_count);
		emd_mesh_object->triangles.nor_offset = SDL_SwapLE32(emd_mesh_object->triangles.nor_offset);
		emd_mesh_object->triangles.nor_count = SDL_SwapLE32(emd_mesh_object->triangles.nor_count);
		emd_mesh_object->triangles.mesh_offset = SDL_SwapLE32(emd_mesh_object->triangles.mesh_offset);
		emd_mesh_object->triangles.mesh_count = SDL_SwapLE32(emd_mesh_object->triangles.mesh_count);
		emd_mesh_object->triangles.tex_offset = SDL_SwapLE32(emd_mesh_object->triangles.tex_offset);

		list_vtx_done = (emd_vertex_t **) malloc(sizeof(emd_vertex_t *)*
			(emd_mesh_object->triangles.vtx_count+emd_mesh_object->triangles.nor_count));
		if (!list_vtx_done) {
			fprintf(stderr, "Can not allocate mem for vtx/nor list conversion\n");
			break;
		}

		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->triangles.vtx_offset]);
		for (j=0; j<emd_mesh_object->triangles.vtx_count; j++) {
			emd_vtx[j].x = SDL_SwapLE16(emd_vtx[j].x);
			emd_vtx[j].y = SDL_SwapLE16(emd_vtx[j].y);
			emd_vtx[j].z = SDL_SwapLE16(emd_vtx[j].z);
			emd_vtx[j].w = SDL_SwapLE16(emd_vtx[j].w);
			list_vtx_done[j] = &emd_vtx[j];
		}

		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->triangles.nor_offset]);
		for (j=0; j<emd_mesh_object->triangles.nor_count; j++) {
			emd_vtx[j].x = SDL_SwapLE16(emd_vtx[j].x);
			emd_vtx[j].y = SDL_SwapLE16(emd_vtx[j].y);
			emd_vtx[j].z = SDL_SwapLE16(emd_vtx[j].z);
			emd_vtx[j].w = SDL_SwapLE16(emd_vtx[j].w);
			list_vtx_done[emd_mesh_object->triangles.vtx_count+j] = &emd_vtx[j];
		}

		emd_tri_idx = (emd_triangle_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->triangles.mesh_offset]);
		for (j=0; j<emd_mesh_object->triangles.mesh_count; j++) {
			emd_tri_idx[j].n0 = SDL_SwapLE16(emd_tri_idx[j].n0);
			emd_tri_idx[j].v0 = SDL_SwapLE16(emd_tri_idx[j].v0);
			emd_tri_idx[j].n1 = SDL_SwapLE16(emd_tri_idx[j].n1);
			emd_tri_idx[j].v1 = SDL_SwapLE16(emd_tri_idx[j].v1);
			emd_tri_idx[j].n2 = SDL_SwapLE16(emd_tri_idx[j].n2);
			emd_tri_idx[j].v2 = SDL_SwapLE16(emd_tri_idx[j].v2);

			/* FIXME: convert texture info */
		}

		/* Quads */
		emd_mesh_object->quads.vtx_offset = SDL_SwapLE32(emd_mesh_object->quads.vtx_offset);
		emd_mesh_object->quads.vtx_count = SDL_SwapLE32(emd_mesh_object->quads.vtx_count);
		emd_mesh_object->quads.nor_offset = SDL_SwapLE32(emd_mesh_object->quads.nor_offset);
		emd_mesh_object->quads.nor_count = SDL_SwapLE32(emd_mesh_object->quads.nor_count);
		emd_mesh_object->quads.mesh_offset = SDL_SwapLE32(emd_mesh_object->quads.mesh_offset);
		emd_mesh_object->quads.mesh_count = SDL_SwapLE32(emd_mesh_object->quads.mesh_count);
		emd_mesh_object->quads.tex_offset = SDL_SwapLE32(emd_mesh_object->quads.tex_offset);

		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->quads.vtx_offset]);
		for (j=0; j<emd_mesh_object->quads.vtx_count; j++) {
			/* Check not already converted */
			int k, must_skip = 0;
			for (k=0; k<emd_mesh_object->triangles.vtx_count; k++) {
				if (list_vtx_done[k] == &emd_vtx[j]) {
					must_skip = 1;
					break;
				}
			}
			if (must_skip) {
				continue;
			}

			emd_vtx[j].x = SDL_SwapLE16(emd_vtx[j].x);
			emd_vtx[j].y = SDL_SwapLE16(emd_vtx[j].y);
			emd_vtx[j].z = SDL_SwapLE16(emd_vtx[j].z);
			emd_vtx[j].w = SDL_SwapLE16(emd_vtx[j].w);
		}

		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->quads.nor_offset]);
		for (j=0; j<emd_mesh_object->quads.nor_count; j++) {
			int k, must_skip = 0;
			for (k=0; k<emd_mesh_object->triangles.nor_count; k++) {
				if (list_vtx_done[emd_mesh_object->triangles.vtx_count+k] == &emd_vtx[j]) {
					must_skip = 1;
					break;
				}
			}
			if (must_skip) {
				continue;
			}

			emd_vtx[j].x = SDL_SwapLE16(emd_vtx[j].x);
			emd_vtx[j].y = SDL_SwapLE16(emd_vtx[j].y);
			emd_vtx[j].z = SDL_SwapLE16(emd_vtx[j].z);
			emd_vtx[j].w = SDL_SwapLE16(emd_vtx[j].w);
		}

		emd_quad_idx = (emd_quad_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->quads.mesh_offset]);
		for (j=0; j<emd_mesh_object->quads.mesh_count; j++) {
			emd_quad_idx[j].n0 = SDL_SwapLE16(emd_quad_idx[j].n0);
			emd_quad_idx[j].v0 = SDL_SwapLE16(emd_quad_idx[j].v0);
			emd_quad_idx[j].n1 = SDL_SwapLE16(emd_quad_idx[j].n1);
			emd_quad_idx[j].v1 = SDL_SwapLE16(emd_quad_idx[j].v1);
			emd_quad_idx[j].n2 = SDL_SwapLE16(emd_quad_idx[j].n2);
			emd_quad_idx[j].v2 = SDL_SwapLE16(emd_quad_idx[j].v2);
			emd_quad_idx[j].n3 = SDL_SwapLE16(emd_quad_idx[j].n3);
			emd_quad_idx[j].v3 = SDL_SwapLE16(emd_quad_idx[j].v3);

			/* FIXME: convert texture info */
		}

		free(list_vtx_done);

		emd_mesh_object++;
	}
}

static void emd_convert_endianness_skel(int num_skel,
	emd_skel_relpos_t *emd_skel_relpos,
	emd_skel_data_t *emd_skel_data)
{
	/* FIXME: mark already converted skel parts, to avoid multiple conversion if needed*/
	int i;
	Uint8 *emd_skel_mesh = (Uint8 *) emd_skel_data;

	emd_skel_relpos[num_skel].x = SDL_SwapLE16(emd_skel_relpos[num_skel].x);
	emd_skel_relpos[num_skel].y = SDL_SwapLE16(emd_skel_relpos[num_skel].y);
	emd_skel_relpos[num_skel].z = SDL_SwapLE16(emd_skel_relpos[num_skel].z);

	emd_skel_data[num_skel].num_mesh = SDL_SwapLE16(emd_skel_data[num_skel].num_mesh);
	emd_skel_data[num_skel].offset = SDL_SwapLE16(emd_skel_data[num_skel].offset);
	for (i=0; i<emd_skel_data[num_skel].num_mesh; i++) {
		int num_mesh = emd_skel_mesh[emd_skel_data[num_skel].offset+i];
		emd_convert_endianness_skel(num_mesh, emd_skel_relpos, emd_skel_data);
	}
}
