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

/*---- External types ---*/

struct vertexf_s;

/*--- Types ---*/

typedef struct {
	int x,y;	/* 2D screen coordinates */
	int u,v;	/* U,V texture coordinates */
} draw_vertex_t;

typedef struct draw_s draw_t;

struct draw_s {
	/*--- Functions ---*/
	void (*shutdown)(draw_t *this);

	void (*resize)(draw_t *this, int w, int h, int bpp);
	void (*startFrame)(draw_t *this);
	void (*flushFrame)(draw_t *this);
	void (*endFrame)(draw_t *this);

	/* Wireframe */
	void (*line)(draw_t *this, draw_vertex_t *v1, draw_vertex_t *v2);
	void (*triangle)(draw_t *this, draw_vertex_t v[3]);
	void (*quad)(draw_t *this, draw_vertex_t v[4]);

	void (*polyLine)(draw_t *this, struct vertexf_s *vtx, int num_vtx);

	/* Filled */
	void (*polyFill)(draw_t *this, struct vertexf_s *vtx, int num_vtx);

	/* Gouraud */
	void (*polyGouraud)(draw_t *this, struct vertexf_s *vtx, int num_vtx);

	/* Textured */
	void (*polyTexture)(draw_t *this, struct vertexf_s *vtx, int num_vtx);

	/* Add mask segment */
	void (*addMaskSegment)(draw_t *this, int y1, int x1, int x2, float w);

	/* Perspective correction ? */
	/* 0:none, 1:per scanline, 2:every 16 pixels */
	int correctPerspective;
};

/*--- Variables ---*/

extern draw_t draw;

/*--- Functions ---*/

void draw_init(draw_t *this);

int clip_line(int *x1, int *y1, int *x2, int *y2);

#endif /* DRAW_H */
