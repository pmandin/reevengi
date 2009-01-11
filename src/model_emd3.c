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
#include "log.h"

/*--- Defines ---*/

#define EMD_SKELETON 3
#define EMD_MESHES 14

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
	unsigned char page, dummy1;

	unsigned char tu1,tv1;
	unsigned char clutid, v0;

	unsigned char tu2,tv2;
	unsigned char v1,v2;
} emd_triangle_t;

typedef struct {
	unsigned char tu0,tv0;
	unsigned char page, dummy1;

	unsigned char tu1,tv1;
	unsigned char clutid, dummy3;

	unsigned char tu2,tv2;
	unsigned char v0,v1;

	unsigned char tu3,tv3;
	unsigned char v2,v3;
} emd_quad_t;

typedef struct {
	Uint16	vtx_offset;
	Uint16	dummy0;
	Uint16	nor_offset;
	Uint16	dummy1;
	Uint8	vtx_count;
	Uint8	dummy2[3];
	Uint16	tri_offset;
	Uint16	dummy3;
	Uint16	quad_offset;
	Uint16	dummy4;
	Uint8	tri_count;
	Uint8	dummy5;
	Uint8	quad_count;
	Uint8	dummy6;
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

model_t *model_emd3_load(void *emd, void *tim, Uint32 emd_length, Uint32 tim_length)
{
	model_t	*model;

	model = (model_t *) calloc(1, sizeof(model_t));
	if (!model) {
		fprintf(stderr, "Can not allocate memory for model\n");
		return NULL;
	}

	model->emd_file = emd;
	model->emd_length = emd_length;

	model->tim_file = tim;
	model->tim_length = tim_length;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	emd_convert_endianness(model);
#endif

	model->texture = render_texture_load_from_tim(model->tim_file);

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
	int num_objects, i;
	emd_vertex_t *emd_tri_vtx, *emd_quad_vtx;
	emd_triangle_t *emd_tri_idx;
	emd_quad_t *emd_quad_idx;
	void *emd_file = this->emd_file;
	vertex_t v[4];

	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[emd_header->offset]);

	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[hdr_offsets[EMD_MESHES]]);
	num_objects = emd_mesh_header->num_objects;

	if ((num_mesh<0) || (num_mesh>=num_objects)) {
		logMsg(3, "Invalid mesh %d\n", num_mesh);
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
		int page = emd_tri_idx[i].page;
		/*printf("d: 0x%04x 0x%04x 0x%04x\n", emd_tri_idx[i].dummy0, emd_tri_idx[i].dummy1, emd_tri_idx[i].dummy2);*/

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

		render.set_texture(emd_tri_idx[i].clutid & 3, this->texture);
		render.triangle_tex(&v[0], &v[1], &v[2]);
	}

	/* Draw quads */
	emd_quad_vtx = (emd_vertex_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->vtx_offset]);
	emd_quad_idx = (emd_quad_t *)
		(&((char *) emd_file)[mesh_offset+emd_mesh_object->quad_offset]);

	for (i=0; i<emd_mesh_object->quad_count; i++) {
		int page = emd_quad_idx[i].page;

		v[0].x = emd_quad_vtx[emd_quad_idx[i].v0].x;
		v[0].y = emd_quad_vtx[emd_quad_idx[i].v0].y;
		v[0].z = emd_quad_vtx[emd_quad_idx[i].v0].z;
		v[0].u = emd_quad_idx[i].tu0 + page;
		v[0].v = emd_quad_idx[i].tv0;

		v[1].x = emd_quad_vtx[emd_quad_idx[i].v1].x;
		v[1].y = emd_quad_vtx[emd_quad_idx[i].v1].y;
		v[1].z = emd_quad_vtx[emd_quad_idx[i].v1].z;
		v[1].u = emd_quad_idx[i].tu1 + page;
		v[1].v = emd_quad_idx[i].tv1;

		v[2].x = emd_quad_vtx[emd_quad_idx[i].v2].x;
		v[2].y = emd_quad_vtx[emd_quad_idx[i].v2].y;
		v[2].z = emd_quad_vtx[emd_quad_idx[i].v2].z;
		v[2].u = emd_quad_idx[i].tu2 + page;
		v[2].v = emd_quad_idx[i].tv2;

		v[3].x = emd_quad_vtx[emd_quad_idx[i].v3].x;
		v[3].y = emd_quad_vtx[emd_quad_idx[i].v3].y;
		v[3].z = emd_quad_vtx[emd_quad_idx[i].v3].z;
		v[3].u = emd_quad_idx[i].tu3;
		v[3].v = emd_quad_idx[i].tv3;

		render.set_texture(emd_quad_idx[i].clutid & 3, this->texture);
		render.quad_tex(&v[0], &v[1], &v[3], &v[2]);
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
		void **list_done;

		/* Mesh */
		emd_mesh_object->vtx_offset = SDL_SwapLE16(emd_mesh_object->vtx_offset);
		emd_mesh_object->nor_offset = SDL_SwapLE16(emd_mesh_object->nor_offset);
		emd_mesh_object->tri_offset = SDL_SwapLE16(emd_mesh_object->tri_offset);
		emd_mesh_object->quad_offset = SDL_SwapLE16(emd_mesh_object->quad_offset);

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
