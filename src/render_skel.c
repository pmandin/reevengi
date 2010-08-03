/*
	3D skeleton, composed of meshes

	Copyright (C) 2009-2010	Patrice Mandin

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
#include <assert.h>

#include <SDL.h>

#include "render_texture.h"
#include "render_mesh.h"
#include "render_skel.h"
#include "video.h"
#include "render.h"
#include "log.h"
#include "render_skel_list.h"

/*--- Functions prototypes ---*/

static void shutdown(render_skel_t *this);

static void upload(render_skel_t *this);
static void download(render_skel_t *this);

static void addMesh(render_skel_t *this, render_mesh_t *mesh,
	Sint16 x, Sint16 y, Sint16 z);
static void setParent(render_skel_t *this, int parent, int child);

#if 1
static void draw(render_skel_t *this, int num_parent);
static void drawBones(render_skel_t *this, int num_parent);
#else
static void draw(render_skel_t *this, render_skel_mesh_t *parent);
static void drawBones(render_skel_t *this, render_skel_mesh_t *parent);
#endif

static int getChild(render_skel_t *this, int num_parent, int num_child);

static void setAnimFrame(render_skel_t *this, int num_anim, int num_frame);
static void getAnimPosition(render_skel_t *this, int *x, int *y, int *z);
static void getAnimSpeed(render_skel_t *this, int *x, int *y, int *z);
static void getAnimAngles(render_skel_t *this, int num_mesh, int *x, int *y, int *z); 

/*--- Functions ---*/

render_skel_t *render_skel_create(void *emd_file, Uint32 emd_length, render_texture_t *texture)
{
	render_skel_t *skel;

	skel = calloc(1, sizeof(render_skel_t));
	if (!skel) {
		fprintf(stderr, "Can not allocate memory for skeleton\n");
		return NULL;
	}

	skel->shutdown = shutdown;

	skel->emd_file = emd_file;
	skel->emd_length = emd_length;

	skel->upload = upload;
	skel->download = download;

	skel->addMesh = addMesh;
	skel->setParent = setParent;
	skel->draw = draw;
	skel->drawBones = drawBones;

	skel->texture = texture;

	skel->getChild = getChild;

	skel->num_anim = skel->num_frame = 0;
	skel->setAnimFrame = setAnimFrame;
	skel->getAnimPosition = getAnimPosition;
	skel->getAnimSpeed = getAnimSpeed;
	skel->getAnimAngles = getAnimAngles;

	logMsg(3, "render_skel: skel 0x%p created\n", skel);

	list_render_skel_add(skel);

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

	if (this->texture) {
		this->texture->shutdown(this->texture);
	}

	if (this->emd_file) {
		free(this->emd_file);
	}

	logMsg(3, "render_skel: skel 0x%p destroyed\n", this);

	list_render_skel_remove(this);

	free(this);
}

static void upload(render_skel_t *this)
{
	int i;

	logMsg(2, "render_skel: upload\n");

	for (i=0; i<this->num_meshes; i++) {
		render_mesh_t *mesh = this->meshes[i].mesh;

		mesh->upload(mesh);
	}	

	/* Texture reuploaded on render.set_texture() call */
}

static void download(render_skel_t *this)
{
	int i;

	logMsg(2, "render_skel: download\n");

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

	logMsg(3, "render_skel: skel 0x%p, adding mesh 0x%p\n", this, mesh);

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

	if ((parent>=this->num_meshes) || (child>=this->num_meshes) || (parent==child)) {
		return;
	}

	logMsg(3, "render_skel: skel 0x%p, setting mesh %d as parent for mesh %d\n", this, parent, child);

	parent_mesh = &(this->meshes[parent]);
	child_mesh = &(this->meshes[child]);

	child_mesh->parent = parent_mesh;
}

