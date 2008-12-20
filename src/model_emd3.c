/*
	Load EMD model
	Resident Evil 3

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
#include "model.h"
#include "model_emd3.h"

/*--- Defines ---*/

#define EMD_SKELETON 3
#define EMD_MESHES 13

/*--- Types ---*/

typedef struct {
	Uint32 offset;
	Uint32 length;
} emd_header_t;

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
	unsigned char tu0,tv0;
	unsigned char dummy0, dummy1;

	unsigned char tu1,tv1;
	unsigned char dummy2, v0;

	unsigned char tu2,tv2;
	unsigned char v1,v2;
} emd_triangle_t;

typedef struct {
	unsigned char tu0,tv0;
	unsigned char dummy0, dummy1;

	unsigned char tu1,tv1;
	unsigned char dummy2, dummy3;

	unsigned char tu2,tv2;
	unsigned char v0,v1;

	unsigned char tu3,tv3;
	unsigned char v2,v3;
} emd_quad_t;

typedef struct {
	Uint32	vtx_offset;
	Uint32	nor_offset;
	Uint32	vtx_count;
	Uint32	tri_offset;
	Uint32	quad_offset;
	Uint16	tri_count;
	Uint16	quad_count;
} emd_mesh_object_t;

typedef struct {
	Uint32 length;
	Uint32 num_objects;
} emd_mesh_header_t;

/*--- Functions prototypes ---*/

static void model_emd3_shutdown(model_t *this);
static void model_emd3_draw(model_t *this);

static void emd_convert_endianness(model_t *this);
static void emd_convert_endianness_skel(
	model_t *this, int num_skel,
	emd_skel_relpos_t *emd_skel_relpos,
	emd_skel_data_t *emd_skel_data);

static void emd_draw_skel(model_t *this, int num_skel,
	emd_skel_relpos_t *emd_skel_relpos,
	emd_skel_data_t *emd_skel_data);
static void emd_draw_mesh(model_t *this, int num_mesh);

/*--- Functions ---*/

model_t *model_emd3_load(SDL_RWops *src_emd, SDL_RWops *src_tim)
{
	model_t	*model;

	model = (model_t *) calloc(1, sizeof(model_t));
	if (!model) {
		fprintf(stderr, "Can not allocate memory for model\n");
		return NULL;
	}

	model->emd_file = FS_LoadRW(src_emd, &(model->emd_length));
	if (!model->emd_file) {
		fprintf(stderr, "Can not allocate memory for EMD file\n");
		free(model);
		return NULL;
	}

	model->tim_file = FS_LoadRW(src_tim, &(model->tim_length));
	if (!model->tim_file) {
		fprintf(stderr, "Can not allocate memory for TIM file\n");
		free(model->emd_file);
		free(model);
		return NULL;
	}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	emd_convert_endianness(model);
#endif

	model->shutdown = model_emd3_shutdown;
	model->draw = model_emd3_draw;

	return model;
}

static void model_emd3_shutdown(model_t *this)
{
	if (this) {
		if (this->emd_file) {
			free(this->emd_file);
		}
		if (this->tim_file) {
			free(this->tim_file);
		}
		free(this);
	}
}

static void model_emd3_draw(model_t *this)
{
	emd_header_t *emd_header;
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_skel_data_t *emd_skel_data;
	Uint32 *hdr_offsets;
	void *emd_file;

	if (!this) {
		return;
	}
	if (!this->emd_file) {
		return;
	}
	emd_file = this->emd_file;

	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[emd_header->offset]);

	emd_skel_header = (emd_skel_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]]);
	emd_skel_relpos = (emd_skel_relpos_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]+sizeof(emd_skel_header_t)]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_SKELETON]+emd_skel_header->relpos_offset]);

	emd_draw_skel(this, 0, emd_skel_relpos, emd_skel_data);
}

static void emd_draw_skel(model_t *this, int num_skel,
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
	emd_draw_mesh(this, num_skel);

	/* Draw children meshes */
	for (i=0; i<emd_skel_data[num_skel].num_mesh; i++) {
		int num_mesh = emd_skel_mesh[emd_skel_data[num_skel].offset+i];
		emd_draw_skel(this, num_mesh, emd_skel_relpos, emd_skel_data);
	}

	render.pop_matrix();
}

