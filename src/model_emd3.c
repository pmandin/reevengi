/*
	Load EMD model
	Resident Evil 3

	Copyright (C) 2008-2010	Patrice Mandin

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

#include <assert.h>

#include <SDL.h>

#include "filesystem.h"
#include "video.h"
#include "render.h"
#include "log.h"
#include "render_skel.h"

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
	Uint16	vtx_count;
	Uint16	dummy2;
	Uint16	tri_offset;
	Uint16	dummy3;
	Uint16	quad_offset;
	Uint16	dummy4;
	Uint16	tri_count;
	Uint16	quad_count;
} emd_mesh_object_t;

typedef struct {
	Uint32 length;
	Uint32 num_objects;
} emd_mesh_header_t;

/*--- Functions prototypes ---*/

static render_skel_t *emd_load_render_skel(void *emd_file, Uint32 emd_length, render_texture_t *texture);

static int getChild(render_skel_t *this, int num_parent, int num_child);

/*--- Functions ---*/

render_skel_t *model_emd3_load(void *emd, void *tim, Uint32 emd_length, Uint32 tim_length)
{
	render_texture_t *texture;
	render_skel_t *skel;

	texture = render.createTexture(RENDER_TEXTURE_MUST_POT);
	if (!texture) {
		return NULL;
	}
	texture->load_from_tim(texture, tim);

	skel = emd_load_render_skel(emd, emd_length, texture);
	if (!skel) {
		texture->shutdown(texture);
		return NULL;
	}

	skel->getChild = getChild;

	return skel;
}

static render_skel_t *emd_load_render_skel(void *emd_file, Uint32 emd_length, render_texture_t *texture)
{
	Uint32 *hdr_offsets, skel_offset, mesh_offset;
	int i,j;
	emd_skel_header_t *emd_skel_header;
	emd_skel_relpos_t *emd_skel_relpos;
	emd_skel_data_t *emd_skel_data;
	emd_mesh_header_t *emd_mesh_header;
	emd_mesh_object_t *emd_mesh_object;
	emd_header_t *emd_header;

	render_skel_t *skeleton;

	/* Directory offsets */
	emd_header = (emd_header_t *) emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) emd_file)[SDL_SwapLE32(emd_header->offset)]);

	/* Offset 3: Skeleton */
	skel_offset = SDL_SwapLE32(hdr_offsets[EMD_SKELETON]);

	emd_skel_header = (emd_skel_header_t *)
		(&((char *) emd_file)[skel_offset]);
	emd_skel_relpos = (emd_skel_relpos_t *)
		(&((char *) emd_file)[skel_offset+sizeof(emd_skel_header_t)]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) emd_file)[skel_offset+SDL_SwapLE16(emd_skel_header->relpos_offset)]);

	skeleton = render.createSkel(emd_file, emd_length, texture);
	if (!skeleton) {
		fprintf(stderr, "Can not create skeleton\n");
		return NULL;
	}

	/* Offset 14: Mesh data */
	mesh_offset = SDL_SwapLE32(hdr_offsets[EMD_MESHES]);
	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[mesh_offset]);

	mesh_offset += sizeof(emd_mesh_header_t);
	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[mesh_offset]);

	for (i=0; i<SDL_SwapLE32(emd_mesh_header->num_objects); i++) {
		emd_vertex_t *emd_tri_vtx;
		emd_triangle_t *emd_tri_idx;
		emd_quad_t *emd_quad_idx;
		Uint16 *txcoordPtr, *txcoords;
		int num_tri, num_quad, num_tx, start_tx;

		render_mesh_t *mesh = render.createMesh(texture);
		if (!mesh) {
			fprintf(stderr, "Can not create mesh\n");
			break;
		}

		num_tri = SDL_SwapLE16(emd_mesh_object->tri_count);
		if (num_tri == SDL_SwapLE16(emd_mesh_object->tri_offset)) {
			num_tri = 0;
		}
		num_quad = SDL_SwapLE16(emd_mesh_object->quad_count);
		if (num_quad > 0x100) {
			num_quad = 0;
		}

		/* Vertex array */
		emd_tri_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE16(emd_mesh_object->vtx_offset)]);

		mesh->setArray(mesh, RENDER_ARRAY_VERTEX, 3, RENDER_ARRAY_SHORT,
			SDL_SwapLE16(emd_mesh_object->vtx_count), sizeof(emd_vertex_t),
			emd_tri_vtx,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			1
#else
			0
#endif
			);

#if 0
		/* Normal array */
		emd_tri_nor = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE16(emd_mesh_object->nor_offset)]);

		mesh->setArray(mesh, RENDER_ARRAY_NORMAL, 3, RENDER_ARRAY_SHORT,
			SDL_SwapLE16(emd_mesh_object->vtx_count), sizeof(emd_vertex_t),
			norPtr,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			1
#else
			0
#endif
			);