#if 1
static void draw(render_skel_t *this, int num_parent)
{
	int i=0, num_child;
	render_skel_mesh_t *skel_mesh = &(this->meshes[num_parent]);

	/* Draw mesh */
	render.push_matrix();
	render.translate(
		skel_mesh->x,
		skel_mesh->y,
		skel_mesh->z
	);

	skel_mesh->mesh->draw(skel_mesh->mesh);

	/* Draw children, relative to parent */
	num_child = this->getChild(this, num_parent, i);
	while (num_child != -1) {
		this->draw(this, num_child);
		
		++i;
		num_child = this->getChild(this, num_parent, i);
	}

	render.pop_matrix();
}

static void drawBones(render_skel_t *this, int num_parent)
{
	int i=0, num_child;
	vertex_t v[2];
	render_skel_mesh_t *skel_mesh = &(this->meshes[num_parent]);

	render.set_color(0x0000ff00);

	/* Draw mesh */
	v[0].x = 0;
	v[0].y = 0;
	v[0].z = 0;

	v[1].x = skel_mesh->x;
	v[1].y = skel_mesh->y;
	v[1].z = skel_mesh->z;

	render.line(&v[0], &v[1]);

	render.push_matrix();
	render.translate(
		skel_mesh->x,
		skel_mesh->y,
		skel_mesh->z
	);

	/* Draw children, relative to parent */
	num_child = this->getChild(this, num_parent, i);
	while (num_child != -1) {
		this->drawBones(this, num_child);
		
		++i;
		num_child = this->getChild(this, num_parent, i);
	}

	render.pop_matrix();
}

#else

static void draw(render_skel_t *this, render_skel_mesh_t *parent)
{
	int i;

	for (i=0; i<this->num_meshes; i++) {
		render_skel_mesh_t *skel_mesh = &(this->meshes[i]);

		if (skel_mesh->parent != parent) {
			continue;
		}

		render.push_matrix();
		render.translate(
			skel_mesh->x,
			skel_mesh->y,
			skel_mesh->z
		);

		/* Draw mesh */
		skel_mesh->mesh->draw(skel_mesh->mesh);

		/*render.pop_matrix();
		break;*/

		/* Draw children, relative to parent */
		this->draw(this, skel_mesh);

		render.pop_matrix();

		/* FIXME: Only draw first parent object */
		if (parent==NULL) {
			break;
		}
	}
}

static void drawBones(render_skel_t *this, render_skel_mesh_t *parent)
{
	int i;
	vertex_t v[2];

	render.set_color(0x0000ff00);

	for (i=0; i<this->num_meshes; i++) {
		render_skel_mesh_t *skel_mesh = &(this->meshes[i]);

		if (skel_mesh->parent != parent) {
			continue;
		}

		if (parent) {
			v[0].x = 0;
			v[0].y = 0;
			v[0].z = 0;

			v[1].x = skel_mesh->x;
			v[1].y = skel_mesh->y;
			v[1].z = skel_mesh->z;

			render.line(&v[0], &v[1]);
		}

		render.push_matrix();
		render.translate(
			skel_mesh->x,
			skel_mesh->y,
			skel_mesh->z
		);

		/* Draw children, relative to parent */
		this->drawBones(this, skel_mesh);

		render.pop_matrix();

		/* FIXME: Only draw first parent object */
		if (parent==NULL) {
			break;
		}
	}
}
#endif

static int getChild(render_skel_t *this, int num_parent, int num_child)
{
	return -1;
}

static void setAnimFrame(render_skel_t *this, int num_anim, int num_frame)
{
	assert(this);

	this->num_anim = num_anim;
	this->num_frame = num_frame;
}

static void getAnimPosition(render_skel_t *this, int *x, int *y, int *z)
{
	*x = *z = 0;
	*y = -2000;
}

static void getAnimSpeed(render_skel_t *this, int *x, int *y, int *z)
{
	*x = *y = *z = 0;
}

static void getAnimAngles(render_skel_t *this, int num_mesh, int *x, int *y, int *z)
{
	*x = *y = *z = 0;
}
