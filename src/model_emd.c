/*
	Load EMD model
	Resident Evil

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
#include "model_emd.h"

/*--- Defines ---*/

#define EMD_SKELETON 0
#define EMD_MESHES 2
#define EMD_TIM 3

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
	Uint32 id;

	unsigned char tu0,tv0;
	Uint16 clutid;
	unsigned char tu1,tv1;
	Uint16 page;
	unsigned char tu2,tv2;
	Uint16 dummy;

	Uint16	n0,v0;
	Uint16	n1,v1;
	Uint16	n2,v2;
} emd_triangle_t;

typedef struct {
	Uint32	vtx_offset;
	Uint32	vtx_count;
	Uint32	nor_offset;
	Uint32	nor_count;
	Uint32	mesh_offset;
	Uint32	mesh_count;
	Uint32	dummy;
} emd_mesh_t;

typedef struct {
	Uint32 length;
	Uint32 dummy;
	Uint32 num_objects;
} emd_mesh_header_t;

typedef struct {
	emd_mesh_t triangles;
} emd_mesh_object_t;

/*--- Functions prototypes ---*/

static void model_emd_shutdown(model_t *this);
static void model_emd_draw(model_t *this);

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

model_t *model_emd_load(void *emd, Uint32 emd_length)
{
	model_t	*model;
	Uint32 *hdr_offsets;
	
	model = (model_t *) calloc(1, sizeof(model_t));
	if (!model) {
		fprintf(stderr, "Can not allocate memory for model\n");
		return NULL;
	}

	model->emd_file = emd;
	model->emd_length = emd_length;

	/* TIM file embedded */
	hdr_offsets = (Uint32 *)
		(&((char *) model->emd_file)[model->emd_length-16]);

	model->tim_file = (&((char *) model->emd_file)[hdr_offsets[EMD_TIM]]);
	model->tim_length = model->emd_length - hdr_offsets[EMD_TIM];

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	emd_convert_endianness(model);
#endif

	model->shutdown = model_emd_shutdown;
	model->draw = model_emd_draw;

	return model;
}

static void model_emd_shutdown(model_t *this)
{
	if (this) {
		if (this->emd_file) {
			free(this->emd_file);
		}
		free(this);
	}
}

static void model_emd_draw(model_t *this)
{
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

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[this->emd_length-16]);

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
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;
	Uint32 *hdr_offsets, mesh_offset;
	int num_objects, i;
	emd_vertex_t *emd_tri_vtx;
	emd_triangle_t *emd_tri_idx;
	void *emd_file = this->emd_file;
	vertex_t v[3];

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[this->emd_length-16]);

	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_MESHES]]);
	num_objects = emd_mesh_header->num_objects;

	if ((num_mesh<0) || (num_mesh>=num_objects)) {
		fprintf(stderr, "Invalid mesh %d/%d\n", num_mesh, num_objects);
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
		int page = emd_tri_idx[i].page << 6;

		v[0].x = emd_tri_vtx[emd_tri_idx[i].v0].x;
		v[0].y = emd_tri_vtx[emd_tri_idx[i].v0].y;
		v[0].z = emd_tri_vtx[emd_tri_idx[i].v0].z;
		v[0].u = emd_tri_idx[i].tu0 + page;
		v[0].v = emd_tri_idx[i].tv0;

		v[1].x = emd_tri_vtx[emd_tri_idx[i].v1].x;
		v[1].y = emd_tri_vtx[emd_tri_idx[i].v1].y;
		v[1].z = emd_tri_vtx[emd_tri_idx[i].v1].z;
		v[1].u = emd_tri_idx[i].tu1 + page;
		v[1].v = emd_tri_idx[i].tv1;

		v[2].x = emd_tri_vtx[emd_tri_idx[i].v2].x;
		v[2].y = emd_tri_vtx[emd_tri_idx[i].v2].y;
		v[2].z = emd_tri_vtx[emd_tri_idx[i].v2].z;
		v[2].u = emd_tri_idx[i].tu2 + page;
		v[2].v = emd_tri_idx[i].tv2;

		render.triangle(&v[0], &v[1], &v[2]);
	}
}

/*--- Convert EMD file (little endian) to big endian ---*/

static void emd_convert_endianness(model_t *this)
{
	Uint32 *hdr_offsets, mesh_offset;
	int i;
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_skel_data_t *emd_skel_data;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;
	void *emd_file = this->emd_file;

	/* Directory offsets */
	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[this->emd_length-16]);
	for (i=0; i<4; i++) {
		hdr_offsets[i] = SDL_SwapLE32(hdr_offsets[i]);
	}

	/* Offset 0: Skeleton */
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

	/* Offset 2: Mesh data */
	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_MESHES]]);
	emd_mesh_header->length = SDL_SwapLE32(emd_mesh_header->length);
	emd_mesh_header->dummy = SDL_SwapLE32(emd_mesh_header->dummy);
	emd_mesh_header->num_objects = SDL_SwapLE32(emd_mesh_header->num_objects);

	mesh_offset = hdr_offsets[EMD_MESHES]+sizeof(emd_mesh_header_t);
	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[mesh_offset]);
	for (i=0; i<emd_mesh_header->num_objects; i++) {
		int j;
		emd_vertex_t *emd_vtx;
		emd_triangle_t *emd_tri_idx;
		void **list_done;

		/* Triangles */
		emd_mesh_object->triangles.vtx_offset = SDL_SwapLE32(emd_mesh_object->triangles.vtx_offset);
		emd_mesh_object->triangles.vtx_count = SDL_SwapLE32(emd_mesh_object->triangles.vtx_count);
		emd_mesh_object->triangles.nor_offset = SDL_SwapLE32(emd_mesh_object->triangles.nor_offset);
		emd_mesh_object->triangles.nor_count = SDL_SwapLE32(emd_mesh_object->triangles.nor_count);
		emd_mesh_object->triangles.mesh_offset = SDL_SwapLE32(emd_mesh_object->triangles.mesh_offset);
		emd_mesh_object->triangles.mesh_count = SDL_SwapLE32(emd_mesh_object->triangles.mesh_count);

		list_done = malloc(sizeof(void *)*
			(emd_mesh_object->triangles.vtx_count+
			emd_mesh_object->triangles.nor_count));
		if (!list_done) {
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
			list_done[j] = &emd_vtx[j];
		}

		emd_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+emd_mesh_object->triangles.nor_offset]);
		for (j=0; j<emd_mesh_object->triangles.nor_count; j++) {
			emd_vtx[j].x = SDL_SwapLE16(emd_vtx[j].x);
			emd_vtx[j].y = SDL_SwapLE16(emd_vtx[j].y);
			emd_vtx[j].z = SDL_SwapLE16(emd_vtx[j].z);
			emd_vtx[j].w = SDL_SwapLE16(emd_vtx[j].w);
			list_done[emd_mesh_object->triangles.vtx_count+j] = &emd_vtx[j];
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

			emd_tri_idx[j].clutid = SDL_SwapLE16(emd_tri_idx[j].clutid);
			emd_tri_idx[j].page = SDL_SwapLE16(emd_tri_idx[j].page);
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
