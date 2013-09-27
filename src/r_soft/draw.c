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

#include "../video.h"

#include "../r_common/render.h"

#include "draw.h"

/*--- Defines ---*/

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

/*--- Variables ---*/

draw_t draw;

/*--- Functions prototypes ---*/

static void shutdown(draw_t *this);

static void resize(draw_t *this, int w, int h, int bpp);
static void startFrame(draw_t *this);
static void endFrame(draw_t *this);
static void set_depth(draw_t *this, int enable);

static void line(draw_t *this, draw_vertex_t *v1, draw_vertex_t *v2);
static void triangle(draw_t *this, draw_vertex_t v[3]);
static void quad(draw_t *this, draw_vertex_t v[4]);

static void poly_line(draw_t *this, vertexf_t *vtx, int num_vtx);
static void poly_fill(draw_t *this, vertexf_t *vtx, int num_vtx);
static void poly_gouraud(draw_t *this, vertexf_t *vtx, int num_vtx);
static void poly_tex(draw_t *this, vertexf_t *vtx, int num_vtx);

static void add_mask_segment(draw_t *this, int y, int x1, int x2, float w);

static int clipEncode (int x, int y, int left, int top, int right, int bottom);

/*--- Functions ---*/

void draw_init(draw_t *this)
{
	this->shutdown = shutdown;

	this->resize = resize;
	this->startFrame = startFrame;
	this->endFrame = endFrame;

	this->set_depth = set_depth;

	this->line = line;
	this->triangle = triangle;
	this->quad = quad;

	this->polyLine = poly_line;
	this->polyFill = poly_fill;
	this->polyGouraud = poly_gouraud;
	this->polyTexture = poly_tex;

	this->addMaskSegment = add_mask_segment;

	this->correctPerspective = 0;
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

static void endFrame(draw_t *this)
{
}

static void set_depth(draw_t *this, int enable)
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

/* Clip line to viewport size */
static int clipEncode (int x, int y, int left, int top, int right, int bottom)
{
	int code = 0;

	if (x < left) {
		code |= CLIP_LEFT_EDGE;
	} else if (x > right) {
		code |= CLIP_RIGHT_EDGE;
	}
	if (y < top) {
		code |= CLIP_TOP_EDGE;
	} else if (y > bottom) {
		code |= CLIP_BOTTOM_EDGE;
	}

	return code;
}

int clip_line(int *x1, int *y1, int *x2, int *y2)
{
	int left = video.viewport.x;
	int top = video.viewport.y;
	int right = left+video.viewport.w-1;
	int bottom = top+video.viewport.h-1;

	int draw_line = 0;
	for(;;) {
		int code1 = clipEncode(*x1, *y1, left, top, right, bottom);
		int code2 = clipEncode(*x2, *y2, left, top, right, bottom);
		float m;

		if (CLIP_ACCEPT(code1, code2)) {
			return 1;
		}

		if (CLIP_REJECT(code1, code2)) {
			return 0;
		}
		
		if (CLIP_INSIDE(code1)) {
			int swaptmp = *x2; *x2 = *x1; *x1 = swaptmp;
			swaptmp = *y2; *y2 = *y1; *y1 = swaptmp;
			swaptmp = code2; code2 = code1; code1 = swaptmp;
		}

		m = 1.0f;
		if (*x2 != *x1) {
			m = (*y2 - *y1) / (float)(*x2 - *x1);
		}
		if (code1 & CLIP_LEFT_EDGE) {
			*y1 += (int)((left - *x1) * m);
			*x1 = left;
		} else if (code1 & CLIP_RIGHT_EDGE) {
			*y1 += (int)((right - *x1) * m);
			*x1 = right;
		} else if (code1 & CLIP_BOTTOM_EDGE) {
			if (*x2 != *x1) {
				*x1 += (int)((bottom - *y1) / m);
			}
			*y1 = bottom;
		} else if (code1 & CLIP_TOP_EDGE) {
			if (*x2 != *x1) {
				*x1 += (int)((top - *y1) / m);
			}
			*y1 = top;
		}
	}

	return draw_line;
}
