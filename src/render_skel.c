/*
	3D skeleton, composed of meshes

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

#include "render_texture.h"
#include "render_mesh.h"
#include "render_skel.h"
#include "video.h"
#include "render.h"

/*--- Functions prototypes ---*/

static void shutdown(render_skel_t *this);

static void upload(render_skel_t *this);
static void download(render_skel_t *this);

static void addMesh(render_skel_t *this, render_mesh_t *mesh);

static void drawSkel(render_skel_t *this);

/*--- Functions ---*/

render_skel_t *render_skel_create(render_texture_t *texture)
{
	render_skel_t *skel;

	skel = calloc(1, sizeof(render_skel_t));
	if (!skel) {
		fprintf(stderr, "Can not allocate memory for skeleton\n");
		return NULL;
	}

	skel->shutdown = shutdown;

	skel->upload = upload;
	skel->download = download;

	skel->addMesh = addMesh;
	skel->drawSkel = drawSkel;

	skel->texture = texture;

	return skel;
}

static void shutdown(render_skel_t *this)
{
	int i;

	if (!this) {
		return;
	}

	for (i=0; i<this->num_meshes; i++) {
		render_mesh_t *mesh = this->meshes[i];

		mesh->shutdown(mesh);
	}	

	free(this);
}

static void upload(render_skel_t *this)
{
	int i;

	for (i=0; i<this->num_meshes; i++) {
		render_mesh_t *mesh = this->meshes[i];

		mesh->upload(mesh);
	}	
}

static void download(render_skel_t *this)
{
	int i;

	for (i=0; i<this->num_meshes; i++) {
		render_mesh_t *mesh = this->meshes[i];

		mesh->download(mesh);
	}	
}

static void addMesh(render_skel_t *this, render_mesh_t *mesh)
{
	render_mesh_t **new_meshes;
	int num_meshes = this->num_meshes + 1;

	new_meshes = (render_mesh_t **) realloc(this->meshes, num_meshes * sizeof(render_mesh_t *));
	if (new_meshes) {
		return;
	}

	this->meshes = new_meshes;
	this->meshes[this->num_meshes++] = mesh;
}

static void drawSkel(render_skel_t *this)
{
	int i;

	for (i=0; i<this->num_meshes; i++) {
		render_mesh_t *mesh = this->meshes[i];

		mesh->drawMesh(mesh);
	}	
}
