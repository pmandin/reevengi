/*
	3D mesh
	OpenGL renderer

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_OPENGL

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"

#include "render_texture.h"
#include "render_mesh_opengl.h"
#include "video.h"
#include "render.h"
#include "log.h"

/*--- Defines ---*/

#define INVALID_LIST 0xffffffffUL

/*--- Functions prototypes ---*/

static void upload(render_mesh_t *this);
static void download(render_mesh_t *this);

static void draw(render_mesh_t *this);

/*--- Functions ---*/

render_mesh_t *render_mesh_gl_create(render_texture_t *texture)
{
	render_mesh_t *mesh;
	render_mesh_gl_t *gl_mesh;

	mesh = render_mesh_create(texture);
	if (!mesh) {
		return NULL;
	}

	gl_mesh = realloc(mesh, sizeof(render_mesh_gl_t));
	if (!gl_mesh) {
		fprintf(stderr, "Can not allocate memory for mesh\n");
		return NULL;
	}

	mesh = (render_mesh_t *) gl_mesh;

	mesh->upload = upload;
	mesh->download = download;

	mesh->draw = draw;

	gl_mesh->num_list = INVALID_LIST;

	logMsg(2, "render_mesh_gl: created\n");

	return mesh;
}

static void upload(render_mesh_t *this)
{
	render_mesh_gl_t *gl_mesh = (render_mesh_gl_t *) this;
	int i, j, prevpal=-1;
	vertex_t v[4];

	logMsg(2, "render_mesh_gl: creating new list\n");

	gl_mesh->num_list = gl.GenLists(1);	

	gl.NewList(gl_mesh->num_list, GL_COMPILE);

	for (i=0; i<this->num_tris; i++) {
		render_mesh_tri_t *tri = &(this->triangles[i]);

		if (this->vertex.data) {
			switch(this->vertex.type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Uint8 *) this->vertex.data;
						for (j=0; j<3; j++) {
							v[j].x = src[(tri->v[j]*this->vertex.stride)+0];
							v[j].y = src[(tri->v[j]*this->vertex.stride)+1];
							v[j].z = src[(tri->v[j]*this->vertex.stride)+2];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->vertex.data;
						for (j=0; j<3; j++) {
							v[j].x = src[(tri->v[j]*(this->vertex.stride>>1))+0];
							v[j].y = src[(tri->v[j]*(this->vertex.stride>>1))+1];
							v[j].z = src[(tri->v[j]*(this->vertex.stride>>1))+2];
						}
					}
					break;
			}
		}

		if (this->texcoord.data) {
			switch(this->texcoord.type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Uint8 *) this->texcoord.data;
						for (j=0; j<3; j++) {
							v[j].u = src[(tri->tx[j]*this->texcoord.stride)+0];
							v[j].v = src[(tri->tx[j]*this->texcoord.stride)+1];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->texcoord.data;
						for (j=0; j<3; j++) {
							v[j].u = src[(tri->tx[j]*(this->texcoord.stride>>1))+0];
							v[j].v = src[(tri->tx[j]*(this->texcoord.stride>>1))+1];
						}
					}
					break;
			}
		}

		if (tri->txpal != prevpal) {
			render.set_texture(tri->txpal, this->texture);
			prevpal = tri->txpal;
		}
		render.triangle(&v[0], &v[1], &v[2]);
	}

	for (i=0; i<this->num_quads; i++) {
		render_mesh_quad_t *quad = &(this->quads[i]);

		if (this->vertex.data) {
			switch(this->vertex.type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Uint8 *) this->vertex.data;
						for (j=0; j<4; j++) {
							v[j].x = src[(quad->v[j]*this->vertex.stride)+0];
							v[j].y = src[(quad->v[j]*this->vertex.stride)+1];
							v[j].z = src[(quad->v[j]*this->vertex.stride)+2];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->vertex.data;
						for (j=0; j<4; j++) {
							v[j].x = src[(quad->v[j]*(this->vertex.stride>>1))+0];
							v[j].y = src[(quad->v[j]*(this->vertex.stride>>1))+1];
							v[j].z = src[(quad->v[j]*(this->vertex.stride>>1))+2];
						}
					}
					break;
			}
		}

		if (this->texcoord.data) {
			switch(this->texcoord.type) {
				case RENDER_ARRAY_BYTE:
					{
						Uint8 *src = (Uint8 *) this->texcoord.data;
						for (j=0; j<4; j++) {
							v[j].u = src[(quad->tx[j]*this->texcoord.stride)+0];
							v[j].v = src[(quad->tx[j]*this->texcoord.stride)+1];
						}
					}
					break;
				case RENDER_ARRAY_SHORT:
					{
						Sint16 *src = (Sint16 *) this->texcoord.data;
						for (j=0; j<4; j++) {
							v[j].u = src[(quad->tx[j]*(this->texcoord.stride>>1))+0];
							v[j].v = src[(quad->tx[j]*(this->texcoord.stride>>1))+1];
						}
					}
					break;
			}
		}

		if (quad->txpal != prevpal) {
			render.set_texture(quad->txpal, this->texture);
			prevpal = quad->txpal;
		}
		render.quad(&v[0], &v[1], &v[3], &v[2]);
	}

	gl.EndList();
}

static void download(render_mesh_t *this)
{
	render_mesh_gl_t *gl_mesh = (render_mesh_gl_t *) this;

	if (gl_mesh->num_list == INVALID_LIST) {
		return;
	}

	logMsg(2, "render_mesh_gl: destroying list\n");

	gl.DeleteLists(gl_mesh->num_list, 1);
	gl_mesh->num_list = INVALID_LIST;
}

static void draw(render_mesh_t *this)
{
	render_mesh_gl_t *gl_mesh = (render_mesh_gl_t *) this;

	if (!gl_mesh) {
		return;
	}

	if (gl_mesh->num_list == INVALID_LIST) {
		this->upload(this);
	}

	gl.CallList(gl_mesh->num_list);
}

#endif /* ENABLE_OPENGL */