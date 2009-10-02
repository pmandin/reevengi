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

#include "render_mesh.h"
#include "video.h"
#include "render.h"

/*--- Functions prototypes ---*/

static void shutdown(render_mesh_t *this);

static void upload(render_mesh_t *this);
static void download(render_mesh_t *this);

static void setArray(render_mesh_t *this, int array_type, int size, int type,
	int stride, void *data);
static void setTriangleIndex(render_mesh_t *this, int *triangle_idx);
static void setQuadIndex(render_mesh_t *this, int *quad_idx);

static void drawMesh(render_mesh_t *this);

/*--- Functions ---*/

render_mesh_t *render_mesh_create(void)
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
	mesh->setTriangleIndex = setTriangleIndex;
	mesh->setQuadIndex = setQuadIndex;
	mesh->drawMesh = drawMesh;

	return mesh;
}

static void shutdown(render_mesh_t *this)
{
	if (!this) {
		return;
	}

	if (this->vertex->data) {
		free(this->vertex->data);
	}
	if (this->normal->data) {
		free(this->normal->data);
	}
	if (this->texcoord->data) {
		free(this->texcoord->data);
	}

	if (this->triangles) {
		free(this->triangles);
	}
	if (this->quads) {
		free(this->quads);
	}

	free(this);
}

static void upload(render_mesh_t *this)
{
}

static void download(render_mesh_t *this)
{
}

static void setArray(render_mesh_t *this, int array_type, int size, int type,
	int stride, void *data)
{
	render_mesh_array_t *array = NULL;

	switch(array_type) {
		case RENDER_ARRAY_VERTEX:
			array = this->vertex;
			break;
		case RENDER_ARRAY_NORMAL:
			array = this->normal;
			break;
		case RENDER_ARRAY_TEXCOORD:
			array = this->texcoord;
			break;
		default:
			fprintf(stderr, "Invalid array %d\n", array_type);
			break;
	}

	if (array->data) {
		free(array->data);
	}

	array->data = data;
	array->type = type;
	array->size = size;
	array->stride = stride;
}

static void setTriangleIndex(render_mesh_t *this, int *triangle_idx)
{
}

static void setQuadIndex(render_mesh_t *this, int *quad_idx)
{
}

static void drawMesh(render_mesh_t *this)
{
	int i;
	vertex_t v[4];

	for (i=0; i<this->num_tris; i++) {
		int *tri = &(this->triangles[i*3]);

		if (this->vertex->data) {
			switch(this->vertex->type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Sint8 *) this->vertex->data;
						for (i=0; i<3; i++) {
							v[i].x = src[(tri[i]*this->vertex->stride)+0];
							v[i].y = src[(tri[i]*this->vertex->stride)+1];
							v[i].z = src[(tri[i]*this->vertex->stride)+2];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->vertex->data;
						for (i=0; i<3; i++) {
							v[i].x = src[(tri[i]*this->vertex->stride)+0];
							v[i].y = src[(tri[i]*this->vertex->stride)+1];
							v[i].z = src[(tri[i]*this->vertex->stride)+2];
						}
					}
					break;
			}
		}

		if (this->texcoord->data) {
			switch(this->texcoord->type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Sint8 *) this->texcoord->data;
						for (i=0; i<3; i++) {
							v[i].u = src[(tri[i]*this->texcoord->stride)+0];
							v[i].v = src[(tri[i]*this->texcoord->stride)+1];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->texcoord->data;
						for (i=0; i<3; i++) {
							v[i].u = src[(tri[i]*this->texcoord->stride)+0];
							v[i].v = src[(tri[i]*this->texcoord->stride)+1];
						}
					}
					break;
			}
		}

		render.triangle(&v[0], &v[1], &v[2]);
	}

	for (i=0; i<this->num_quads; i++) {
		int *quad = &(this->quads[i*3]);

		if (this->vertex->data) {
			switch(this->vertex->type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Sint8 *) this->vertex->data;
						for (i=0; i<4; i++) {
							v[i].x = src[(quad[i]*this->vertex->stride)+0];
							v[i].y = src[(quad[i]*this->vertex->stride)+1];
							v[i].z = src[(quad[i]*this->vertex->stride)+2];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->vertex->data;
						for (i=0; i<4; i++) {
							v[i].x = src[(quad[i]*this->vertex->stride)+0];
							v[i].y = src[(quad[i]*this->vertex->stride)+1];
							v[i].z = src[(quad[i]*this->vertex->stride)+2];
						}
					}
					break;
			}
		}

		if (this->texcoord->data) {
			switch(this->texcoord->type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Sint8 *) this->texcoord->data;
						for (i=0; i<4; i++) {
							v[i].u = src[(quad[i]*this->texcoord->stride)+0];
							v[i].v = src[(quad[i]*this->texcoord->stride)+1];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->texcoord->data;
						for (i=0; i<4; i++) {
							v[i].u = src[(quad[i]*this->texcoord->stride)+0];
							v[i].v = src[(quad[i]*this->texcoord->stride)+1];
						}
					}
					break;
			}
		}

		render.quad(&v[0], &v[1], &v[2], &v[3]);
	}
}
