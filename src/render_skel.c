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

static void addMesh(render_skel_t *this, render_mesh_t *mesh,
	Sint16 x, Sint16 y, Sint16 z);
static void setParent(render_skel_t *this, int parent, int child);

static void draw(render_skel_t *this);
static void drawChild(render_skel_t *this, render_skel_mesh_t *parent);

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
	skel->setParent = setParent;
	skel->draw = draw;

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
		render_mesh_t *mesh = this->meshes[i].mesh;

		mesh->shutdown(mesh);
	}	

	if (this->meshes) {
		free(this->meshes);
	}

	/* TODO: enable when render_skel is completed and in use
	if (this->texture) {
		this->texture->shutdown(this->texture);
	}*/

	free(this);
}

static void upload(render_skel_t *this)
{
	int i;

	for (i=0; i<this->num_meshes; i++) {
		render_mesh_t *mesh = this->meshes[i].mesh;

		mesh->upload(mesh);
	}	

	/* Texture reuploaded on render.set_texture() call */
}

static void download(render_skel_t *this)
{
	int i;

	for (i=0; i<this->num_meshes; i++) {
		render_mesh_t *mesh = this->meshes[i].mesh;

		mesh->download(mesh);
	}	

	if (this->texture) {
		this->texture->download(this->texture);
	}
}

static void addMesh(render_skel_t *this, render_mesh_t *mesh,
	Sint16 x, Sint16 y, Sint16 z)
{
	render_skel_mesh_t *new_meshes;
	int num_meshes = this->num_meshes + 1;

	new_meshes = (render_skel_mesh_t *) realloc(this->meshes, num_meshes * sizeof(render_skel_mesh_t));
	if (!new_meshes) {
		fprintf(stderr, "Can not allocate memory for mesh\n");
		return;
	}

	this->meshes = new_meshes;

	this->meshes[this->num_meshes].x = x;
	this->meshes[this->num_meshes].y = y;
	this->meshes[this->num_meshes].z = z;
	this->meshes[this->num_meshes].mesh = mesh;
	this->meshes[this->num_meshes].parent = NULL;

	this->num_meshes++;
}

static void setParent(render_skel_t *this, int parent, int child)
{
	render_skel_mesh_t *parent_mesh, *child_mesh;

	if ((parent>=this->num_meshes) || (child>=this->num_meshes)) {
		return;
	}

	parent_mesh = &(this->meshes[parent]);
	child_mesh = &(this->meshes[child]);

	child_mesh->parent = parent_mesh;
}

static void draw(render_skel_t *this)
{
	int i;

	this->upload(this);

	printf("skel: draw %d meshes\n", this->num_meshes);

	for (i=0; i<this->num_meshes; i++) {
		render_skel_mesh_t *skel_mesh = &(this->meshes[i]);

		if (skel_mesh->parent) {
			continue;
		}

		printf("skel: draw mesh %d\n", i);

		render.push_matrix();
		render.translate(
			skel_mesh->x,
			skel_mesh->y,
			skel_mesh->z
		);

		/* Draw mesh */
		skel_mesh->mesh->drawMesh(skel_mesh->mesh);

		/* Draw children, relative to parent */
		drawChild(this, skel_mesh);

		render.pop_matrix();
	}	
}

static void drawChild(render_skel_t *this, render_skel_mesh_t *parent)
{
	int i;

	for (i=0; i<this->num_meshes; i++) {
		render_skel_mesh_t *skel_mesh = &(this->meshes[i]);

		if (skel_mesh->parent != parent) {
			continue;
		}

		printf("skel: draw mesh %d\n", i);

		render.push_matrix();
		render.translate(
			skel_mesh->x,
			skel_mesh->y,
			skel_mesh->z
		);

		/* Draw mesh */
		skel_mesh->mesh->drawMesh(skel_mesh->mesh);

		/* Draw children, relative to parent */
		drawChild(this, skel_mesh);

		render.pop_matrix();
	}	
}