#endif

		/* Texcoord array */
		num_tx = num_tri*3
			+ num_quad*4;

		txcoordPtr = (Uint16 *) malloc(2*sizeof(Uint16)*num_tx);
		if (!txcoordPtr) {
			fprintf(stderr, "Can not allocate memory for txcoords\n");
			mesh->shutdown(mesh);
			break;
		}
		txcoords = txcoordPtr;

		emd_tri_idx = (emd_triangle_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE16(emd_mesh_object->tri_offset)]);

		for (j=0; j<num_tri; j++) {
			int page = (emd_tri_idx[j].page & 0xff)<<1;

			*txcoords++ = emd_tri_idx[j].tu0 + page;
			*txcoords++ = emd_tri_idx[j].tv0;
			*txcoords++ = emd_tri_idx[j].tu1 + page;
			*txcoords++ = emd_tri_idx[j].tv1;
			*txcoords++ = emd_tri_idx[j].tu2 + page;
			*txcoords++ = emd_tri_idx[j].tv2;
		}

		emd_quad_idx = (emd_quad_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE16(emd_mesh_object->quad_offset)]);

		for (j=0; j<num_quad; j++) {
			int page = (emd_quad_idx[j].page & 0xff)<<1;

			*txcoords++ = emd_quad_idx[j].tu0 + page;
			*txcoords++ = emd_quad_idx[j].tv0;
			*txcoords++ = emd_quad_idx[j].tu1 + page;
			*txcoords++ = emd_quad_idx[j].tv1;
			*txcoords++ = emd_quad_idx[j].tu2 + page;
			*txcoords++ = emd_quad_idx[j].tv2;
			*txcoords++ = emd_quad_idx[j].tu3 + page;
			*txcoords++ = emd_quad_idx[j].tv3;
		}

		mesh->setArray(mesh, RENDER_ARRAY_TEXCOORD, 2, RENDER_ARRAY_SHORT,
			num_tx, sizeof(Uint16)*2,
			txcoordPtr, 0);

		free(txcoordPtr);

		/* Triangles */
		for (j=0; j<num_tri; j++) {
			render_mesh_tri_t	mesh_tri;
			
			mesh_tri.v[0] = emd_tri_idx[j].v0;
			mesh_tri.v[1] = emd_tri_idx[j].v1;
			mesh_tri.v[2] = emd_tri_idx[j].v2;

			/*mesh_tri.n[0] = emd_tri_idx[j].n0;
			mesh_tri.n[1] = emd_tri_idx[j].n1;
			mesh_tri.n[2] = emd_tri_idx[j].n2;*/

			mesh_tri.tx[0] = j*3;
			mesh_tri.tx[1] = j*3+1;
			mesh_tri.tx[2] = j*3+2;

			mesh_tri.txpal = emd_tri_idx[j].clutid & 3;

			mesh->addTriangle(mesh, &mesh_tri);
		}

		/* Quads */
		start_tx = num_tri*3;

		for (j=0; j<num_quad; j++) {
			render_mesh_quad_t	mesh_quad;

			mesh_quad.v[0] = emd_quad_idx[j].v0;
			mesh_quad.v[1] = emd_quad_idx[j].v1;
			mesh_quad.v[2] = emd_quad_idx[j].v2;
			mesh_quad.v[3] = emd_quad_idx[j].v3;

			/*mesh_quad.n[0] = emd_quad_idx[j].n0;
			mesh_quad.n[1] = emd_quad_idx[j].n1;
			mesh_quad.n[2] = emd_quad_idx[j].n2;
			mesh_quad.n[3] = emd_quad_idx[j].n3;*/

			mesh_quad.tx[0] = start_tx + j*4;
			mesh_quad.tx[1] = start_tx + j*4+1;
			mesh_quad.tx[2] = start_tx + j*4+2;
			mesh_quad.tx[3] = start_tx + j*4+3;

			mesh_quad.txpal = emd_quad_idx[j].clutid & 3;

			mesh->addQuad(mesh, &mesh_quad);
		}

		/* Add mesh to skeleton */
		skeleton->addMesh(skeleton, mesh,
			SDL_SwapLE16(emd_skel_relpos[i].x),
			SDL_SwapLE16(emd_skel_relpos[i].y),
			SDL_SwapLE16(emd_skel_relpos[i].z));

		emd_mesh_object++;
	}

	return skeleton;
}

static int getChild(render_skel_t *this, int num_parent, int num_child)
{
	Uint32 *hdr_offsets, skel_offset;
	emd_header_t *emd_header;
	emd_skel_header_t *emd_skel_header;
	emd_skel_data_t *emd_skel_data;
	int i, num_meshes;
	Uint8 *mesh_numbers;

	assert(this);
	assert(this->emd_file);
	assert(num_parent>=0);
	assert(num_child>=0);

	emd_header = (emd_header_t *) this->emd_file;

	hdr_offsets = (Uint32 *)
		(&((char *) (this->emd_file))[SDL_SwapLE32(emd_header->offset)]);

	/* Offset 3: Skeleton */
	skel_offset = SDL_SwapLE32(hdr_offsets[EMD_SKELETON]);

	emd_skel_header = (emd_skel_header_t *)
		(&((char *) (this->emd_file))[skel_offset]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) (this->emd_file))[skel_offset+SDL_SwapLE16(emd_skel_header->relpos_offset)]);

	num_meshes = SDL_SwapLE16(emd_skel_data[num_parent].num_mesh);
	if (num_child>=num_meshes) {
		return -1;
	}

	mesh_numbers = (Uint8 *) emd_skel_data;
	return mesh_numbers[SDL_SwapLE16(emd_skel_data[num_parent].offset)+num_child];
}
