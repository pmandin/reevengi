/*
	Load EMD model
	Resident Evil 2

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
#include "render_skel.h"
#include "log.h"

/*--- Defines ---*/

#define EMD_SKELETON 2
#define EMD_MESHES 7

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
	Uint16	n0,v0;
	Uint16	n1,v1;
	Uint16	n2,v2;
} emd_triangle_t;

typedef struct {
	unsigned char u0,v0;
	Uint16 page;
	unsigned char u1,v1;
	Uint16 clutid;
	unsigned char u2,v2;
	Uint16 dummy;
} emd_triangle_tex_t;

typedef struct {
	Uint16	n0,v0;
	Uint16	n1,v1;
	Uint16	n2,v2;
	Uint16	n3,v3;
} emd_quad_t;

typedef struct {
	unsigned char u0,v0;
	Uint16 page;
	unsigned char u1,v1;
	Uint16 clutid;
	unsigned char u2,v2;
	Uint16 dummy0;
	unsigned char u3,v3;
	Uint16 dummy1;
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

static render_skel_t *emd_load_render_skel(void *emd_file, render_texture_t *texture);

static void emd_load_render_skel_hierarchy(render_skel_t *skel, emd_skel_data_t *skel_data,
	int num_mesh);

/*--- Functions ---*/

render_skel_t *model_emd2_load(void *emd, void *tim, Uint32 emd_length, Uint32 tim_length)
{
	render_texture_t *texture;
	render_skel_t *skel;

	texture = render.createTexture(RENDER_TEXTURE_MUST_POT);
	if (!texture) {
		return NULL;
	}
	texture->load_from_tim(texture, tim);

	skel = emd_load_render_skel(emd, texture);
	if (!skel) {
		texture->shutdown(texture);
		return NULL;
	}

	return skel;
}

static render_skel_t *emd_load_render_skel(void *emd_file, render_texture_t *texture)
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

	/* Offset 2: Skeleton */
	skel_offset = SDL_SwapLE32(hdr_offsets[EMD_SKELETON]);

	emd_skel_header = (emd_skel_header_t *)
		(&((char *) emd_file)[skel_offset]);
	emd_skel_relpos = (emd_skel_relpos_t *)
		(&((char *) emd_file)[skel_offset+sizeof(emd_skel_header_t)]);
	emd_skel_data = (emd_skel_data_t *)
		(&((char *) emd_file)[skel_offset+SDL_SwapLE16(emd_skel_header->relpos_offset)]);

	skeleton = render.createSkel(emd_file, texture);
	if (!skeleton) {
		fprintf(stderr, "Can not create skeleton\n");
		return NULL;
	}

	/* Offset 7: Mesh data */
	mesh_offset = SDL_SwapLE32(hdr_offsets[EMD_MESHES]);
	emd_mesh_header = (emd_mesh_header_t *)
		(&((char *) emd_file)[mesh_offset]);

	mesh_offset += sizeof(emd_mesh_header_t);
	emd_mesh_object = (emd_mesh_object_t *)
		(&((char *) emd_file)[mesh_offset]);

	for (i=0; i<SDL_SwapLE32(emd_mesh_header->num_objects)>>1; i++) {
		emd_vertex_t *emd_tri_vtx, *emd_quad_vtx;
		emd_vertex_t *emd_tri_nor, *emd_quad_nor;
		emd_triangle_t *emd_tri_idx;
		emd_quad_t *emd_quad_idx;
		Uint16 *txcoordPtr, *txcoords;
		Uint16 *vtxPtr, *curVtx;
		Uint16 *norPtr, *curNor;
		int num_vtx, num_tx, start_tx, start_vtx, start_nor;
		emd_triangle_tex_t *emd_tri_tex;
		emd_quad_tex_t *emd_quad_tex;

		render_mesh_t *mesh = render.createMesh(texture);
		if (!mesh) {
			fprintf(stderr, "Can not create mesh\n");
			break;
		}

		/* Vertex array */
		emd_tri_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->triangles.vtx_offset)]);
		emd_quad_vtx = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->quads.vtx_offset)]);

		num_vtx = SDL_SwapLE32(emd_mesh_object->triangles.vtx_count) +
			SDL_SwapLE32(emd_mesh_object->quads.vtx_count);

		vtxPtr = (Uint16 *) malloc(3*sizeof(Uint16)*num_vtx);
		if (!vtxPtr) {
			fprintf(stderr, "Can not allocate memory for vertex array\n");
			mesh->shutdown(mesh);
			break;
		}

		curVtx = vtxPtr;
		for (j=0; j<SDL_SwapLE32(emd_mesh_object->triangles.vtx_count); j++) {
			*curVtx++ = SDL_SwapLE16(emd_tri_vtx[j].x);
			*curVtx++ = SDL_SwapLE16(emd_tri_vtx[j].y);
			*curVtx++ = SDL_SwapLE16(emd_tri_vtx[j].z);
		}
		for (j=0; j<SDL_SwapLE32(emd_mesh_object->quads.vtx_count); j++) {
			*curVtx++ = SDL_SwapLE16(emd_quad_vtx[j].x);
			*curVtx++ = SDL_SwapLE16(emd_quad_vtx[j].y);
			*curVtx++ = SDL_SwapLE16(emd_quad_vtx[j].z);
		}

		mesh->setArray(mesh, RENDER_ARRAY_VERTEX, 3, RENDER_ARRAY_SHORT,
			num_vtx, 3*sizeof(Uint16),
			vtxPtr, 0);

		free(vtxPtr);

		/* Normal array */
		emd_tri_nor = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->triangles.nor_offset)]);
		emd_quad_nor = (emd_vertex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->quads.nor_offset)]);

		num_vtx = SDL_SwapLE32(emd_mesh_object->triangles.nor_count) +
			SDL_SwapLE32(emd_mesh_object->quads.nor_count);

		norPtr = (Uint16 *) malloc(3*sizeof(Uint16)*num_vtx);
		if (!norPtr) {
			fprintf(stderr, "Can not allocate memory for normal array\n");
			mesh->shutdown(mesh);
			break;
		}

		curNor = norPtr;
		for (j=0; j<SDL_SwapLE32(emd_mesh_object->triangles.nor_count); j++) {
			*curNor++ = SDL_SwapLE16(emd_tri_nor[j].x);
			*curNor++ = SDL_SwapLE16(emd_tri_nor[j].y);
			*curNor++ = SDL_SwapLE16(emd_tri_nor[j].z);
		}
		for (j=0; j<SDL_SwapLE32(emd_mesh_object->quads.nor_count); j++) {
			*curNor++ = SDL_SwapLE16(emd_quad_nor[j].x);
			*curNor++ = SDL_SwapLE16(emd_quad_nor[j].y);
			*curNor++ = SDL_SwapLE16(emd_quad_nor[j].z);
		}

		mesh->setArray(mesh, RENDER_ARRAY_NORMAL, 3, RENDER_ARRAY_SHORT,
			num_vtx, 3*sizeof(Uint16),
			norPtr, 0);

		free(norPtr);

		/* Texcoord array */
		num_tx = SDL_SwapLE32(emd_mesh_object->triangles.mesh_count)*3
			+ SDL_SwapLE32(emd_mesh_object->quads.mesh_count)*4;

		txcoordPtr = (Uint16 *) malloc(2*sizeof(Uint16)*num_tx);
		if (!txcoordPtr) {
			fprintf(stderr, "Can not allocate memory for txcoords\n");
			mesh->shutdown(mesh);
			break;
		}
		txcoords = txcoordPtr;

		emd_tri_tex = (emd_triangle_tex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->triangles.tex_offset)]);

		for (j=0; j<SDL_SwapLE32(emd_mesh_object->triangles.mesh_count); j++) {
			int page = (SDL_SwapLE16(emd_tri_tex[j].page) & 0xff)<<1;

			*txcoords++ = emd_tri_tex[j].u0 + page;
			*txcoords++ = emd_tri_tex[j].v0;
			*txcoords++ = emd_tri_tex[j].u1 + page;
			*txcoords++ = emd_tri_tex[j].v1;
			*txcoords++ = emd_tri_tex[j].u2 + page;
			*txcoords++ = emd_tri_tex[j].v2;
		}

		emd_quad_tex = (emd_quad_tex_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->quads.tex_offset)]);

		for (j=0; j<SDL_SwapLE32(emd_mesh_object->quads.mesh_count); j++) {
			int page = (SDL_SwapLE16(emd_quad_tex[j].page) & 0xff)<<1;

			*txcoords++ = emd_quad_tex[j].u0 + page;
			*txcoords++ = emd_quad_tex[j].v0;
			*txcoords++ = emd_quad_tex[j].u1 + page;
			*txcoords++ = emd_quad_tex[j].v1;
			*txcoords++ = emd_quad_tex[j].u2 + page;
			*txcoords++ = emd_quad_tex[j].v2;
			*txcoords++ = emd_quad_tex[j].u3 + page;
			*txcoords++ = emd_quad_tex[j].v3;
		}

		mesh->setArray(mesh, RENDER_ARRAY_TEXCOORD, 2, RENDER_ARRAY_SHORT,
			num_tx, sizeof(Uint16)*2,
			txcoordPtr, 0);

		free(txcoordPtr);

		/* Triangles */
		emd_tri_idx = (emd_triangle_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->triangles.mesh_offset)]);

		for (j=0; j<SDL_SwapLE32(emd_mesh_object->triangles.mesh_count); j++) {
			render_mesh_tri_t	mesh_tri;
			
			mesh_tri.v[0] = SDL_SwapLE16(emd_tri_idx[j].v0);
			mesh_tri.v[1] = SDL_SwapLE16(emd_tri_idx[j].v1);
			mesh_tri.v[2] = SDL_SwapLE16(emd_tri_idx[j].v2);

			mesh_tri.n[0] = SDL_SwapLE16(emd_tri_idx[j].n0);
			mesh_tri.n[1] = SDL_SwapLE16(emd_tri_idx[j].n1);
			mesh_tri.n[2] = SDL_SwapLE16(emd_tri_idx[j].n2);

			mesh_tri.tx[0] = j*3;
			mesh_tri.tx[1] = j*3+1;
			mesh_tri.tx[2] = j*3+2;

			mesh_tri.txpal = SDL_SwapLE16(emd_tri_tex[j].clutid) & 3;

			mesh->addTriangle(mesh, &mesh_tri);
		}

		/* Quads */
		start_vtx = SDL_SwapLE32(emd_mesh_object->triangles.vtx_count);
		start_nor = SDL_SwapLE32(emd_mesh_object->triangles.nor_count);
		start_tx = SDL_SwapLE32(emd_mesh_object->triangles.mesh_count)*3;

		emd_quad_idx = (emd_quad_t *)
			(&((char *) emd_file)[mesh_offset+SDL_SwapLE32(emd_mesh_object->quads.mesh_offset)]);

		for (j=0; j<SDL_SwapLE32(emd_mesh_object->quads.mesh_count); j++) {
			render_mesh_quad_t	mesh_quad;

			mesh_quad.v[0] = start_vtx + SDL_SwapLE16(emd_quad_idx[j].v0);
			mesh_quad.v[1] = start_vtx + SDL_SwapLE16(emd_quad_idx[j].v1);
			mesh_quad.v[2] = start_vtx + SDL_SwapLE16(emd_quad_idx[j].v2);
			mesh_quad.v[3] = start_vtx + SDL_SwapLE16(emd_quad_idx[j].v3);

			mesh_quad.n[0] = start_nor + SDL_SwapLE16(emd_quad_idx[j].n0);
			mesh_quad.n[1] = start_nor + SDL_SwapLE16(emd_quad_idx[j].n1);
			mesh_quad.n[2] = start_nor + SDL_SwapLE16(emd_quad_idx[j].n2);
			mesh_quad.n[3] = start_nor + SDL_SwapLE16(emd_quad_idx[j].n3);

			mesh_quad.tx[0] = start_tx + j*4;
			mesh_quad.tx[1] = start_tx + j*4+1;
			mesh_quad.tx[2] = start_tx + j*4+2;
			mesh_quad.tx[3] = start_tx + j*4+3;

			mesh_quad.txpal = SDL_SwapLE16(emd_quad_tex[j].clutid) & 3;

			mesh->addQuad(mesh, &mesh_quad);
		}

		/* Add mesh to skeleton */
		skeleton->addMesh(skeleton, mesh,
			SDL_SwapLE16(emd_skel_relpos[i].x),
			SDL_SwapLE16(emd_skel_relpos[i].y),
			SDL_SwapLE16(emd_skel_relpos[i].z));

		emd_mesh_object++;
	}

	/* Define hierarchy */
	emd_load_render_skel_hierarchy(skeleton, emd_skel_data, 0);

	return skeleton;
}

static void emd_load_render_skel_hierarchy(render_skel_t *skel, emd_skel_data_t *skel_data,
	int num_mesh)
{
	int i;
	Uint8 *mesh_numbers = (Uint8 *) skel_data;

	for (i=0; i<SDL_SwapLE16(skel_data[num_mesh].num_mesh); i++) {
		int child = mesh_numbers[SDL_SwapLE16(skel_data[num_mesh].offset)+i];

		if (num_mesh != child) {
			skel->setParent(skel, num_mesh, child);

			emd_load_render_skel_hierarchy(skel, skel_data, child);
		}
	}
}
