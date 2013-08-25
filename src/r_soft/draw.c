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

#include <SDL.h>

#include "../r_common/render.h"

#include "draw.h"

/*--- Variables ---*/

draw_t draw;

/*--- Functions prototypes ---*/

static void shutdown(draw_t *this);

static void resize(draw_t *this, int w, int h, int bpp);
static void startFrame(draw_t *this);
static void flushFrame(draw_t *this);
static void endFrame(draw_t *this);

static void line(draw_t *this, draw_vertex_t *v1, draw_vertex_t *v2);
static void triangle(draw_t *this, draw_vertex_t v[3]);
static void quad(draw_t *this, draw_vertex_t v[4]);

static void poly_line(draw_t *this, vertexf_t *vtx, int num_vtx);
static void poly_fill(draw_t *this, vertexf_t *vtx, int num_vtx);
static void poly_gouraud(draw_t *this, vertexf_t *vtx, int num_vtx);
static void poly_tex(draw_t *this, vertexf_t *vtx, int num_vtx);

static void add_mask_segment(draw_t *this, int y, int x1, int x2, float w);

/*--- Functions ---*/

void draw_init(draw_t *this)
{
	this->shutdown = shutdown;

	this->resize = resize;
	this->startFrame = startFrame;
	this->flushFrame = flushFrame;
	this->endFrame = endFrame;

	this->line = line;
	this->triangle = triangle;
	this->quad = quad;

	this->polyLine = poly_line;
	this->polyFill = poly_fill;
	this->polyGouraud = poly_gouraud;
	this->polyTexture = poly_tex;

	this->addMaskSegment = add_mask_segment;
}

static void shutdown(draw_t *this)
{
}

static void resize(draw_t *this, int w, int h, int bpp)
{
}

static void startFrame(draw_t *this)
{
}

static void flushFrame(draw_t *this)
{
}

static void endFrame(draw_t *this)
{
}

static void line(draw_t *this, draw_vertex_t *v1, draw_vertex_t *v2)
{
}

static void triangle(draw_t *this, draw_vertex_t v[3])
{
}

static void quad(draw_t *this, draw_vertex_t v[4])
{
}

static void poly_line(draw_t *this, vertexf_t *vtx, int num_vtx)
{
}

static void poly_fill(draw_t *this, vertexf_t *vtx, int num_vtx)
{
}

static void poly_gouraud(draw_t *this, vertexf_t *vtx, int num_vtx)
{
}

static void poly_tex(draw_t *this, vertexf_t *vtx, int num_vtx)
{
}

static void add_mask_segment(draw_t *this, int y, int x1, int x2, float w)
{
}
