/*
	2D drawing functions
	SBuffer renderer, 8 bits mode

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

void draw_render_fill8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	Uint32 color;
	int r = segment->start.r;
	int g = segment->start.g;
	int b = segment->start.b;

	if (draw.correctPerspective>0) {
		r = segment->start.r / segment->start.w;
		g = segment->start.g / segment->start.w;
		b = segment->start.b / segment->start.w;
	}

	Uint8 *dst_col = dst_line;
	color = dither_nearest_index(r,g,b);

	memset(dst_col, color, x2 - x1 + 1);
}

void draw_render_gouraud8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
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

	if (draw.correctPerspective>0) {
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
		*dst_col++ = dither_nearest_index(r,g,b);
		r += dr;
		g += dg;
		b += db;
	}
}

void draw_render_textured8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	int dx, dxtotal, i;
	render_texture_t *tex = segment->texture;
	Uint8 *dst_col = dst_line;

	u1 = segment->start.u;
	v1 = segment->start.v;
	u2 = segment->end.u;
	v2 = segment->end.v;

	if (draw.correctPerspective>0) {
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

#if defined(__GNUC__) && defined(__m68k__)
/*
	XXxxYYyy	uv/uvd
	XXxx--YY	lsr.w
	xx--YYXX	rol.l
*/
	if ((tex->pitchw<=256) && (tex->pitchh<=256)) {
		/* Integer calculations */
		Sint32 vi = v * 65536.0f;
		Sint32 vd = dv * 65536.0f;
		Sint32 ui = u * 65536.0f;
		Sint32 ud = du * 65536.0f;
		int ushift = 16-logbase2(tex->pitchw);
		int vshift = logbase2(tex->pitchh);
		Uint32 uv = (vi>>vshift) & 0x0000ffff;
		Uint32 uvd = (vd>>vshift) & 0x0000ffff;
		uv |= (ui<<ushift) & 0xffff0000;
		uvd |= (ud<<ushift) & 0xffff0000;

		ushift = 16-ushift;
		vshift = 16-vshift;

		if (tex->paletted) {
			Uint32 *palette = tex->palettes[segment->tex_num_pal];
			Uint8 *tex_pixels = (Uint8 *) tex->pixels;

			/* for signed d0:w addressing */
			if (tex->pitchh*tex->pitchw>32768) {
				tex_pixels += 32768;
				uv ^= 0x8000;
			}

			--dx;
__asm__ __volatile__ (
	"movel	%5,d0\n\t"
	"moveql	#0,d1\n"

"R_DrawSpan8_loop:\n\t"

	"lsrw	%7,d0\n\t"
	"roll	%6,d0\n\t"
	"moveb	%1@(0,d0:w),d1\n\t"
	"addl	%2,%5\n\t"
	"moveb	%3@(3,d1:w*4),d1\n\t"
	"movel	%5,d0\n\t"
	"moveb	d1,%4@+\n\t"

	"subqw	#1,%0\n\t"
	"bpls	R_DrawSpan8_loop\n"

	: /* no return value */
	: /* input */
		"d"(dx), "a"(tex_pixels), "r"(uvd), "a"(palette),
		"a"(dst_col), "r"(uv), "d"(ushift), "d"(vshift)
	: /* clobbered registers */
		"d0", "d1", "cc", "memory" 
);						
		}/* else {
			Uint16 *tex_pixels = (Uint16 *) tex->pixels;

			for (k=dx-1; k>=0; k--) {
				Uint32 uv = (vi & vm) | (ui & 0xffff);
				uv >>= rshift;
				ui += ud;
				*dst_col++ = tex_pixels[uv];
				vi += vd;
			}
		}*/
	} else
#endif
	{
		/* Float calculations */
		if (tex->paletted) {
			Uint32 *palette = tex->palettes[segment->tex_num_pal];
			Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
			for (i=0; i<dx; i++) {
				Uint8 c = tex->pixels[((int) v)*tex->pitchw + ((int) u)];

				if (alpha_pal[c]) {
					*dst_col = palette[c];
				}
				dst_col++;
				u += du;
				v += dv;
			}
		}/* else {
			Uint16 *tex_pixels = (Uint16 *) tex->pixels;
	
			for (k=0; k<dx; k++) {
				*dst_col++ = tex_pixels[((int) v)*tex->pitchw + ((int) u)];
				u += du;
				v += dv;
			}
		}*/
	}
}
