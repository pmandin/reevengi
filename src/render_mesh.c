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

/*--- Functions prototypes ---*/

static void shutdown(render_mesh_t *this);

static void upload(render_mesh_t *this);
static void download(render_mesh_t *this);

static void setArray(render_mesh_t *this, int size, int data_type,
	int stride, void *data);

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

	return mesh;
}

static void shutdown(render_mesh_t *this)
{
	if (!this) {
		return;
	}

	if (this->vertex.free_data) {
		free(this->vertex.data);
	}
	if (this->normal.free_data) {
		free(this->normal.data);
	}
	if (this->texcoord.free_data) {
		free(this->texcoord.data);
	}
	if (this->texpal.free_data) {
		free(this->texpal.data);
	}

	free(this);
}

static void upload(render_mesh_t *this)
{
}

static void download(render_mesh_t *this)
{
}

static void setArray(render_mesh_t *this, int size, int data_type,
	int stride, void *data)
{
	render_array_t *array = NULL;

	switch(data_type) {
		case RENDER_ARRAY_VERTEX:
			array = &(this->vertex);
			break;
		case RENDER_ARRAY_NORMAL:
			array = &(this->normal);
			break;
		case RENDER_ARRAY_TEXCOORD:
			array = &(this->texcoord);
			break;
		case RENDER_ARRAY_TEXPAL:
			array = &(this->texpal);
			break;
		default:
			fprintf(stderr, "Invalid array %d\n", data_type);
			break;
	}

	if (array->free_data) {
		free(array->data);
	}

	array->data = data;
	array->size = size;
	array->stride = stride;
	array->free_data = 0;
}
