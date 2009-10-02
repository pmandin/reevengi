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

	return mesh;
}

static void shutdown(render_mesh_t *this)
{
	if (this) {
		free(this);
	}
}

static void upload(render_mesh_t *this)
{
}

static void download(render_mesh_t *this)
{
}
