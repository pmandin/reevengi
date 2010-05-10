/*
	2D drawing functions
	SBuffer renderer

	Copyright (C) 2008-2010	Patrice Mandin

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

#include "video.h"
#include "parameters.h"
#include "dither.h"
#include "render.h"
#include "draw.h"
#include "draw_simple.h"

/*--- Defines ---*/

/* Debug print info */
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#else
#define DEBUG_PRINT(what)
#endif

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

#define NUM_SEGMENTS 128
#define NUM_SEGMENTS_DATA 128

#define SEG1_FRONT 0
#define SEG1_BEHIND 1
#define SEG1_CLIP_LEFT 2
#define SEG1_CLIP_RIGHT 3

#define MIN(x1, x2) \
	( x1 < x2 ? x1 : x2)
#define MAX(x1, x2) \
	( x1 > x2 ? x1 : x2)

/*--- Types ---*/

typedef struct {
	int x;		/* x on screen*/
	float r,g,b;	/* color */
	float u,v;	/* u,v coords */
	float w;	/* w=1/z */
} sbuffer_point_t;

typedef struct {
	sbuffer_point_t sbp[2]; /* 0:min, 1:max */
} poly_hline_t;

typedef struct {
	Uint8	dummy;
	Uint8	render_mode;
	Uint8	tex_num_pal;
	Uint8	masking;
	render_texture_t *texture;
	sbuffer_point_t start, end;
} sbuffer_segment_t;

typedef struct {
	Uint16 id;	/* Index in sbuffer_segment_t array for this row */
	Uint16 dummy;
	Uint16 x1,x2;	/* Start,end on the row */
} sbuffer_segdata_t;

typedef struct {
	Uint16 num_segs;
	Uint16 num_segs_data;
	sbuffer_segment_t segment[NUM_SEGMENTS];
	sbuffer_segdata_t segdata[NUM_SEGMENTS_DATA];
} sbuffer_row_t;

typedef void (*sbuffer_draw_f)(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);

/*--- Variables ---*/

/* for poly rendering */
static int size_poly_minmaxx = 0;
static poly_hline_t *poly_hlines = NULL;

/* Sbuffer */
static int sbuffer_numrows = 0;
static sbuffer_row_t *sbuffer_rows = NULL;
static Uint32 sbuffer_seg_id;

static int drawCorrectPerspective = 0; /* 0:none, 1:per scanline, 2:every 16 pixels */

/*--- Functions prototypes ---*/

static void draw_shutdown(draw_t *this);

#if defined(__GNUC__) && (defined(__M68000__) || defined(__M68020__))
static unsigned logbase2(unsigned n);
#endif

static void clear_sbuffer(void);

static void draw_resize(draw_t *this, int w, int h);
static void draw_startFrame(draw_t *this);
static void draw_flushFrame(draw_t *this);
static void draw_endFrame(draw_t *this);

static void draw_render_fill8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_fill16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_fill24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_fill32(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);

static void draw_render_gouraud8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_gouraud16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_gouraud24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_gouraud32(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);

static void draw_render_textured8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_textured16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_textured24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);
static void draw_render_textured32(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);

static void draw_poly_sbuffer(draw_t *this, vertexf_t *vtx, int num_vtx);
static void draw_mask_segment(draw_t *this, int y, int x1, int x2, float w);

/*--- Functions ---*/

void draw_init_sbuffer(draw_t *draw)
{
	draw_init_simple(draw);

	draw->shutdown = draw_shutdown;

	draw->resize = draw_resize;
	draw->startFrame = draw_startFrame;
	draw->flushFrame = draw_flushFrame;
	draw->endFrame = draw_endFrame;

	draw->polyLine = draw_poly_sbuffer;
	draw->polyFill = draw_poly_sbuffer;
	draw->polyGouraud = draw_poly_sbuffer;
	draw->polyTexture = draw_poly_sbuffer;
	draw->addMaskSegment = draw_mask_segment;

	clear_sbuffer();
}

static void draw_shutdown(draw_t *this)
{
	if (sbuffer_rows) {
		free(sbuffer_rows);
		sbuffer_rows = NULL;
	}
	sbuffer_numrows = 0;

	if (poly_hlines) {
		free(poly_hlines);
		poly_hlines = NULL;
	}
	size_poly_minmaxx = 0;
}

#if defined(__GNUC__) && (defined(__M68000__) || defined(__M68020__))
static unsigned logbase2(unsigned n)
{
	unsigned log2 = 0;
	while (n >>= 1)
		++log2;
	return log2;
}
#endif

static void draw_resize(draw_t *this, int w, int h)
{
	if (h>sbuffer_numrows) {
		sbuffer_rows = realloc(sbuffer_rows, h * sizeof(sbuffer_row_t));
		sbuffer_numrows = h;
	}
}

