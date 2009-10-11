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

static void set_color_from_texture(render_texture_t *texture, int num_pal, int u, int v)
{
	Uint32 color = 0xffffffff;

	if (!texture) {
		return;
	}

	if (texture->paletted) {
		Uint8 pix = texture->pixels[texture->pitch * v + u];
		
		color = texture->palettes[num_pal][pix];
	} else {
		int r,g,b;
		Uint16 pix = ((Uint16 *) texture->pixels)[((texture->pitch>>1) * v) + u];

		r = (pix>>8) & 0xf8;
		r |= r>>5;
		g = (pix>>3) & 0xfc;
		g |= g>>6;
		b = (pix<<3) & 0xf8;
		b |= b>>5;
		color = (r<<16)|(g<<8)|b;
	}

	gl.Color4ub((color>>16) & 0xff, (color>>8) & 0xff,
		color & 0xff, (color>>24) & 0xff);
}

static void upload(render_mesh_t *this)
{
	render_mesh_gl_t *gl_mesh = (render_mesh_gl_t *) this;
	int i, j, prevpal=0;
	vertex_t v[4];
	Sint16 *srcVtx = (Sint16 *) this->vertex.data;
	Sint16 *srcTx = (Sint16 *) this->texcoord.data;

	logMsg(2, "render_mesh_gl: creating new list\n");

	gl_mesh->num_list = gl.GenLists(1);	

	/*gl.EnableClientState(GL_VERTEX_ARRAY);*/
	/*gl.EnableClientState(GL_TEXTURE_COORD_ARRAY);*/

	/*gl.VertexPointer(3, GL_SHORT, 0, this->vertex.data);*/
	/*gl.TexCoordPointer(2, GL_SHORT, 0, this->texcoord.data);*/

	gl.NewList(gl_mesh->num_list, GL_COMPILE);

	if (this->num_tris>0) {
		gl.Begin(GL_TRIANGLES);

		for (i=0; i<this->num_tris; i++) {
			render_mesh_tri_t *tri = &(this->triangles[i]);
			Sint16 *srcTxi = &srcTx[tri->tx[0]*(this->texcoord.stride>>1)];

			switch(render.render_mode) {
				case RENDER_WIREFRAME:
				case RENDER_FILLED:
					set_color_from_texture(this->texture, tri->txpal, srcTxi[0], srcTxi[1]);
					break;
				case RENDER_TEXTURED:
					if (tri->txpal != prevpal) {
						render.set_texture(tri->txpal, this->texture);
						prevpal = tri->txpal;
					}
					break;
			}

			for (j=0; j<3; j++) {
				srcTxi = &srcTx[tri->tx[j]*(this->texcoord.stride>>1)];

				if (render.render_mode == RENDER_GOURAUD) {
					set_color_from_texture(this->texture, tri->txpal, srcTxi[0], srcTxi[1]);
				}
				if (render.render_mode == RENDER_TEXTURED) {
					gl.TexCoord2sv( srcTxi );
				}
				gl.Vertex3sv( &srcVtx[tri->v[j]*(this->vertex.stride>>1)] );
			}

			/*gl.ArrayElement(tri->v[0]);
			gl.ArrayElement(tri->v[1]);
			gl.ArrayElement(tri->v[2]);*/
		}

		gl.End();
	}

	if (this->num_quads>0) {
		gl.Begin(GL_QUADS);

		for (i=0; i<this->num_quads; i++) {
			render_mesh_quad_t *quad = &(this->quads[i]);
			Sint16 *srcTxi = &srcTx[quad->tx[0]*(this->texcoord.stride>>1)];

			switch(render.render_mode) {
				case RENDER_WIREFRAME:
				case RENDER_FILLED:
					set_color_from_texture(this->texture, quad->txpal, srcTxi[0], srcTxi[1]);
					break;
				case RENDER_TEXTURED:
					if (quad->txpal != prevpal) {
						render.set_texture(quad->txpal, this->texture);
						prevpal = quad->txpal;
					}
					break;
			}

			for (j=0; j<4; j++) {
				int k = j;

				if (j==2) k=3;
				if (j==3) k=2;

				srcTxi = &srcTx[quad->tx[j]*(this->texcoord.stride>>1)];
				if (render.render_mode == RENDER_GOURAUD) {
					set_color_from_texture(this->texture, quad->txpal, srcTxi[0], srcTxi[1]);
				}
				if (render.render_mode == RENDER_TEXTURED) {
					gl.TexCoord2sv( srcTxi );
				}
				gl.Vertex3sv( &srcVtx[quad->v[k]*(this->vertex.stride>>1)] );
			}

			/*gl.ArrayElement(quad->v[0]);
			gl.ArrayElement(quad->v[1]);
			gl.ArrayElement(quad->v[3]);
			gl.ArrayElement(quad->v[2]);*/
		}

		gl.End();
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
