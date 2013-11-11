/*
	2D drawing functions
	SBuffer renderer, 16 bits mode

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

void draw_render_fill16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	Uint32 color;
	float r,g,b;
	Uint16 *dst_col = (Uint16 *) dst_line;
	int i;

	r = segment->start.r;
	g = segment->start.g;
	b = segment->start.b;
	if (draw.correctPerspective>0) {
		r /= segment->start.w;
		g /= segment->start.w;
		b /= segment->start.w;
	}

	color = SDL_MapRGB(surf->format, r,g,b);

	for (i=x1; i<=x2; i++) {
		*dst_col++ = color;
	}
}

void draw_render_gouraud16_pc0(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	int dxtotal, i;
	Uint16 *dst_col = (Uint16 *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;

	r1 = segment->start.r;
	g1 = segment->start.g;
	b1 = segment->start.b;
	r2 = segment->end.r;
	g2 = segment->end.g;
	b2 = segment->end.b;

	dr = (r2-r1)/dxtotal;
	dg = (g2-g1)/dxtotal;
	db = (b2-b1)/dxtotal;

	r = r1 + dr * (x1-segment->start.x);
	g = g1 + dg * (x1-segment->start.x);
	b = b1 + db * (x1-segment->start.x);

	for (i=x1; i<=x2; i++) {
		*dst_col++ = SDL_MapRGB(surf->format, r,g,b);
		r += dr;
		g += dg;
		b += db;
	}
}

void draw_render_gouraud16_pc1(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db, invw;
	int dxtotal, i;
	Uint16 *dst_col = (Uint16 *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;

	invw = 1.0f / segment->start.w;
	r1 = segment->start.r * invw;
	g1 = segment->start.g * invw;
	b1 = segment->start.b * invw;
	invw = 1.0f / segment->end.w;
	r2 = segment->end.r * invw;
	g2 = segment->end.g * invw;
	b2 = segment->end.b * invw;

	dr = (r2-r1)/dxtotal;
	dg = (g2-g1)/dxtotal;
	db = (b2-b1)/dxtotal;

	r = r1 + dr * (x1-segment->start.x);
	g = g1 + dg * (x1-segment->start.x);
	b = b1 + db * (x1-segment->start.x);

	for (i=x1; i<=x2; i++) {
		*dst_col++ = SDL_MapRGB(surf->format, r,g,b);
		r += dr;
		g += dg;
		b += db;
	}
}

void draw_render_gouraud16_pc3(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	float w1, w2, w, dw, invw;
	int dxtotal, i;
	Uint16 *dst_col = (Uint16 *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;

	r1 = segment->start.r;
	g1 = segment->start.g;
	b1 = segment->start.b;
	w1 = segment->start.w;
	r2 = segment->end.r;
	g2 = segment->end.g;
	b2 = segment->end.b;
	w2 = segment->end.w;

	dr = (r2-r1)/dxtotal;
	dg = (g2-g1)/dxtotal;
	db = (b2-b1)/dxtotal;
	dw = (w2-w1)/dxtotal;

	r = r1 + dr * (x1-segment->start.x);
	g = g1 + dg * (x1-segment->start.x);
	b = b1 + db * (x1-segment->start.x);
	w = w1 + dw * (x1-segment->start.x);

	for (i=x1; i<=x2; i++) {
		int rr,gg,bb;

		invw = 1.0f / w;
		rr = r * invw;
		gg = g * invw;
		bb = b * invw;
		*dst_col++ = SDL_MapRGB(surf->format, rr,gg,bb);
		r += dr;
		g += dg;
		b += db;
		w += dw;
	}
}

void draw_render_textured16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	int dx, dxtotal, i;
	render_texture_t *tex = segment->texture;
	Uint16 *dst_col = (Uint16 *) dst_line;

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
	"moveql	#0,d2\n"

"R_DrawSpan16_loop:\n\t"

	"lsrw	%7,d0\n\t"
	"roll	%6,d0\n\t"
	"moveb	%1@(0,d0:w),d1\n\t"
	"addl	%2,%5\n\t"
	"movew	%3@(2,d1:w*4),d2\n\t"
	"movel	%5,d0\n\t"
	"movew	d2,%4@+\n\t"

	"subqw	#1,%0\n\t"
	"bpls	R_DrawSpan16_loop\n"

	: /* no return value */
	: /* input */
		"d"(dx), "a"(tex_pixels), "r"(uvd), "a"(palette),
		"a"(dst_col), "r"(uv), "d"(ushift), "d"(vshift)
	: /* clobbered registers */
		"d0", "d1", "d2", "cc", "memory" 
);						
		} else {
			Uint16 *tex_pixels = (Uint16 *) tex->pixels;

			for (i=0; i<dx; i++) {
				*dst_col++ = tex_pixels[((int) v)*tex->pitchw + ((int) u)];
				u += du;
				v += dv;
			}
#if 0
			Uint16 *tex_pixels = (Uint16 *) tex->pixels;
			Uint32 vm = (0xffffffff>>vshift) & 0xffff0000;
			int rshift = 16-ushift;

			for (i=dx-1; i>=0; i--) {
				Uint32 uv = (vi & vm) | (ui & 0xffff);
				uv >>= rshift;
				ui += ud;
				*dst_col++ = tex_pixels[uv];
				vi += vd;
			}
#endif
		}
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
		} else {
			Uint16 *tex_pixels = (Uint16 *) tex->pixels;

			for (i=0; i<dx; i++) {
				*dst_col++ = tex_pixels[((int) v)*tex->pitchw + ((int) u)];
				u += du;
				v += dv;
			}
		}
	}
}