static void clear_sbuffer(void)
{
	int i;

	DEBUG_PRINT(("----------clearing sbuffer\n"));

	for (i=0; i<sbuffer_numrows; i++) {
		sbuffer_rows[i].num_segs = 0;
		sbuffer_rows[i].num_segs_data = 0;
	}
	sbuffer_seg_id = 0;
}

static void draw_startFrame(draw_t *this)
{
	clear_sbuffer();
}

#if 0
static void check_sbuffer(void)
{
	int i;

	for (i=0; i<sbuffer_numrows; i++) {
		if (sbuffer_rows[i].num_segs>NUM_SEGMENTS) {
			printf("row %d: segs %d\n", i,sbuffer_rows[i].num_segs);
		}
		if (sbuffer_rows[i].num_segs_data>NUM_SEGMENTS_DATA) {
			printf("row %d: segs data %d\n", i,sbuffer_rows[i].num_segs_data);
		}
	}
}
#endif

static void draw_flushFrame(draw_t *this)
{
	SDL_Surface *surf = video.screen;
	int i,j;
	Uint8 *dst = (Uint8 *) surf->pixels;
	sbuffer_draw_f draw_render_fill = draw_render_fill8;
	sbuffer_draw_f draw_render_gouraud = draw_render_gouraud8;
	sbuffer_draw_f draw_render_textured = draw_render_textured8;

	/*check_sbuffer();*/

	if (SDL_MUSTLOCK(surf)) {
		SDL_LockSurface(surf);
	}

	switch(surf->format->BytesPerPixel) {
		case 2:
			draw_render_fill = draw_render_fill16;
			draw_render_gouraud = draw_render_gouraud16;
			draw_render_textured = draw_render_textured16;
			break;
		case 3:
			draw_render_fill = draw_render_fill24;
			draw_render_gouraud = draw_render_gouraud24;
			draw_render_textured = draw_render_textured24;
			break;
		case 4:
			draw_render_fill = draw_render_fill32;
			draw_render_gouraud = draw_render_gouraud32;
			draw_render_textured = draw_render_textured32;
			break;
		default:
			break;
	}

	dst += video.viewport.y * surf->pitch;
	dst += video.viewport.x * surf->format->BytesPerPixel;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		sbuffer_segdata_t *segdata = sbuffer_rows[i].segdata;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs_data; j++) {
			int last;
			sbuffer_segment_t *current = &segments[segdata[j].id];

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs_data-1) && (segdata[j].id==segdata[last+1].id);
				last++)
			{
			}

			if (!current->masking && (segdata[j].x1 <= segdata[last].x2)) {
				Uint8 *dst_line = dst + segdata[j].x1 * surf->format->BytesPerPixel;
				switch(current->render_mode) {
					case RENDER_FILLED:
						(*draw_render_fill)(surf, dst_line, current, segdata[j].x1, segdata[last].x2);
						break;
					case RENDER_GOURAUD:
						(*draw_render_gouraud)(surf, dst_line, current, segdata[j].x1, segdata[last].x2);
						break;
					case RENDER_TEXTURED:
						(*draw_render_textured)(surf, dst_line, current, segdata[j].x1, segdata[last].x2);
						break;
				}
			}

			j = last;
		}

		dst += surf->pitch;
	}

	if (SDL_MUSTLOCK(surf)) {
		SDL_UnlockSurface(surf);
	}

	clear_sbuffer();
}

static void draw_endFrame(draw_t *this)
{
	draw_flushFrame(this);
}

static void draw_render_fill8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	Uint32 color;
	int i;
	int r = segment->start.r;
	int g = segment->start.g;
	int b = segment->start.b;

	Uint8 *dst_col = dst_line;
	color = dither_nearest_index(r,g,b);

	memset(dst_col, color, x2 - x1 + 1);
}

static void draw_render_fill16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	Uint32 color;
	int i;
	int r = segment->start.r;
	int g = segment->start.g;
	int b = segment->start.b;

	Uint16 *dst_col = (Uint16 *) dst_line;
	color = SDL_MapRGB(surf->format, r,g,b);

	for (i=x1; i<=x2; i++) {
		*dst_col++ = color;
	}
}

static void draw_render_fill24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
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

static void draw_render_fill32(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	Uint32 color;
	int i;
	int r = segment->start.r;
	int g = segment->start.g;
	int b = segment->start.b;

	Uint32 *dst_col =  (Uint32 *) dst_line;
	color = SDL_MapRGB(surf->format, r,g,b);

	for (i=x1; i<=x2; i++) {
		*dst_col++ = color;
	}
}

