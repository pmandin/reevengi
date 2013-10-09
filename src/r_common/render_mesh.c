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

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "../log.h"
#include "../parameters.h"

#include "r_misc.h"
#include "render.h"
#include "render_mesh.h"
#include "render_texture.h"

/*--- Functions prototypes ---*/

static void shutdown(render_mesh_t *this);

static void upload(render_mesh_t *this);
static void download(render_mesh_t *this);

static void setArray(render_mesh_t *this, int array_type, int components, int type,
	int items, int stride, void *data, int byteswap);
static void addTriangle(render_mesh_t *this, render_mesh_tri_t *tri);
static void addQuad(render_mesh_t *this, render_mesh_quad_t *quad);

/*static void markTrans(render_mesh_t *this, Uint16 num_pal, Uint16 *start_idx, Uint16 count);*/

static int checkHasAlpha(render_mesh_t *this, Uint16 num_pal, Uint16 *start_idx, Uint16 count);

static void draw(render_mesh_t *this);

/*--- Functions ---*/

render_mesh_t *render_mesh_create(render_texture_t *texture)
{
	render_mesh_t *mesh;

	mesh = calloc(1, sizeof(render_mesh_t));
	if (!mesh) {
		fprintf(stderr, "Can not allocate memory for mesh\n");
		return NULL;
	}

	mesh->shutdown = shutdown;

	mesh->upload = upload;
	mesh->download = download;

	mesh->setArray = setArray;
	mesh->addTriangle = addTriangle;
	mesh->addQuad = addQuad;
	mesh->draw = draw;

	mesh->texture = texture;

	logMsg(3, "render_mesh: mesh 0x%p created\n", mesh);

	return mesh;
}

static void shutdown(render_mesh_t *this)
{
	if (!this) {
		return;
	}

	if (this->vertex.data) {
		free(this->vertex.data);
	}
	if (this->normal.data) {
		free(this->normal.data);
	}
	if (this->texcoord.data) {
		free(this->texcoord.data);
	}

	if (this->triangles) {
		free(this->triangles);
	}
	if (this->quads) {
		free(this->quads);
	}

	logMsg(3, "render_mesh: mesh 0x%p destroyed\n", this);

	free(this);
}

static void upload(render_mesh_t *this)
{
}

static void download(render_mesh_t *this)
{
}

static void setArray(render_mesh_t *this, int array_type, int components, int type,
	int items, int stride, void *data, int byteswap)
{
	render_mesh_array_t *array = NULL;
	void *new_data;
	int i,j, item_size = components, dst_type = type;
	int src_comp_size = 1;
	Uint8 *src;
	Sint16 *dst;

	if (type == RENDER_ARRAY_BYTE) {
		dst_type = RENDER_ARRAY_SHORT;
	}
	if (type == RENDER_ARRAY_SHORT) {
		item_size <<= 1;
		src_comp_size = 2;
	}

	switch(array_type) {
		case RENDER_ARRAY_VERTEX:
			array = &(this->vertex);
			break;
		case RENDER_ARRAY_NORMAL:
			array = &(this->normal);
			break;
		case RENDER_ARRAY_TEXCOORD:
			array = &(this->texcoord);
			break;
		default:
			fprintf(stderr, "Invalid array %d\n", array_type);
			break;
	}

	new_data = realloc(array->data, item_size * items);
	if (!new_data) {
		fprintf(stderr, "render_mesh: Can not allocate %d bytes of memory for new array data\n", item_size * items);
		return;
	}

	array->data = new_data;

	array->stride = item_size;
	array->items = items;

	array->type = dst_type;
	array->components = components;

	/* Copy array data */
	src = (Uint8 *) data;
	dst = (Sint16 *) array->data;
	for (i=0; i<items; i++) {
		Uint8 *srcItem = src;
		for (j=0; j<components; j++) {
			Uint16 srcValue;

			switch(type) {
				case RENDER_ARRAY_BYTE:
					*dst++ = ((Sint16) *srcItem) & 255;
					break;
				case RENDER_ARRAY_SHORT:
					srcValue = * ((Sint16 *)srcItem);
					if (byteswap) {
						srcValue = SDL_SwapLE16(srcValue);
					}
					*dst++ = srcValue;
					break;
			}

			srcItem += src_comp_size;
		}

		src += stride;
	}
}

static void addTriangle(render_mesh_t *this, render_mesh_tri_t *tri)
{
	render_mesh_tri_t *new_tris, *new_tri;
	int num_tris = this->num_tris + 1;

	new_tris = realloc(this->triangles, num_tris * sizeof(render_mesh_tri_t));
	if (!new_tris) {
		return;
	}

	new_tri = &(new_tris[this->num_tris]);
	memcpy(new_tri, tri, sizeof(render_mesh_tri_t));
	new_tri->has_alpha = checkHasAlpha(this, new_tri->txpal, new_tri->tx, 3);

	this->num_tris++;
	this->triangles = new_tris;

	/* TODO: sort per texture palette index */
}

