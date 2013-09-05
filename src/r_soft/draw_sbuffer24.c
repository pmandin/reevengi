/*
	2D drawing functions
	SBuffer renderer, 24 bits mode

	Copyright (C) 2008-2013	Patrice Mandin

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

#include <SDL.h>
#include <assert.h>

#include "../video.h"
#include "../parameters.h"

#include "../r_common/render.h"
#include "../r_common/r_misc.h"

#include "dither.h"
#include "draw.h"
#include "draw_sbuffer.h"

/*--- Functions ---*/

void draw_render_fill24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	Uint32 color;
	int i;
	int r = segment->start.r;
	int g = segment->start.g;
	int b = segment->start.b;

	Uint8 *dst_col = dst_line;
	color = SDL_MapRGB(surf->format, r,g,b);
 
	for (i=x1; i<=x2; i++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		*dst_col++ = color>>16;
		*dst_col++ = color>>8;
		*dst_col++ = color;
#else
		*dst_col++ = color;
		*dst_col++ = color>>8;
		*dst_col++ = color>>16;
#endif
	}
}

void draw_render_gouraud24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	int dxtotal, i;
	Uint8 *dst_col = dst_line;

	r1 = segment->start.r;
	g1 = segment->start.g;
	b1 = segment->start.b;
	r2 = segment->end.r;
	g2 = segment->end.g;
	b2 = segment->end.b;

	if (drawCorrectPerspective>0) {
		r1 = segment->start.r / segment->start.w;
		g1 = segment->start.g / segment->start.w;
		b1 = segment->start.b / segment->start.w;
		r2 = segment->end.r / segment->end.w;
		g2 = segment->end.g / segment->end.w;
		b2 = segment->end.b / segment->end.w;
	}

	dxtotal = segment->end.x - segment->start.x + 1;

	dr = (r2-r1)/dxtotal;
	dg = (g2-g1)/dxtotal;
	db = (b2-b1)/dxtotal;

	r = r1 + dr * (x1-segment->start.x);
	g = g1 + dg * (x1-segment->start.x);
	b = b1 + db * (x1-segment->start.x);

	for (i=x1; i<=x2; i++) {
		Uint32 color = SDL_MapRGB(surf->format, r,g,b);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		*dst_col++ = color>>16;
		*dst_col++ = color>>8;
		*dst_col++ = color;
#else
		*dst_col++ = color;
		*dst_col++ = color>>8;
		*dst_col++ = color>>16;
#endif
		r += dr;
		g += dg;
		b += db;
	}
}

void draw_render_textured24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	int dx, dxtotal, i;
	render_texture_t *tex = segment->texture;
	Uint8 *dst_col = dst_line;

	u1 = segment->start.u;
	v1 = segment->start.v;
	u2 = segment->end.u;
	v2 = segment->end.v;

	if (drawCorrectPerspective>0) {
		u1 = segment->start.u / segment->start.w;
		v1 = segment->start.v / segment->start.w;
		u2 = segment->end.u / segment->end.w;
		v2 = segment->end.v / segment->end.w;
	}

	dxtotal = segment->end.x - segment->start.x + 1;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;

	u = u1 + du * (x1-segment->start.x);
	v = v1 + dv * (x1-segment->start.x);

	dx = x2 - x1 + 1;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		for (i=0; i<dx; i++) {
			Uint8 c = tex->pixels[((int) v)*tex->pitchw + ((int) u)];

			if (c) {
				Uint32 color = palette[c];
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				dst_col[0] = color>>16;
				dst_col[1] = color>>8;
				dst_col[2] = color;
#else
				dst_col[0] = color;
				dst_col[1] = color>>8;
				dst_col[2] = color>>16;
#endif
			}
			dst_col += 3;
			u += du;
			v += dv;
		}
	} else {
		Uint32 *tex_pixels = (Uint32 *) tex->pixels;
	
		for (i=0; i<dx; i++) {
			Uint32 color = tex_pixels[((int)v)*tex->pitchw + ((int) u)];
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			*dst_col++ = color>>16;
			*dst_col++ = color>>8;
			*dst_col++ = color;
#else
			*dst_col++ = color;
			*dst_col++ = color>>8;
			*dst_col++ = color>>16;
#endif
			u += du;
			v += dv;
		}
	}
}