void draw_render_textured16_pc0(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2;
	int dxtotal, i;
	render_texture_t *tex = segment->texture;
	Uint32 u,v,du,dv;
	Uint32 ubits, umask, vbits, vmask;
	Uint16 *dst_col = (Uint16 *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;

	u1 = segment->start.u * 65536.0f;
	v1 = segment->start.v * 65536.0f;
	u2 = segment->end.u * 65536.0f;
	v2 = segment->end.v * 65536.0f;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;

	u = u1 + du * (x1-segment->start.x);
	v = v1 + dv * (x1-segment->start.x);

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint8 c;
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			c = tex_pixels[pv|pu];
			if (alpha_pal[c]) {
				*dst_col = palette[c];
			}
			dst_col++;

			u += du;
			v += dv;
		}
	} else {
		Uint16 *tex_pixels = (Uint16 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			*dst_col++ = tex_pixels[pv|pu];

			u += du;
			v += dv;
		}
	}
}

void draw_render_textured16_pc1(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, invw;
	int dxtotal, i;
	render_texture_t *tex = segment->texture;
	Uint32 u,v,du,dv;
	Uint32 ubits, umask, vbits, vmask;
	Uint16 *dst_col = (Uint16 *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;

	invw = 65536.0f / segment->start.w;
	u1 = segment->start.u * invw;
	v1 = segment->start.v * invw;
	invw = 65536.0f / segment->end.w;
	u2 = segment->end.u * invw;
	v2 = segment->end.v * invw;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;

	u = u1 + du * (x1-segment->start.x);
	v = v1 + dv * (x1-segment->start.x);

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint8 c;
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			c = tex_pixels[pv|pu];
			if (alpha_pal[c]) {
				*dst_col = palette[c];
			}
			dst_col++;

			u += du;
			v += dv;
		}
	} else {
		Uint16 *tex_pixels = (Uint16 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			*dst_col++ = tex_pixels[pv|pu];

			u += du;
			v += dv;
		}
	}
}