static void addQuad(render_mesh_t *this, render_mesh_quad_t *quad)
{
	render_mesh_quad_t *new_quads, *new_quad;
	int num_quads = this->num_quads + 1;

	new_quads = realloc(this->quads, num_quads * sizeof(render_mesh_quad_t));
	if (!new_quads) {
		return;
	}

	new_quad = &(new_quads[this->num_quads]);
	memcpy(new_quad, quad, sizeof(render_mesh_quad_t));
	new_quad->has_alpha = checkHasAlpha(this, new_quad->txpal, new_quad->tx, 4);

	this->num_quads++;
	this->quads = new_quads;

	/* TODO: sort per texture palette index */
}

#if 0
static void markTrans(render_mesh_t *this, Uint16 num_pal, Uint16 *start_idx, Uint16 count)
{
	int j, u,v, minx,maxx,miny,maxy;

	if (params.use_opengl || !this->texcoord.data || !this->texture) {
		return;
	}

	minx = miny = 32768;
	maxx = maxy = 0;

	switch(this->texcoord.type) {
		case RENDER_ARRAY_BYTE:
			{
				Uint8 *src = (Uint8 *) this->texcoord.data;
				for (j=0; j<count; j++) {
					u = src[(start_idx[j]*this->texcoord.stride)+0];
					v = src[(start_idx[j]*this->texcoord.stride)+1];
					minx = MIN(minx, u);
					maxx = MAX(maxx, u);
					miny = MIN(miny, v);
					maxy = MAX(maxy, v);
				}
			}
			break;
		case RENDER_ARRAY_SHORT:
			{
				Sint16 *src = (Sint16 *) this->texcoord.data;
				for (j=0; j<count; j++) {
					u = src[(start_idx[j]*(this->texcoord.stride>>1))+0];
					v = src[(start_idx[j]*(this->texcoord.stride>>1))+1];
					minx = MIN(minx, u);
					maxx = MAX(maxx, u);
					miny = MIN(miny, v);
					maxy = MAX(maxy, v);
				}
			}
			break;
	}

	this->texture->mark_trans(this->texture, num_pal, minx,miny, maxx,maxy);
}
#endif

/* Check if a portion of texture has any transparent zone */
static int checkHasAlpha(render_mesh_t *this, Uint16 num_pal, Uint16 *start_idx, Uint16 count)
{
	int has_alpha = 0, y;
	int p1,p2, /*minx,maxx,*/ miny,maxy;
	int *poly_x1, *poly_x2;	/* min and max for each texture line */
	render_texture_t *tex;
	Uint8 *alpha_pal, *src_line;

	if (params.use_opengl) {
		return 1;
	}
	
	if (!this->texcoord.data || !this->texture) {
		return 0;
	}

	/*minx = this->texture->w-1;
	maxx = 0;*/
	miny = this->texture->h-1;
	maxy = 0;

	poly_x1 = calloc(this->texture->h * 2, sizeof(int));
	if (!poly_x1) {
		return 0;
	}
	poly_x2 = &poly_x1[this->texture->h];

	p1 = count-1;
	for(p2=0; p2<count; p2++) {
		int u1=0,v1=0, u2=0,v2=0, dv, du;

		switch(this->texcoord.type) {
			case RENDER_ARRAY_BYTE:
				{
					Uint8 *src = (Uint8 *) this->texcoord.data;
					u1 = src[(start_idx[p1]*this->texcoord.stride)+0];
					v1 = src[(start_idx[p1]*this->texcoord.stride)+1];
					u2 = src[(start_idx[p2]*this->texcoord.stride)+0];
					v2 = src[(start_idx[p2]*this->texcoord.stride)+1];
				}
				break;
			case RENDER_ARRAY_SHORT:
				{
					Sint16 *src = (Sint16 *) this->texcoord.data;
					u1 = src[(start_idx[p1]*this->texcoord.stride>>1)+0];
					v1 = src[(start_idx[p1]*this->texcoord.stride>>1)+1];
					u2 = src[(start_idx[p2]*(this->texcoord.stride>>1))+0];
					v2 = src[(start_idx[p2]*(this->texcoord.stride>>1))+1];
				}
				break;
		}

		if (v1>v2) {
			int tmp;

			tmp=u1; u1=u2; u2=tmp;
			tmp=v1; v1=v2; v2=tmp;
		}

		miny = MIN(v1, miny);
		maxy = MAX(v2, maxy);

		dv=v2-v1;
		du=u2-u1;

		if (dv==0) {
			/* Horizontal line */
			poly_x1[v1] = MIN(u1,u2);
			poly_x2[v1] = MAX(u1,u2);
		} else {
			int *poly_x;

			poly_x = (du<0 ? poly_x1 : poly_x2);
			for(y=0; y<=dv; y++) {
				poly_x[v1++] = u1 + (du*y) / dv;
			}
		}

		p1=p2;
	}

	/* Check alpha for each poly_x1[y]..poly_x2[y] segment */
	tex = this->texture;
	src_line = tex->pixels;
	src_line += miny * tex->pitch;

	alpha_pal = tex->alpha_palettes[num_pal];

	for (y=miny; (y<=maxy) && !has_alpha; y++) {
		int x;
		Uint8 *src_col = &src_line[poly_x1[y]];

		for (x=poly_x1[y]; x<=poly_x2[y]; x++) {
			if (!alpha_pal[*src_col++]) {
				/* Transparent pixel */
				has_alpha=1;
				break;
			}
		}

		src_line += tex->pitch;
	}

	free(poly_x1);
	return has_alpha;
}

static void draw(render_mesh_t *this)
{
}
