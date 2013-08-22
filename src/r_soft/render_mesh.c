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

#include "../video.h"
#include "../render.h"
#include "../log.h"
#include "../parameters.h"
#include "../render_texture.h"

#include "../r_common/render_mesh.h"
#include "../r_common/r_misc.h"

#include "render_mesh.h"

/*--- Functions prototypes ---*/

static void draw(render_mesh_t *this);

/*--- Functions ---*/

render_mesh_t *render_mesh_soft_create(render_texture_t *texture)
{
	render_mesh_t *mesh;

	mesh = render_mesh_create(texture);
	if (!mesh) {
		fprintf(stderr, "Can not allocate memory for mesh\n");
		return NULL;
	}

	mesh->draw = draw;

	logMsg(3, "render_mesh: mesh 0x%p created\n", mesh);

	return mesh;
}

static void draw(render_mesh_t *this)
{
	int i, j, prevpal=-1;
	vertex_t v[4];

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
	/*return;*/

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
}
