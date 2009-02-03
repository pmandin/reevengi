/*
	2D drawing functions

	Copyright (C) 2008	Patrice Mandin

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

#ifndef DRAW_H
#define DRAW_H 1

/*--- Types ---*/

typedef struct {
	int x,y;	/* 2D screen coordinates */
	int u,v;	/* U,V texture coordinates */
} draw_vertex_t;

/*--- Functions prototypes ---*/

void draw_init(void);
void draw_resize(int w, int h);
void draw_clear(void);
void draw_shutdown(void);

void draw_setColor(Uint32 color);
void draw_setTexture(int num_pal, render_texture_t *render_tex);

void draw_line(draw_vertex_t *v1, draw_vertex_t *v2);

void draw_triangle(draw_vertex_t v[3]);

void draw_quad(draw_vertex_t v[4]);

void draw_poly_fill(vertexf_t *vtx, int num_vtx);

void draw_poly_gouraud(vertexf_t *vtx, int num_vtx);

void draw_poly_tex(vertexf_t *vtx, int num_vtx);

#endif /* DRAW_H */