static void emd_draw_mesh(model_t *this, int num_mesh)
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
	void *emd_file = this->emd_file;

	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[emd_header->offset]);

	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_MESHES]]);
	num_objects = emd_mesh_header->num_objects;

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
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->vtx_offset]);
	emd_tri_idx = (emd_triangle_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->tri_offset]);

	for (i=0; i<emd_mesh_object->tri_count; i++) {
		render.triangle(
			&emd_tri_vtx[emd_tri_idx[i].v0].x,
			&emd_tri_vtx[emd_tri_idx[i].v1].x,
			&emd_tri_vtx[emd_tri_idx[i].v2].x
		);
	}

	/* Draw quads */
	emd_quad_vtx = (emd_vertex_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->vtx_offset]);
	emd_quad_idx = (emd_quad_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->quad_offset]);

	for (i=0; i<emd_mesh_object->quad_count; i++) {
		render.quad(
			&emd_quad_vtx[emd_quad_idx[i].v0].x,
			&emd_quad_vtx[emd_quad_idx[i].v1].x,
			&emd_quad_vtx[emd_quad_idx[i].v3].x,
			&emd_quad_vtx[emd_quad_idx[i].v2].x
		);
	}
}

/*--- Convert EMD file (little endian) to big endian ---*/

static void emd_convert_endianness(model_t *this)
{
	emd_header_t *emd_header;
	Uint32 *hdr_offsets, mesh_offset;
	int i;
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_skel_data_t *emd_skel_data;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;
	void *emd_file = this->emd_file;

	/* Header */
	emd_header = (emd_header_t *) emd_file;
	emd_header->offset = SDL_SwapLE32(emd_header->offset);
	emd_header->length = SDL_SwapLE32(emd_header->length);

	/* Directory offsets */
	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[emd_header->offset]);
	for (i=0; i<emd_header->length; i++) {
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

	emd_convert_endianness_skel(this, 0, emd_skel_relpos, emd_skel_data);

	/* Offset 7: Mesh data */
	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_MESHES]]);
	emd_mesh_header->length = SDL_SwapLE32(emd_mesh_header->length);
	emd_mesh_header->num_objects = SDL_SwapLE32(emd_mesh_header->num_objects);

	mesh_offset = hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t);
	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[mesh_offset]);
	for (i=0; i<emd_mesh_header->num_objects; i++) {
		int j;
		emd_vertex_t *emd_vtx;
		emd_triangle_t *emd_tri_idx;
		emd_quad_t *emd_quad_idx;
		void **list_done;

		/* Mesh */
		emd_mesh_object->vtx_offset = SDL_SwapLE32(emd_mesh_object->vtx_offset);
		emd_mesh_object->nor_offset = SDL_SwapLE32(emd_mesh_object->nor_offset);
		emd_mesh_object->vtx_count = SDL_SwapLE32(emd_mesh_object->vtx_count);
		emd_mesh_object->tri_offset = SDL_SwapLE32(emd_mesh_object->tri_offset);
		emd_mesh_object->quad_offset = SDL_SwapLE32(emd_mesh_object->quad_offset);
		emd_mesh_object->tri_count = SDL_SwapLE16(emd_mesh_object->tri_count);
		emd_mesh_object->quad_count = SDL_SwapLE16(emd_mesh_object->quad_count);

		list_done = malloc(sizeof(void *)*(emd_mesh_object->vtx_count*2));
		if (!list_done) {
			fprintf(stderr, "Can not allocate mem for vtx/nor list conversion\n");
			break;
		}

		/* Triangles */
		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->vtx_offset]);
		for (j=0; j<emd_mesh_object->vtx_count; j++) {
			emd_vtx[j].x = SDL_SwapLE16(emd_vtx[j].x);
			emd_vtx[j].y = SDL_SwapLE16(emd_vtx[j].y);
			emd_vtx[j].z = SDL_SwapLE16(emd_vtx[j].z);
			emd_vtx[j].w = SDL_SwapLE16(emd_vtx[j].w);
			list_done[j] = &emd_vtx[j];
		}

		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->nor_offset]);
		for (j=0; j<emd_mesh_object->vtx_count; j++) {
			emd_vtx[j].x = SDL_SwapLE16(emd_vtx[j].x);
			emd_vtx[j].y = SDL_SwapLE16(emd_vtx[j].y);
			emd_vtx[j].z = SDL_SwapLE16(emd_vtx[j].z);
			emd_vtx[j].w = SDL_SwapLE16(emd_vtx[j].w);
			list_done[emd_mesh_object->vtx_count+j] = &emd_vtx[j];
		}

		/* Quads */
		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->vtx_offset]);
		for (j=0; j<emd_mesh_object->vtx_count; j++) {
			/* Check not already converted */
			int k, must_skip = 0;
			for (k=0; k<emd_mesh_object->vtx_count; k++) {
				if (list_done[k] == &emd_vtx[j]) {
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
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->nor_offset]);
		for (j=0; j<emd_mesh_object->vtx_count; j++) {
			int k, must_skip = 0;
			for (k=0; k<emd_mesh_object->vtx_count; k++) {
				if (list_done[emd_mesh_object->vtx_count+k] == &emd_vtx[j]) {
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

		free(list_done);

		emd_mesh_object++;
	}
}

static void emd_convert_endianness_skel(model_t *this, int num_skel,
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
		emd_convert_endianness_skel(this, num_mesh, emd_skel_relpos, emd_skel_data);
	}
}

/*
a4 26 00 00 0f 00 00 00

vtx_offset,nor_offset,count,tri_offset,tx_offset,?*4,tx_offset,tx_offset

9c 13 00 00 1c 1d 00 00 2b 00 00 00 68 01 00 00 48 03 00 00 28 00 13 00
f4 14 50 0e 74 1e 00 50 58 00 00 00 78 04 00 00 24 07 00 00 39 00 2f 00
b4 17 00 00 34 21 00 00 0d 0e 50 00 14 0a 00 00 44 0a 00 00 04 00 08 00
1c 18 00 00 9c 21 00 00 0e 00 00 00 c4 0a 00 00 0c 0b 00 00 06 00 09 00
8c 18 00 00 0c 22 00 00 12 00 00 00 9c 0b 00 00 b4 0b 00 00 02 00 0e 00
1c 19 00 00 9c 22 00 00 0d 0e 50 00 94 0c 0e 50 c4 0c 0e 50 04 00 08 00
84 19 00 00 04 23 00 00 0e 00 00 00 44 0d 0e 50 8c 0d 0e 50 06 00 09 00
f4 19 00 00 74 23 00 00 12 00 00 00 1c 0e 00 00 34 0e 00 00 02 00 0e 00
84 1a 00 00 04 24 00 00 0f 00 00 00 14 0f 00 00 44 0f 00 00 04 00 0b 00
fc 1a 00 00 7c 24 00 00 12 00 00 00 f4 0f 00 00 a8 10 00 00 0f 00 08 00
8c 1b 00 00 0c 25 00 00 08 00 00 00 28 11 00 00 28 11 00 00 00 00 04 00
cc 1b 00 00 4c 25 00 00 08 00 00 00 68 11 00 00 68 11 00 00 00 00 06 00
0c 1c 0e 50 8c 25 00 00 12 00 00 00 c8 11 00 00 7c 12 00 00 0f 00 08 00
9c 1c 0e 50 1c 26 00 00 08 00 00 00 fc 12 00 00 fc 12 00 00 00 00 04 00
dc 1c 0e 50 5c 26 00 00 08 00 00 00
*/

/*
model 0x2d

offset 0x20f64

70 1c 00 00 0f 00 00 00
48 0e 00 00 58 15 00 00 08 00 00 00 68 01 00 00 68 01 00 00 00 00 06 00
88 0e 00 00 98 15 00 00 15 00 00 00 c8 01 00 00 e8 02 00 00 18 00 07 00
30 0f 00 00 40 16 00 00 11 00 00 00 58 03 00 00 d0 03 00 00 0a 00 08 00
b8 0f 00 00 c8 16 00 00 17 00 00 00 50 04 00 00 c8 04 00 00 0a 00 10 00
70 10 00 00 80 17 00 00 0c 00 00 00 c8 05 00 00 c8 05 00 00 00 00 09 00
d0 10 00 00 e0 17 00 00 0c 00 00 00 58 06 00 00 58 06 00 00 00 00 09 00
30 11 00 00 40 18 00 00 0c 00 00 00 e8 06 00 00 e8 06 00 00 00 00 09 00
90 11 00 00 a0 18 00 00 0c 00 00 00 78 07 00 00 78 07 00 00 00 00 09 00
f0 11 00 00 00 19 00 00 15 00 00 00 08 08 00 00 28 09 00 00 18 00 07 00
98 12 00 00 a8 19 00 00 11 00 00 00 98 09 00 00 10 0a 00 00 0a 00 08 00
20 13 00 00 30 1a 00 00 17 00 00 00 90 0a 00 00 08 0b 00 00 0a 00 10 00
d8 13 00 00 e8 1a 00 00 0c 00 00 00 08 0c 00 00 08 0c 00 00 00 00 09 00
38 14 00 00 48 1b 00 00 0c 00 00 00 98 0c 00 00 98 0c 00 00 00 00 09 00
98 14 00 00 a8 1b 00 00 0c 00 00 00 28 0d 00 00 28 0d 00 00 00 00 09 00
f8 14 00 00 08 1c 00 00 0c 00 00 00 b8 0d 00 00 b8 0d 00 00 00 00 09 00

offset 0x168
                              v  v  v  v  v  v
04 00 00 78 00 00 80 00 04 03 02 03 00 03 00 01
00 00 00 78 00 00 80 00 00 03 03 07 00 03 01 05
00 00 00 78 04 00 80 00 00 03 07 06 04 03 05 04
04 00 00 78 04 00 80 00 04 03 06 02 04 03 04 00
04 03 00 78 00 03 80 00 04 03 00 01 00 03 04 05
04 00 00 78 00 00 80 00 04 00 06 07 00 00 02 03

offset 0x1c8
      v              v        v  v
26 1f 00 78 15 0f 80 0b 03 1f 07 01
2a 0e 00 78 15 0f 80 0e 26 1f 07 0b
58 04 00 78 52 0f 80 02 61 0f 12 03
00 0e 00 78 15 0f 80 0f 0c 00 07 05
36 58 00 78 28 1f 80 13 2e 58 0b 08
1d 00 00 78 0c 00 80 0d 15 0f 05 07
6b 5e 00 78 6b 58 80 09 62 58 14 0a
36 5e 00 78 36 58 80 09 2e 58 13 08

offset 0x2e8
                              v  v        v  v 
42 1f 00 78 41 0f 80 00 28 1f 11 0c 2a 0e 0b 0e
26 1f 00 78 03 1f 80 00 1d 58 0b 01 0c 58 08 06
7a 1f 00 78 78 0e 80 00 60 1f 01 0f 61 0f 10 03
49 00 00 78 4a 04 80 00 59 00 0d 04 58 04 05 02
5b 58 00 78 60 1f 80 00 49 58 0a 10 44 1f 00 11
41 0f 00 78 3d 04 80 00 2a 0e 0c 04 2f 00 0e 0d
78 0e 00 78 72 00 80 00 61 0f 0f 05 65 04 03 02

offset 0x450
                     v        v  v        v  v
54 ba 00 78 48 bf 80 04 5f bf 01 02
19 c0 00 78 11 ba 80 00 07 bf 04 02
40 bf 00 78 38 ba 80 01 30 c0 04 03
24 ba 00 78 18 c0 80 04 30 c0 00 03
13 c2 00 78 12 d4 80 0d 17 d3 0f 06
13 c2 00 78 17 d3 80 0d 18 c0 06 00
12 d4 00 78 13 c2 80 0f 0c cb 0d 15
30 c0 00 78 24 de 80 03 38 d8 11 07
24 de 00 78 30 c0 80 11 18 c0 03 00
65 d2 00 78 6b cb 80 10 63 c3 16 0e
60 d3 00 78 66 d4 80 00 5e d2 06 0f 65 d2 08 10
68 db 00 78 69 de 80 00 6e d9 0c 0b 6f dc 0a 09
6e d9 00 78 6b cb 80 00 68 db 0a 16 65 d2 0c 10
48 bf 00 78 42 d9 80 00 5f bf 01 05 5e d2 02 08
5e d2 00 78 65 d2 80 00 5f bf 08 10 63 c3 02 0e
18 c0 00 78 17 d3 80 00 24 de 00 06 18 d9 11 13
03 cb 00 78 01 d9 80 00 0b cb 16 0a 05 dc 15 09
42 d9 00 78 53 de 80 00 5e d2 05 12 5e da 08 14
40 bf 00 78 30 c0 80 00 42 d9 01 03 3a d8 05 07
*/