void draw_render_textured16_pc2(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	float w1, w2, w, dw, invw;
	float du16,dv16,dw16;
	int dxtotal, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	Uint16 *dst_col = (Uint16 *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;

	u1 = segment->start.u;
	v1 = segment->start.v;
	w1 = segment->start.w;

	u2 = segment->end.u;
	v2 = segment->end.v;
	w2 = segment->end.w;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;
	dw = (w2-w1)/dxtotal;

	u1 += du * (x1-segment->start.x);
	v1 += dv * (x1-segment->start.x);
	w1 += dw * (x1-segment->start.x);

	du16 = du * 16.0f;
	dv16 = dv * 16.0f;
	dw16 = dw * 16.0f;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i+=16) {
			int j;
			Uint32 dui, dvi, uu, vv, uu2,vv2;

			u2 = u1 + du16;
			v2 = v1 + dv16;
			w2 = w1 + dw16;

			invw = 65536.0f / w1;
			uu = u1 * invw;
			vv = v1 * invw;
			invw = 65536.0f / w2;
			uu2 = u2 * invw;
			vv2 = v2 * invw;

			dui = (uu2-uu)/16.0f;
			dvi = (vv2-vv)/16.0f;

			for (j=0; j<MIN(x2-i+1,16); j++) {
				Uint8 c;
				Uint32 pu,pv;

				pu = uu>>16;		/* 0000XXXX */
				pu &= umask;		/* 0000---X */
				pv = vv>>(16-ubits);	/* 000YYYYy */
				pv &= vmask;		/* 000YYYY- */

				c = tex_pixels[pv|pu];
				if (alpha_pal[c]) {
					*dst_col = palette[c];
				}
				dst_col++;

				uu += dui;
				vv += dvi;
			}

			u1 = u2;
			v1 = v2;
			w1 = w2;
		}
	} else {
		Uint16 *tex_pixels = (Uint16 *) tex->pixels;

		for (i=x1; i<=x2; i+=16) {
			int j;
			Uint32 dui, dvi, uu, vv, uu2,vv2;

			u2 = u1 + du16;
			v2 = v1 + dv16;
			w2 = w1 + dw16;

			invw = 65536.0f / w1;
			uu = u1 * invw;
			vv = v1 * invw;
			invw = 65536.0f / w2;
			uu2 = u2 * invw;
			vv2 = v2 * invw;

			dui = (uu2-uu)/16.0f;
			dvi = (vv2-vv)/16.0f;

			for (j=0; j<MIN(x2-i+1,16); j++) {
				Uint32 pu,pv;

				pu = uu>>16;		/* 0000XXXX */
				pu &= umask;		/* 0000---X */
				pv = vv>>(16-ubits);	/* 000YYYYy */
				pv &= vmask;		/* 000YYYY- */

				*dst_col++ = tex_pixels[pv|pu];

				uu += dui;
				vv += dvi;
			}

			u1 = u2;
			v1 = v2;
			w1 = w2;
		}
	}
}

void draw_render_textured16_pc3(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	float w1, w2, w, dw;
	int dxtotal, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	Uint16 *dst_col = (Uint16 *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;

	u1 = segment->start.u;
	v1 = segment->start.v;
	w1 = segment->start.w;
	u2 = segment->end.u;
	v2 = segment->end.v;
	w2 = segment->end.w;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;
	dw = (w2-w1)/dxtotal;

	u = u1 + du * (x1-segment->start.x);
	v = v1 + dv * (x1-segment->start.x);
	w = w1 + dw * (x1-segment->start.x);

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint8 c;
			Uint32 uu,vv;
			float invw;

			invw = 65536.0f / w;
			uu = u * invw;	/* XXXXxxxx */
			vv = v * invw;	/* YYYYyyyy */

			uu >>= 16;		/* 0000XXXX */
			uu &= umask;		/* 0000---X */
			vv >>= 16-ubits;	/* 000YYYYy */
			vv &= vmask;		/* 000YYYY- */

			c = tex_pixels[vv|uu];
			if (alpha_pal[c]) {
				*dst_col = palette[c];
			}
			dst_col++;

			u += du;
			v += dv;
			w += dw;
		}
	} else {
		Uint16 *tex_pixels = (Uint16 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 uu,vv;
			float invw;

			invw = 65536.0f / w;
			uu = u * invw;	/* XXXXxxxx */
			vv = v * invw;	/* YYYYyyyy */

			uu >>= 16;		/* 0000XXXX */
			uu &= umask;		/* 0000---X */
			vv >>= 16-ubits;	/* 000YYYYy */
			vv &= vmask;		/* 000YYYY- */

			*dst_col++ = tex_pixels[vv|uu];

			u += du;
			v += dv;
			w += dw;
		}
	}
}