static void draw_render_gouraud8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	int dx, dxtotal, i;
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

	dx = x2 - x1 + 1;
 
	for (i=0; i<dx; i++) {
		*dst_col++ = dither_nearest_index(r,g,b);
		r += dr;
		g += dg;
		b += db;
	}
}

static void draw_render_gouraud16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	int dx, dxtotal, i;
	Uint16 *dst_col = (Uint16 *) dst_line;

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

	dx = x2 - x1 + 1;
 
	for (i=0; i<dx; i++) {
		*dst_col++ = SDL_MapRGB(surf->format, r,g,b);
		r += dr;
		g += dg;
		b += db;
	}
}

static void draw_render_gouraud24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	int dx, dxtotal, i;
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

	dx = x2 - x1 + 1;
 
	for (i=0; i<dx; i++) {
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

static void draw_render_gouraud32(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	int dx, dxtotal, i;
	Uint32 *dst_col =  (Uint32 *) dst_line;

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

	dx = x2 - x1 + 1;
 
	for (i=0; i<dx; i++) {
		*dst_col++ = SDL_MapRGB(surf->format, r,g,b);
		r += dr;
		g += dg;
		b += db;
	}
}

static void draw_render_textured8(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
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

#if defined(__GNUC__) && (defined(__M68000__) || defined(__M68020__))
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
			for (i=0; i<dx; i++) {
				Uint8 c = tex->pixels[((int) v)*tex->pitchw + ((int) u)];

				if (c) {
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

static void draw_render_textured16(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	int dx, dxtotal, i;
	render_texture_t *tex = segment->texture;
	Uint16 *dst_col = (Uint16 *) dst_line;

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

#if defined(__GNUC__) && (defined(__M68000__) || defined(__M68020__))
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
			Uint32 vm = (0xffffffff>>vshift) & 0xffff0000;
			int rshift = 16-ushift;

			for (i=dx-1; i>=0; i--) {
				Uint32 uv = (vi & vm) | (ui & 0xffff);
				uv >>= rshift;
				ui += ud;
				*dst_col++ = tex_pixels[uv];
				vi += vd;
			}
		}
	} else
#endif
	{
		/* Float calculations */
		if (tex->paletted) {
			Uint32 *palette = tex->palettes[segment->tex_num_pal];
			for (i=0; i<dx; i++) {
				Uint8 c = tex->pixels[((int) v)*tex->pitchw + ((int) u)];

				if (c) {
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

static void draw_render_textured24(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
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

static void draw_render_textured32(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	int dx, dxtotal, i;
	render_texture_t *tex = segment->texture;
	Uint32 *dst_col =  (Uint32 *) dst_line;

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
				*dst_col = palette[c];
			}
			dst_col++;
			u += du;
			v += dv;
		}
	} else {
		Uint32 *tex_pixels = (Uint32 *) tex->pixels;

		for (i=0; i<dx; i++) {
			*dst_col++ = tex_pixels[((int)v)*tex->pitchw + ((int) u)];
			u += du;
			v += dv;
		}
	}
}

/* Calc w coordinate for a given x */
static float calc_w(const sbuffer_segment_t *segment, int x)
{
	int dx,nx;
	float dw;

	dx = segment->end.x - segment->start.x + 1;
	nx = x - segment->start.x;

	dw = segment->end.w - segment->start.w;

	return (segment->start.w + ((dw * nx)/dx));
}

/* Check if a segment is in front or behind another */
static int check_behind(const sbuffer_segment_t *seg1, const sbuffer_segment_t *seg2,
	int x1, int x2, int *cx)
{
	float s1w1,s1w2, s2w1,s2w2, dw1,dw2;
	int dx;

	s1w1 = calc_w(seg1, x1);
	s1w2 = calc_w(seg1, x2);
	s2w1 = calc_w(seg2, x1);
	s2w2 = calc_w(seg2, x2);

	DEBUG_PRINT(("%d->%d: seg1: %p %.3f->%.3f, seg2: %p %.3f->%.3f\n",
		x1,x2, seg1,s1w1*4096.0f,s1w2*4096.0f, seg2,s2w1*4096.0f,s2w2*4096.0f));

	/* Do we have an intersection ? */
	if (s1w1>s2w1) {
		if (s1w2>s2w2) {
			return SEG1_FRONT;
		}
	} else {
		if (s1w2<s2w2) {
			return SEG1_BEHIND;
		}
	}

	/* Calc X coordinate of intersection where W is the same for both segments */
	dx = x2 - x1 + 1;
	dw1 = s1w2 - s1w1;
	dw2 = s2w2 - s2w1;

	if (dw2 - dw1 < 0.0001f) {
		return SEG1_FRONT;
	}

	*cx = x1 + (((s1w1-s2w1)*dx)/(dw2-dw1));

	if (*cx == x1) {
		return (s1w2>s2w2 ? SEG1_FRONT : SEG1_BEHIND);
	} else if (*cx == x2) {
		return (s1w1>s2w1 ? SEG1_FRONT : SEG1_BEHIND);
	}

	return (s1w1>s2w1 ? SEG1_CLIP_LEFT : SEG1_CLIP_RIGHT);
}

#if 0
static void dump_sbuffer(void)
{
	int i,j;

	DEBUG_PRINT(("----------dump sbuffer start\n"));

	for (i=0; i<sbuffer_numrows; i++) {
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			DEBUG_PRINT(("----- seg %d: %d,%d\n", i,
				sbuffer_rows[i].segment[j].start.x, sbuffer_rows[i].segment[j].end.x));
		}
		for (j=0; j<sbuffer_rows[i].num_segs_data; j++) {
			DEBUG_PRINT(("----- data %d (seg %d): %d,%d\n", i, sbuffer_rows[i].segdata[j].id,
				sbuffer_rows[i].segdata[j].x1, sbuffer_rows[i].segdata[j].x2));
		}
	}

	DEBUG_PRINT(("----------dump sbuffer end\n"));
}
#endif

static void add_base_segment(int num_seg, int y, const sbuffer_segment_t *segment)
{
	sbuffer_segment_t *new_seg = &(sbuffer_rows[y].segment[num_seg]);

	new_seg->render_mode = segment->render_mode;
	new_seg->tex_num_pal = segment->tex_num_pal;
	new_seg->masking = segment->masking;
	new_seg->texture = segment->texture;

	memcpy(&(new_seg->start), &(segment->start), sizeof(sbuffer_point_t));
	memcpy(&(new_seg->end), &(segment->end), sizeof(sbuffer_point_t));
}

static void push_data_segment(int num_seg, int num_segdata, int y, int x1, int x2)
{
	sbuffer_segdata_t *new_segdata;

	if (num_segdata>=NUM_SEGMENTS_DATA) {
		return;
	}

	/* Write new segment data */
	new_segdata = &(sbuffer_rows[y].segdata[num_segdata]);

	new_segdata->id = num_seg;
	new_segdata->x1 = x1;
	new_segdata->x2 = x2;
}

static void insert_data_segment(int num_seg, int new_segdata, int y, int x1, int x2)
{
	int num_segs_data = sbuffer_rows[y].num_segs_data;

	/* Move stuff that starts after this segment */
	if (num_segs_data>0) {
		int i;
		int num_seg_copy = MIN(NUM_SEGMENTS_DATA-2, num_segs_data-1);
		sbuffer_segdata_t *src = &(sbuffer_rows[y].segdata[num_seg_copy]);
		sbuffer_segdata_t *dst = &(sbuffer_rows[y].segdata[num_seg_copy+1]);
		for (i=num_seg_copy; i>=new_segdata; i--) {
			dst->id = src->id;
			dst->x1 = src->x1;
			dst->x2 = src->x2;
			dst--;
			src--;
		}
	}

	push_data_segment(num_seg, new_segdata, y, x1,x2);
	if (num_segs_data<NUM_SEGMENTS_DATA) {
		++sbuffer_rows[y].num_segs_data;
	}
}

static void draw_add_segment(int y, const sbuffer_segment_t *segment)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	int x1,x2, i;
	int segbase_inserted = 0;
	int num_segs = row->num_segs;
	int clip_seg, clip_pos;

	x1 = segment->start.x;
	x2 = segment->end.x;

	/* Clip if outside */
	if ((x2<0) || (x1>=video.viewport.w) || (y<0) || (y>=video.viewport.h)) {
		return;
	}

	x1 = MAX(0, x1);
	x2 = MIN(video.viewport.w-1, x2);

	if (x2<x1) {
		return;
	}

	/* Still room for common segment data ? */
	if ((num_segs>=NUM_SEGMENTS) || (row->num_segs_data>=NUM_SEGMENTS_DATA)) {
		return;
	}
	add_base_segment(num_segs,y, segment);

	DEBUG_PRINT(("-------add segment %d %d,%d (seg %d segdata %d)\n", y, x1,x2,
		row->num_segs, row->num_segs_data));

	/*--- Trivial cases ---*/

	/* Empty row ? */
	if (num_segs == 0) {
		DEBUG_PRINT(("----empty list\n"));
		insert_data_segment(num_segs,0,y, x1,x2);
		++row->num_segs;
		return;
	}

	/* Finish before first ? */
	if (x2 < row->segdata[0].x1) {
		DEBUG_PRINT(("----finish before first (%d<%d)\n",x2,row->segdata[0].x1));
		insert_data_segment(num_segs,0,y, x1,x2);
		++row->num_segs;
		return;
	}

	/* Start after last ? */
	if (row->segdata[row->num_segs_data-1].x2 < x1) {
		DEBUG_PRINT(("----start after last (%d<%d)\n", row->segdata[row->num_segs_data-1].x2, x1));
		insert_data_segment(num_segs,row->num_segs_data,y, x1,x2);
		++row->num_segs;
		return;
	}

	/*--- Need to check against current list ---*/
	for (i=0; i<row->num_segs_data; i++) {
		int clip_x1, clip_x2, current_end, ic = i;
		sbuffer_segdata_t *current = &(row->segdata[ic]);

		DEBUG_PRINT(("--new %d,%d against %d:%d,%d\n",x1,x2, ic,current->x1, current->x2));

		/* Out of screen ? */
		if ((x2<0) || (x1>=video.viewport.w) || (x1>x2)) {
			DEBUG_PRINT(("  stop\n"));
			row->num_segs += segbase_inserted;
			return;
		}	

		/* Start after current? Will process against next one
		ccccccc
			nnnnn
		*/
		if (current->x2 < x1) {
			DEBUG_PRINT(("  start after %d\n",ic));
			continue;
		}

		/* Finish before current ? Insert before it
			ccccc
		nnnnnn
		  nnnnnnn
		*/
		if (x2 <= current->x1) {
			DEBUG_PRINT(("  finish before or equal start %d\n",ic));
			if (current->x1 > 0) {
				if (x2 == current->x1) {
					--x2;
				}
				insert_data_segment(num_segs,ic,y, x1,x2);
				segbase_inserted = 1;
			}
			row->num_segs += segbase_inserted;
			return;
		}

		/* Start before current, may finish after or in middle of it
		   Insert only non conflicting left part
			ccccccccc
		1 nnnnnnn
		2 nnnnnnnnnnn
		3 nnnnnnnnnnnnnnn
		4 nnnnnnnnnnnnnnnnn
		remains (to insert)
		1       n
		2       nnnnn
		3       nnnnnnnnn
		4       nnnnnnnnnnn
		*/
		if (x1 < current->x1) {
			int next_x1 = current->x1;
			DEBUG_PRINT(("  new start before %d, insert %d,%d, will continue from pos %d\n", ic, x1,next_x1-1, next_x1));
			/*printf("   current before: %d,%d\n", current->start.x, current->end.x);**/

			insert_data_segment(num_segs,ic,y, x1,next_x1-1);
			segbase_inserted = 1;

			++ic;

			x1 = next_x1;

			/* End of list ? */
			if (ic>=NUM_SEGMENTS_DATA) {
				break;
			}

			current = &(row->segdata[ic]);
			/*printf("   current after: %d,%d\n", current->start.x, current->end.x);*/
		}

		/* Now Zcheck both current and new segment */

		/* Single pixel for current ?
			c
		1	n
		2	nnnnnn
		remains
		1
		2	 nnnnn
		*/
		if (current->x1 == current->x2) {
			/* Replace current with new if current behind */
			int cur_x = current->x1;
			int next_x1 = current->x2+1;
			DEBUG_PRINT(("  current is single pixel, will continue from pos %d\n", next_x1));
			/*printf("   new w=%.3f, cur w=%.3f\n", calc_w(start, end, x1), current->start.w);*/
			if (calc_w(segment, x1) > calc_w(&(row->segment[current->id]), cur_x)) {
				DEBUG_PRINT(("   replace current by new\n"));

				push_data_segment(num_segs,ic,y, cur_x,cur_x);
				segbase_inserted=1;
			}
			x1 = next_x1;
			continue;
		}

		/* Single pixel for new ?
			cccccccccc
			    n
		*/
		if (x1 == x2) {
			DEBUG_PRINT((" new single pixel at %d\n", x1));

			/* Skip if we already inserted some part of it before */
			if (segbase_inserted) {
				DEBUG_PRINT(("  skip part already inserted\n"));
				row->num_segs += segbase_inserted;
				return;
			}

			/* Skip if new behind current */
			if (calc_w(&(row->segment[current->id]), x1) > calc_w(segment, x1)) {
				DEBUG_PRINT(("  new behind current, stop\n"));
				row->num_segs += segbase_inserted;
				return;
			}

			/*printf("  check %d:%d,%d against new %d\n", ic,current->start.x,current->end.x, x1);*/

			/* Insert new before current, clip current ?
				cccccccc
				n
			*/
			if (x1 == current->x1) {
				DEBUG_PRINT(("  clip current start from %d,%d at %d\n", current->x1,current->x2, x1+1));
				++current->x1;

				DEBUG_PRINT(("  insert new %d,%d at %d\n", x1,x2, ic));
				insert_data_segment(num_segs,ic,y, x1,x2);
				++row->num_segs;
				return;
			}

			/* Clip current, insert new after current ?
				cccccccc
				       n
			*/
			if (x2 == current->x2) {
				DEBUG_PRINT(("  clip current end from %d,%d at %d\n", current->x1,current->x2, x2-1));
				--current->x2;

				DEBUG_PRINT(("  insert new %d,%d at %d\n", x1,x2, ic+1));
				insert_data_segment(num_segs,ic+1,y, x1,x2);
				++row->num_segs;
				return;
			}

			/* Split current to insert new between both halves
				cccccccc
				   n
			becomes
				ccc
				   n
				    cccc
			*/
			DEBUG_PRINT(("  split current, insert new\n"));
			insert_data_segment(current->id,ic+1,y, x1+1,current->x2);

			current->x2 = x1-1;

			insert_data_segment(num_segs,ic+1,y, x1,x2);
			++row->num_segs;
			return;
		}

		/* Z check for multiple pixels
			ccccccccc
		1       nnnnn
		2	   nnnnn
		3	    nnnnnnnnnn
		4	        nnnnnn
		*/
		clip_x1 = MAX(x1, current->x1);
		clip_x2 = MIN(x2, current->x2);

		if (clip_x1==clip_x2) {
			DEBUG_PRINT((" Zcheck multiple pixels, single pixel common zone\n"));
			/* Skip if new behind current */
			if (calc_w(&(row->segment[current->id]), clip_x1) > calc_w(segment, clip_x1)) {
				DEBUG_PRINT(("  new behind current, continue from %d\n", clip_x1+1));
				x1 = clip_x1+1;
				continue;
			}

			/* Clip current if behind new */
			DEBUG_PRINT(("  clip current end from %d,%d at %d\n", current->x1,current->x2, clip_x1-1));
			current->x2 = clip_x1-1;
			x1 = clip_x1;
			continue;
		}

		clip_seg = check_behind(&(row->segment[current->id]),segment, clip_x1,clip_x2, &clip_pos);

		switch(clip_seg) {
			case SEG1_BEHIND:
				DEBUG_PRINT(("- %d behind new (common from %d->%d)\n", ic,clip_x1,clip_x2));
				current_end = current->x2;

				if (clip_x1 == current->x1) {
					if (current_end <= clip_x2) {
						DEBUG_PRINT((" new replace current from %d->%d\n", current->x1,current_end));
						/* Replace current by new */
						push_data_segment(num_segs,ic,y, current->x1,current_end);
						segbase_inserted=1;
					} else {
						DEBUG_PRINT((" clip current start from %d to %d\n", current->x1,clip_x2+1));
						/* Clip current on the right */
						current->x1 = clip_x2+1;

						DEBUG_PRINT((" insert new from %d->%d\n", clip_x1,clip_x2));
						/* Insert new before current */
						insert_data_segment(num_segs,ic,y, clip_x1,clip_x2);
						segbase_inserted=1;
					}
				} else {
					/* Insert current after clip_x2 ? */
					if (clip_x2 < current_end) {
						DEBUG_PRINT((" split current from %d->%d\n", clip_x2+1, current->x2));
						insert_data_segment(current->id,ic+1,y, clip_x2+1,current->x2);
						segbase_inserted=1;
					}

					DEBUG_PRINT((" insert new from %d->%d\n", clip_x1,clip_x2));
					/* Insert new */
					insert_data_segment(num_segs,ic+1,y, clip_x1,clip_x2);
					segbase_inserted=1;

					DEBUG_PRINT((" clip current end from %d to %d\n", current_end, clip_x1-1));
					/* Clip current before clip_x1 */
					current->x2 = clip_x1-1;
				}

				/* Continue with remaining */
				x1 = current_end+1;
				break;
			case SEG1_FRONT:
				DEBUG_PRINT(("- %d in front of new\n", ic));
				/* Continue with remaining part */
				x1 = current->x2+1;
				break;
			case SEG1_CLIP_LEFT:
				/*if ((clip_pos<clip_x1) || (clip_pos>clip_x2)) {
					printf("- check Z for %d:%d->%d against %d->%d (%d->%d at %d)\n",ic,
						current->x1,current->x2, x1,x2, clip_x1,clip_x2, clip_pos
					);
				}*/

				DEBUG_PRINT(("- keep left of %d against new at pos %d\n", ic, clip_pos));

				current_end = current->x2;

				/* Insert right part of current, after common zone */
				if (x2 < current->x2) {
					DEBUG_PRINT(("  insert right part of current %d (%d,%d)\n", ic, x2+1,current->x2));
					insert_data_segment(current->id,ic+1,y, x2+1,current->x2);
					segbase_inserted=1;
				}

				/* Clip current before clip_pos */
				DEBUG_PRINT(("  clip end of %d (%d,%d) at %d\n", ic, current->x1,current->x2, clip_pos-1));
				current->x2 = clip_pos-1;

				/* Continue with remaining part */
				x1 = clip_pos;
				break;
			case SEG1_CLIP_RIGHT:
				/*if ((clip_pos<clip_x1) || (clip_pos>clip_x2))*/ {
					DEBUG_PRINT(("- check Z for %d:%d->%d against %d->%d (%d->%d at %d)\n",ic,
						current->x1,current->x2, x1,x2, clip_x1,clip_x2, clip_pos
					));
				}

				DEBUG_PRINT(("- keep right of %d against new at pos %d\n", ic, clip_pos));

				current_end = current->x2;

				/* Insert left part of current, before common zone */
				if (current->x1 < x1) {
					DEBUG_PRINT(("  insert left part of current %d (%d,%d)\n", ic, current->x1,x1-1));
					insert_data_segment(current->id,ic,y, current->x1,x1-1);
					segbase_inserted=1;

					if (ic>=NUM_SEGMENTS_DATA) {
						break;
					}

					current = &(row->segdata[ic]);
				}
				/* Clip current */
				DEBUG_PRINT(("  clip start of %d (%d,%d) at %d\n", ic, current->x1,current->x2, clip_pos+1));
				current->x1 = clip_pos+1;

				/* Insert new */
				DEBUG_PRINT(("  insert %d,%d at %d\n", x1,clip_pos, ic+1));
				insert_data_segment(num_segs,ic,y, x1,clip_pos);
				segbase_inserted=1;

				/* Continue with remaining part */
				x1 = current_end+1;
				break;
		}
	}

	DEBUG_PRINT(("--remain %d,%d\n",x1,x2));
	if (x1>x2) {
		DEBUG_PRINT((" stop\n"));
		row->num_segs += segbase_inserted;
		return;
	}

	/* Insert last */
	insert_data_segment(num_segs,row->num_segs_data,y, x1,x2);
	++row->num_segs;
}

static void draw_poly_sbuffer(draw_t *this, vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2;
	sbuffer_segment_t segment;
	int num_array = 1; /* max array */

	if (video.viewport.h>size_poly_minmaxx) {
		poly_hlines = realloc(poly_hlines, sizeof(poly_hline_t) * video.viewport.h);
		size_poly_minmaxx = video.viewport.h;
	}

	if (!poly_hlines) {
		fprintf(stderr, "Not enough memory for poly rendering\n");
		return;
	}

	if (video.viewport.h>sbuffer_numrows) {
		sbuffer_rows = realloc(sbuffer_rows, sizeof(sbuffer_row_t) * video.viewport.h);
		sbuffer_numrows = video.viewport.h;
	}

	if (!sbuffer_rows) {
		fprintf(stderr, "Not enough memory for Sbuffer rendering\n");
		return;
	}

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy;
		float w1, w2;

		num_array = 1; /* max */

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		w1 = 1.0f /*vtx[p1].pos[3]*/ / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];
		w2 = 1.0f /*vtx[p2].pos[3]*/ / vtx[p2].pos[2];

		/*printf("%d,%d (%.3f) -> %d,%d (%.3f)\n",
			x1,y1,w1, x2,y2,w2);*/

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			int tmp;
			float tmpz;
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			tmpz = w1; w1 = w2; w2 = tmpz;
			num_array = 0;	/* min */
			v1 = p2;
			v2 = p1;
		}
		if (y1 < miny) {
			miny = y1;
		}
		if (y2 > maxy) {
			maxy = y2;
		}

		/*DEBUG_PRINT(("from p[%d]: u=%.3f, v=%.3f to p[%d]: u=%.3f,v=%.3f\n",
			v1, vtx[v1].tx[0], vtx[v1].tx[1],
			v2, vtx[v2].tx[0], vtx[v2].tx[1]
		));*/

		dy = y2 - y1;
		if (dy>0) {
			int dx = x2 - x1;
			float r1 = vtx[v1].col[0];
			float dr = vtx[v2].col[0] - r1;
			float g1 = vtx[v1].col[1];
			float dg = vtx[v2].col[1] - g1;
			float b1 = vtx[v1].col[2];
			float db = vtx[v2].col[2] - b1;
			float tu1 = vtx[v1].tx[0];
			float du = vtx[v2].tx[0] - tu1;
			float tv1 = vtx[v1].tx[1];
			float dv = vtx[v2].tx[1] - tv1;
			float dw = w2 - w1;
			if (drawCorrectPerspective>0) {
				r1 *= w1;
				dr = vtx[v2].col[0]*w2 - r1;
				g1 *= w1;
				dg = vtx[v2].col[1]*w2 - g1;
				b1 *= w1;
				db = vtx[v2].col[2]*w2 - b1;

				tu1 *= w1;
				du = vtx[v2].tx[0]*w2 - tu1;
				tv1 *= w1;
				dv = vtx[v2].tx[1]*w2 - tv1;
			}
			for (y=0; y<dy; y++) {
				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}

				poly_hlines[y1].sbp[num_array].r = r1 + ((dr*y)/dy);
				poly_hlines[y1].sbp[num_array].g = g1 + ((dg*y)/dy);
				poly_hlines[y1].sbp[num_array].b = b1 + ((db*y)/dy);
				poly_hlines[y1].sbp[num_array].u = tu1 + ((du*y)/dy);
				poly_hlines[y1].sbp[num_array].v = tv1 + ((dv*y)/dy);
				poly_hlines[y1].sbp[num_array].w = w1 + ((dw*y)/dy);
				poly_hlines[y1++].sbp[num_array].x = x1 + ((dx*y)/dy);

				/*DEBUG_PRINT(("line %d, side %d, %.3f,%.3f tu1=%.3f,tv1=%.3f,du=%.3f,dv=%.3f,dy=%d %.3f,%.3f\n",
					y1-1,num_array,
					poly_hlines[y1-1].sbp[num_array].u,poly_hlines[y1-1].sbp[num_array].v,
					tu1,tv1,du,dv,dy,(du*y)/dy,tu1 + ((du*y)/dy)
				));*/
			}
		}

		p1 = p2;
	}

	/* Render horizontal lines */
	if (miny<0) {
		miny = 0;
	}
	if (maxy>=video.viewport.h) {
		maxy = video.viewport.h;
	}

	/* Copy to other array for a single segment */
	if (num_vtx==2) {
		for (y=miny; y<maxy; y++) {
			memcpy(	&poly_hlines[y].sbp[num_array ^ 1],
				&poly_hlines[y].sbp[num_array],
				sizeof(sbuffer_point_t));
		}
	}

	segment.render_mode = render.render_mode;
	segment.tex_num_pal = render.tex_pal;
	segment.texture = render.texture;
	segment.masking = render.bitmap.masking;

	for (y=miny; y<maxy; y++) {
		int pminx = poly_hlines[y].sbp[0].x;
		int pmaxx = poly_hlines[y].sbp[1].x;
		if (pminx<minx) {
			minx = pminx;
		}
		if (pmaxx>maxx) {
			maxx = pmaxx;
		}

		memcpy(&segment.start, &poly_hlines[y].sbp[0], sizeof(sbuffer_point_t));
		memcpy(&segment.end, &poly_hlines[y].sbp[1], sizeof(sbuffer_point_t));

		draw_add_segment(y, &segment);
	}

	/*dump_sbuffer();*/

	if (minx<0) {
		minx = 0;
	}
	if (maxx>=video.viewport.w) {
		maxx = video.viewport.w;
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

static void draw_mask_segment(draw_t *this, int y, int x1, int x2, float w)
{
	sbuffer_segment_t segment;

	if ((y<0) || (y>=video.viewport.h)) {
		return;
	}
	if ((x1>=video.viewport.w) || (x2<0)) {
		return;
	}

	if (video.viewport.h>size_poly_minmaxx) {
		poly_hlines = realloc(poly_hlines, sizeof(poly_hline_t) * video.viewport.h);
		size_poly_minmaxx = video.viewport.h;
	}

	if (!poly_hlines) {
		fprintf(stderr, "Not enough memory for poly rendering\n");
		return;
	}

	if (video.viewport.h>sbuffer_numrows) {
		sbuffer_rows = realloc(sbuffer_rows, sizeof(sbuffer_row_t) * video.viewport.h);
		sbuffer_numrows = video.viewport.h;
	}

	if (!sbuffer_rows) {
		fprintf(stderr, "Not enough memory for Sbuffer rendering\n");
		return;
	}

	/*printf("mask segment %d: %d,%d\n",y,x1,x2);*/

	segment.start.x = x1;
	segment.end.x = x2;
	segment.start.w = segment.end.w = w;
	segment.start.r = segment.end.r = 0xff;
	segment.start.g = segment.end.g = 0;
	segment.start.b = segment.end.b = 0xff;
	segment.render_mode = RENDER_FILLED;
	segment.tex_num_pal = 0;
	segment.texture = NULL;
	segment.masking = 0 /*render.bitmap.masking*/;

	draw_add_segment(y, &segment);

	/* Upper layer will update dirty rectangles */
}
