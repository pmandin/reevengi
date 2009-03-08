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

#include "video.h"
#include "parameters.h"
#include "dither.h"
#include "render.h"
#include "draw.h"

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

#define NUM_SEGMENTS 64

#define SEG1_FRONT 0
#define SEG1_BEHIND 1
#define SEG1_CLIP_LEFT 2
#define SEG1_CLIP_RIGHT 3

#define SEG_MIN(x1, x2) \
	( x1 < x2 ? x1 : x2)
#define SEG_MAX(x1, x2) \
	( x1 > x2 ? x1 : x2)

/*--- Types ---*/

typedef struct {
	int x;		/* x on screen*/
	float r,g,b;	/* color */
	float u,v;	/* u,v coords */
	float w;	/* w=1/z */
} sbuffer_point_t;

typedef struct {
	int x[2];	/* x 0:min, 1:max */
	Uint32 c[2];	/* color 0:min, 1:max */
	float tu[2];	/* texture 0:min, 1:max */
	float tv[2];
	float w[2];	/* 1/z, 0:min, 1:max */

	sbuffer_point_t sbp[2]; /* 0:min, 1:max */
} poly_hline_t;

typedef struct {
	Uint32	id;	/* ID to merge segments */
	int tex_num_pal;
	render_texture_t *texture;
	sbuffer_point_t start, end;
} sbuffer_segment_t;

typedef struct {
	int num_segs;
	sbuffer_segment_t segment[NUM_SEGMENTS];
} sbuffer_row_t;

/*--- Variables ---*/

static Uint32 draw_color = 0;

/* texture */
static int tex_num_pal;
static render_texture_t *texture = NULL;

/* for poly rendering */
static int size_poly_minmaxx = 0;
static poly_hline_t *poly_hlines = NULL;

/* Sbuffer */
static int sbuffer_numrows = 0;
static sbuffer_row_t *sbuffer_rows = NULL;
static Uint32 sbuffer_seg_id;

static int drawCorrectPerspective = 0; /* 0:none, 1:per scanline, 2:every 16 pixels */

/*--- Functions prototypes ---*/

static void draw_render8_fill(void);
static void draw_render16_fill(void);
static void draw_render32_fill(void);

static void draw_render8_gouraud(void);
static void draw_render16_gouraud(void);
static void draw_render32_gouraud(void);

static void draw_render8_tex(void);
static void draw_render16_tex(void);
static void draw_render32_tex(void);

/*static void draw_clip_segment(int x, const sbuffer_point_t *start, const sbuffer_point_t *end,
	sbuffer_point_t *clipped);
static void draw_add_segment(int y, const sbuffer_point_t *start, const sbuffer_point_t *end);*/

static void draw_hline(int x1, int x2, int y);
static void draw_hline_gouraud(int x1, int x2, int y, Uint32 c1, Uint32 c2);
static void draw_hline_tex(int x1, int x2, int y, float tu1, float tv1, float tu2, float tv2);

static int clipEncode (int x, int y, int left, int top, int right, int bottom);
static int clip_line(int *x1, int *y1, int *x2, int *y2);

/*--- Functions ---*/

void draw_init(void)
{
}

void draw_shutdown(void)
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

/* Sbuffer functions */
void draw_clear(void)
{
	int i;

	for (i=0; i<sbuffer_numrows; i++) {
		sbuffer_rows[i].num_segs = 0;
	}
	sbuffer_seg_id = 0;
}

void draw_resize(int w, int h)
{
	if (h>sbuffer_numrows) {
		sbuffer_rows = realloc(sbuffer_rows, h * sizeof(sbuffer_row_t));
		sbuffer_numrows = h;
	}

	draw_clear();
}

void draw_render(void)
{
	SDL_Surface *surf = video.screen;

	switch(render.render_mode) {
		case RENDER_FILLED:
			switch(surf->format->BytesPerPixel) {
				case 1:
					draw_render8_fill();
					break;
				case 2:
					draw_render16_fill();
					break;
				case 3:
					/* TODO */
					break;
				case 4:
					draw_render32_fill();
					break;
			}
			break;
		case RENDER_GOURAUD:
			switch(surf->format->BytesPerPixel) {
				case 1:
					draw_render8_gouraud();
					break;
				case 2:
					draw_render16_gouraud();
					break;
				case 3:
					/* TODO */
					break;
				case 4:
					draw_render32_gouraud();
					break;
			}
			break;
		case RENDER_TEXTURED:
			switch(surf->format->BytesPerPixel) {
				case 1:
					draw_render8_tex();
					break;
				case 2:
					draw_render16_tex();
					break;
				case 3:
					/* TODO */
					break;
				case 4:
					draw_render32_tex();
					break;
			}
			break;
	}
}

static void draw_render8_fill(void)
{
	int i,j;
	SDL_Surface *surf = video.screen;
	Uint8 *dst = (Uint8 *) surf->pixels;
	dst += video.viewport.y * surf->pitch;
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint8 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint8 *dst_col = &dst_line[segments[j].start.x];
			Uint8 color;
			int last;
			int r = segments[j].start.r;
			int g = segments[j].start.g;
			int b = segments[j].start.b;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			if (drawCorrectPerspective>0) {
				r = segments[j].start.r / segments[j].start.w;
				g = segments[j].start.g / segments[j].start.w;
				b = segments[j].start.b / segments[j].start.w;
			}
 			color = dither_nearest_index(r,g,b);
 
			memset(dst_col, color, segments[last].end.x - segments[j].start.x + 1);
			j = last;
		}

		dst += surf->pitch;
	}
}

static void draw_render16_fill(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint16 *dst = (Uint16 *) surf->pixels;
	dst += video.viewport.y * (surf->pitch>>1);
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint16 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint16 *dst_col = &dst_line[segments[j].start.x];
			Uint32 color;
			int last;
			int r = segments[j].start.r;
			int g = segments[j].start.g;
			int b = segments[j].start.b;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			if (drawCorrectPerspective>0) {
				r = segments[j].start.r / segments[j].start.w;
				g = segments[j].start.g / segments[j].start.w;
				b = segments[j].start.b / segments[j].start.w;
			}
			color = SDL_MapRGB(surf->format, r,g,b);

			for (k=segments[j].start.x; k<=segments[last].end.x; k++) {
				*dst_col++ = color;
			}
			j = last;
		}

		dst += surf->pitch>>1;
	}
}

static void draw_render32_fill(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint32 *dst = (Uint32 *) surf->pixels;
	dst += video.viewport.y * (surf->pitch>>2);
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint32 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint32 *dst_col = &dst_line[segments[j].start.x];
			Uint32 color;
			int last;
			int r = segments[j].start.r;
			int g = segments[j].start.g;
			int b = segments[j].start.b;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			if (drawCorrectPerspective>0) {
				r = segments[j].start.r / segments[j].start.w;
				g = segments[j].start.g / segments[j].start.w;
				b = segments[j].start.b / segments[j].start.w;
			}
			color = SDL_MapRGB(surf->format, r,g,b);

			for (k=segments[j].start.x; k<=segments[last].end.x; k++) {
				*dst_col++ = color;
			}
			j = last;
		}

		dst += surf->pitch>>2;
	}
}

static void draw_render8_gouraud(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint8 *dst = (Uint8 *) surf->pixels;
	dst += video.viewport.y * surf->pitch;
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint8 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint8 *dst_col = &dst_line[segments[j].start.x];
			int dx,last, r1,g1,b1, r2,g2,b2, dr,dg,db;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			r1 = segments[j].start.r;
			g1 = segments[j].start.g;
			b1 = segments[j].start.b;
			r2 = segments[last].end.r;
			g2 = segments[last].end.g;
			b2 = segments[last].end.b;

			if (drawCorrectPerspective>0) {
				r1 = segments[j].start.r / segments[j].start.w;
				g1 = segments[j].start.g / segments[j].start.w;
				b1 = segments[j].start.b / segments[j].start.w;
				r2 = segments[last].end.r / segments[last].end.w;
				g2 = segments[last].end.g / segments[last].end.w;
				b2 = segments[last].end.b / segments[last].end.w;
			}

			dr = r2-r1;
			dg = g2-g1;
			db = b2-b1;
			dx = segments[last].end.x - segments[j].start.x + 1;
 
			for (k=0; k<dx; k++) {
				int r = r1 + ((dr*k)/dx);
				int g = g1 + ((dg*k)/dx);
				int b = b1 + ((db*k)/dx);
				*dst_col++ = dither_nearest_index(r,g,b);
			}
			j = last;
		}

		dst += surf->pitch;
	}
}

static void draw_render16_gouraud(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint16 *dst = (Uint16 *) surf->pixels;
	dst += video.viewport.y * (surf->pitch>>1);
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint16 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint16 *dst_col = &dst_line[segments[j].start.x];
			int dx,last, r1,g1,b1, r2,g2,b2, dr,dg,db;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			r1 = segments[j].start.r;
			g1 = segments[j].start.g;
			b1 = segments[j].start.b;
			r2 = segments[last].end.r;
			g2 = segments[last].end.g;
			b2 = segments[last].end.b;

			if (drawCorrectPerspective>0) {
				r1 = segments[j].start.r / segments[j].start.w;
				g1 = segments[j].start.g / segments[j].start.w;
				b1 = segments[j].start.b / segments[j].start.w;
				r2 = segments[last].end.r / segments[last].end.w;
				g2 = segments[last].end.g / segments[last].end.w;
				b2 = segments[last].end.b / segments[last].end.w;
			}

			dr = r2-r1;
			dg = g2-g1;
			db = b2-b1;
			dx = segments[last].end.x - segments[j].start.x + 1;
 
			for (k=0; k<dx; k++) {
				int r = r1 + ((dr*k)/dx);
				int g = g1 + ((dg*k)/dx);
				int b = b1 + ((db*k)/dx);
				*dst_col++ = SDL_MapRGB(surf->format, r,g,b);
			}
			j = last;
		}

		dst += surf->pitch>>1;
	}
}

static void draw_render32_gouraud(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint32 *dst = (Uint32 *) surf->pixels;
	dst += video.viewport.y * (surf->pitch>>2);
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint32 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint32 *dst_col = &dst_line[segments[j].start.x];
			int dx,last, r1,g1,b1, r2,g2,b2, dr,dg,db;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			r1 = segments[j].start.r;
			g1 = segments[j].start.g;
			b1 = segments[j].start.b;
			r2 = segments[last].end.r;
			g2 = segments[last].end.g;
			b2 = segments[last].end.b;

			if (drawCorrectPerspective>0) {
				r1 = segments[j].start.r / segments[j].start.w;
				g1 = segments[j].start.g / segments[j].start.w;
				b1 = segments[j].start.b / segments[j].start.w;
				r2 = segments[last].end.r / segments[last].end.w;
				g2 = segments[last].end.g / segments[last].end.w;
				b2 = segments[last].end.b / segments[last].end.w;
			}

			dr = r2-r1;
			dg = g2-g1;
			db = b2-b1;
			dx = segments[last].end.x - segments[j].start.x + 1;
 
			for (k=0; k<dx; k++) {
				int r = r1 + ((dr*k)/dx);
				int g = g1 + ((dg*k)/dx);
				int b = b1 + ((db*k)/dx);

				*dst_col++ = SDL_MapRGB(surf->format, r,g,b);
			}
			j = last;
		}

		dst += surf->pitch>>2;
	}
}

static void draw_render8_tex(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint8 *dst = (Uint8 *) surf->pixels;
	dst += video.viewport.y * surf->pitch;
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint8 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint8 *dst_col = &dst_line[segments[j].start.x];
			int dx, u1,v1, u2,v2, du,dv, last;

			render_texture_t *tex = segments[j].texture;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			u1 = segments[j].start.u;
			v1 = segments[j].start.v;
			u2 = segments[last].end.u;
			v2 = segments[last].end.v;

			if (drawCorrectPerspective>0) {
				u1 = segments[j].start.u / segments[j].start.w;
				v1 = segments[j].start.v / segments[j].start.w;
				u2 = segments[last].end.u / segments[last].end.w;
				v2 = segments[last].end.v / segments[last].end.w;
			}

			du = u2-u1;
			dv = v2-v1;
			dx = segments[last].end.x - segments[j].start.x + 1;
 
			if (tex->paletted) {
				Uint32 *palette = tex->palettes[segments[j].tex_num_pal];
				for (k=0; k<dx; k++) {
					int u = u1 + ((du*k)/dx);
					int v = v1 + ((dv*k)/dx);
					*dst_col++ = palette[tex->pixels[v*tex->pitchw + u]];
				}
			}/* else {
				Uint16 *tex_pixels = (Uint16 *) tex->pixels;
			
				for (k=0; k<dx; k++) {
					int u = u1 + ((du*k)/dx);
					int v = v1 + ((dv*k)/dx);
					*dst_col++ = tex_pixels[v*tex->pitchw + u];
				}
			}*/

			j = last;
		}

		dst += surf->pitch;
	}
}

static void draw_render16_tex(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint16 *dst = (Uint16 *) surf->pixels;
	dst += video.viewport.y * (surf->pitch>>1);
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint16 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			DEBUG_PRINT((" [%d:0x%08x] %d->%d %d %p\n", j, sbuffer_rows[i].segment[j].id,
				sbuffer_rows[i].segment[j].start.x, sbuffer_rows[i].segment[j].end.x,
				sbuffer_rows[i].segment[j].tex_num_pal, sbuffer_rows[i].segment[j].texture
			));
		}

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint16 *dst_col = &dst_line[segments[j].start.x];
			int dx, u1,v1, u2,v2, du,dv, last;

			render_texture_t *tex = segments[j].texture;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			u1 = segments[j].start.u;
			v1 = segments[j].start.v;
			u2 = segments[last].end.u;
			v2 = segments[last].end.v;
			if (drawCorrectPerspective>0) {
				u1 = segments[j].start.u / segments[j].start.w;
				v1 = segments[j].start.v / segments[j].start.w;
				u2 = segments[last].end.u / segments[last].end.w;
				v2 = segments[last].end.v / segments[last].end.w;
			}

			du = u2-u1;
			dv = v2-v1;
			dx = segments[last].end.x - segments[j].start.x + 1;
 
			DEBUG_PRINT(("line %d, segment %d->%d (%d,%d), from %d,%d to %d,%d, %d %p\n",
				i,j,last, segments[j].start.x,segments[last].end.x, u1,v1,u2,v2,
				segments[j].tex_num_pal,tex
			));

			if (tex->paletted) {
				Uint32 *palette = tex->palettes[segments[j].tex_num_pal];
				for (k=0; k<dx; k++) {
					int u = u1 + ((du*k)/dx);
					int v = v1 + ((dv*k)/dx);
					/*DEBUG_PRINT(("%d: %d,%d->%d,%d: %d,%d\n",k,u1,v1,u2,v2,u,v));*/
					*dst_col++ = palette[tex->pixels[v*tex->pitchw + u]];
				}
			} else {
				Uint16 *tex_pixels = (Uint16 *) tex->pixels;
			
				for (k=0; k<dx; k++) {
					int u = u1 + ((du*k)/dx);
					int v = v1 + ((dv*k)/dx);
					*dst_col++ = tex_pixels[v*tex->pitchw + u];
				}
			}

			j = last;
		}

		dst += surf->pitch>>1;
	}
}

static void draw_render32_tex(void)
{
	int i,j,k;
	SDL_Surface *surf = video.screen;
	Uint32 *dst = (Uint32 *) surf->pixels;
	dst += video.viewport.y * (surf->pitch>>2);
	dst += video.viewport.x;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++) {
		Uint32 *dst_line = dst;
		sbuffer_segment_t *segments = sbuffer_rows[i].segment;

		/* Render list of segment */
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			Uint32 *dst_col = &dst_line[segments[j].start.x];
			int dx, u1,v1, u2,v2, du,dv, last;

			render_texture_t *tex = segments[j].texture;

			/* Find last segment to merge */
			for (last=j;
				(last<sbuffer_rows[i].num_segs-1) && (segments[j].id==segments[last+1].id);
				last++)
			{
			}

			u1 = segments[j].start.u;
			v1 = segments[j].start.v;
			u2 = segments[last].end.u;
			v2 = segments[last].end.v;

			if (drawCorrectPerspective>0) {
				u1 = segments[j].start.u / segments[j].start.w;
				v1 = segments[j].start.v / segments[j].start.w;
				u2 = segments[last].end.u / segments[last].end.w;
				v2 = segments[last].end.v / segments[last].end.w;
			}

			du = u2-u1;
			dv = v2-v1;
			dx = segments[last].end.x - segments[j].start.x + 1;
 
			if (tex->paletted) {
				Uint32 *palette = tex->palettes[segments[j].tex_num_pal];
				for (k=0; k<dx; k++) {
					int u = u1 + ((du*k)/dx);
					int v = v1 + ((dv*k)/dx);
					*dst_col++ = palette[tex->pixels[v*tex->pitchw + u]];
				}
			} else {
				Uint32 *tex_pixels = (Uint32 *) tex->pixels;
			
				for (k=0; k<dx; k++) {
					int u = u1 + ((du*k)/dx);
					int v = v1 + ((dv*k)/dx);
					*dst_col++ = tex_pixels[v*tex->pitchw + u];
				}
			}

			j = last;
		}

		dst += surf->pitch>>2;
	}
}

/* clipped can be start or end */
static void draw_clip_segment(const sbuffer_segment_t *segment, int x, sbuffer_point_t *clipped)
{
	int dx,nx;
	float dr,dg,db, du,dv, dw;

	dx = segment->end.x - segment->start.x + 1;
	nx = x - segment->start.x;

	dr = segment->end.r - segment->start.r;
	clipped->r = segment->start.r + ((dr * nx)/dx);
	dg = segment->end.g - segment->start.g;
	clipped->g = segment->start.g + ((dg * nx)/dx);
	db = segment->end.b - segment->start.b;
	clipped->b = segment->start.b + ((db * nx)/dx);

	du = segment->end.u - segment->start.u;
	clipped->u = segment->start.u + ((du * nx)/dx);
	dv = segment->end.v - segment->start.v;
	clipped->v = segment->start.v + ((dv * nx)/dx);

	dw = segment->end.w - segment->start.w;
	clipped->w = segment->start.w + ((dw * nx)/dx);

	clipped->x = x;
}

/* Push a segment at a given pos, overwriting previous */
static int draw_push_segment(const sbuffer_segment_t *segment,
	int y,int pos, int x1,int x2)
{
	sbuffer_point_t *p;

#if 0
	/* Merge against previous segment ? */
	if (pos>0) {
		if ((sbuffer_rows[y].segment[pos-1].id == segment->id)
		   && (x1-1 == sbuffer_rows[y].segment[pos-1].end.x))
		{
			DEBUG_PRINT(("merge %d(%d,%d) and %d(%d,%d)\n",
				pos-1, sbuffer_rows[y].segment[pos-1].start.x, sbuffer_rows[y].segment[pos-1].end.x,
				pos, x1,x2));

			p = &(sbuffer_rows[y].segment[pos-1].end);
			memcpy(p, &(segment->end), sizeof(sbuffer_point_t));
			draw_clip_segment(segment, x2, p);

			return 0;
		}		   
	}
#endif

	p = &(sbuffer_rows[y].segment[pos].start);
	memcpy(p, &(segment->start), sizeof(sbuffer_point_t));
	draw_clip_segment(segment, x1, p);
	
	p = &(sbuffer_rows[y].segment[pos].end);
	memcpy(p, &(segment->end), sizeof(sbuffer_point_t));
	draw_clip_segment(segment, x2, p);

	sbuffer_rows[y].segment[pos].id = segment->id;
	sbuffer_rows[y].segment[pos].tex_num_pal = segment->tex_num_pal;
	sbuffer_rows[y].segment[pos].texture = segment->texture;

	return 1;
}

/* Move all remaining segments further, then push */
static int draw_insert_segment(const sbuffer_segment_t *segment,
	int y,int pos, int x1,int x2)
{
	int num_segs = sbuffer_rows[y].num_segs;
	int last_seg = (num_segs>= NUM_SEGMENTS ? NUM_SEGMENTS : num_segs);
	int i;
	/*sbuffer_point_t *p;*/

#if 0
	/* Merge against previous segment ? */
	if (pos>0) {
		if ((sbuffer_rows[y].segment[pos-1].id == segment->id)
		   && (x1-1 == sbuffer_rows[y].segment[pos-1].end.x))
		{
			DEBUG_PRINT(("merge %d and %d\n", pos-1, pos));

			p = &(sbuffer_rows[y].segment[pos-1].end);
			memcpy(p, (&current->end), sizeof(sbuffer_point_t));
			draw_clip_segment(segment, x2, p);

			return 0;
		}		   
	}
#endif

#if 0
	/* Merge against current segment ? does not work, need to return 'merged' or 'inserted' */
	if ((sbuffer_rows[y].segment[pos].id == segment->id)
	   && (x2+1 == sbuffer_rows[y].segment[pos].start.x))
	{
		DEBUG_PRINT(("merge new and %d\n", pos));

		p = &(sbuffer_rows[y].segment[pos].start);
		memcpy(p, &(current->start), sizeof(sbuffer_point_t));
		draw_clip_segment(segment, x1, p);
		return 0;
	}
#endif

	/*printf("%d: copy segs %d to %d\n", y,pos, last_seg);*/
	for (i=last_seg-1; i>=pos; i--) {
		memcpy(&(sbuffer_rows[y].segment[i+1]), &(sbuffer_rows[y].segment[i]), sizeof(sbuffer_segment_t));
	}

	/* Increment only if not merged against previous */
	if (draw_push_segment(segment, y,pos, x1,x2)) {
		++sbuffer_rows[y].num_segs;
	}

	return 1;
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

	DEBUG_PRINT(("%d->%d: seg1: %.3f->%.3f, seg2:%.3f->%.3f\n",
		x1,x2, s1w1*4096.0f,s1w2*4096.0f, s2w1*4096.0f,s2w2*4096.0f));

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

	*cx = x1 + (((s1w1-s2w1)*dx)/(dw2-dw1));

	if (*cx == x1) {
		return (s1w2>s2w2 ? SEG1_FRONT : SEG1_BEHIND);
	} else if (*cx == x2) {
		return (s1w1>s2w1 ? SEG1_FRONT : SEG1_BEHIND);
	}

	return (s1w1>s2w1 ? SEG1_CLIP_LEFT : SEG1_CLIP_RIGHT);
}

static void draw_add_segment(int y, const sbuffer_segment_t *segment)
{
	int x1,x2, i;
	int num_segs = sbuffer_rows[y].num_segs;
	int clip_seg, clip_pos;

	x1 = segment->start.x;
	x2 = segment->end.x;

	/* Clip if outside */
	if ((x2<0) || (x1>=video.viewport.w) || (y<0) || (y>=video.viewport.h)) {
		return;
	}

	/* Clip against left */
	if (x1<0) {
		x1 = 0;
	}
	
	/* Clip against right */
	if (x2>=video.viewport.w) {
		x2 = video.viewport.w-1;
	}

	if (x2<x1) {
		return;
	}

	/*if ((y<(video.viewport.h/2)-4) || (y>(video.viewport.h/2)-4)) {
		return;
	}*/

	DEBUG_PRINT(("-------add segment %d %d,%d (%.3f,%.3f %.3f,%.3f)\n", y, x1,x2,
		segment->start.u,segment->start.v,
		segment->end.u,segment->end.v));

	for (i=0; i<sbuffer_rows[y].num_segs; i++) {
		DEBUG_PRINT((" [%d:0x%08x] %d->%d %d %p\n", i, sbuffer_rows[y].segment[i].id,
			sbuffer_rows[y].segment[i].start.x, sbuffer_rows[y].segment[i].end.x,
			sbuffer_rows[y].segment[i].tex_num_pal, sbuffer_rows[y].segment[i].texture
		));
	}

	/*--- Trivial cases ---*/

	/* Empty row ? */
	if (num_segs == 0) {
		DEBUG_PRINT(("----empty list\n"));
		draw_push_segment(segment, y,0, x1,x2);
		++sbuffer_rows[y].num_segs;
		return;
	}

	/* Finish before first ? */
	if (x2 < sbuffer_rows[y].segment[0].start.x) {
		DEBUG_PRINT(("----finish before first (%d<%d)\n",x2,sbuffer_rows[y].segment[0].start.x));
		draw_insert_segment(segment, y,0, x1,x2);
		return;
	}

	/* Start after last ? */
	if (sbuffer_rows[y].segment[num_segs-1].end.x < x1) {
		if (num_segs<NUM_SEGMENTS) {
			DEBUG_PRINT(("----start after last (%d<%d)\n", sbuffer_rows[y].segment[num_segs-1].end.x, x1));
			draw_push_segment(segment, y,num_segs, x1,x2);
			++sbuffer_rows[y].num_segs;
		}
		return;
	}

	/*--- Need to check against current list ---*/
	for (i=0; i<sbuffer_rows[y].num_segs; i++) {
		int clip_x1, clip_x2, current_end, ic = i;
		sbuffer_segment_t *current = &sbuffer_rows[y].segment[ic];

		DEBUG_PRINT(("--new %d,%d against %d,%d\n",x1,x2, current->start.x, current->end.x));

		/* Out of screen ? */
		if ((x2<0) || (x1>=video.viewport.w) || (x1>x2)) {
			DEBUG_PRINT(("  stop\n"));
			return;
		}	

		/* Start after current? Will process against next one
		ccccccc
			nnnnn
		*/
		if (current->end.x < x1) {
			DEBUG_PRINT(("  start after %d\n",ic));
			continue;
		}

		/* Finish before current ? Insert before it
			ccccc
		nnnnnn
		*/
		if (x2 < current->start.x) {
			DEBUG_PRINT(("  finish before %d\n",ic));
			draw_insert_segment(segment, y,ic, x1,x2);
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
		if (x1 < current->start.x) {
			int next_x1 = current->start.x;
			DEBUG_PRINT(("  new start before %d, insert %d,%d, will continue from pos %d\n", ic, x1,next_x1-1, next_x1));
			/*printf("   current before: %d,%d\n", current->start.x, current->end.x);**/
			if (draw_insert_segment(segment, y,ic, x1,next_x1-1)) {
				++ic;
			}

			x1 = next_x1;

			current = &sbuffer_rows[y].segment[ic];
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
		if (current->start.x == current->end.x) {
			/* Replace current with new if current behind */
			int cur_x = current->start.x;
			int next_x1 = current->end.x+1;
			DEBUG_PRINT(("  current is single pixel, will continue from pos %d\n", next_x1));
			/*printf("   new w=%.3f, cur w=%.3f\n", calc_w(start, end, x1), current->start.w);*/
			if (calc_w(segment, x1) > current->start.w) {
				DEBUG_PRINT(("   replace current by new\n"));
				draw_push_segment(segment, y,ic, cur_x,cur_x);
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

			/* Skip if new behind current */
			if (calc_w(current, x1) > calc_w(segment, x1)) {
				DEBUG_PRINT(("  new behind current, stop\n"));
				return;
			}

			/*printf("  check %d:%d,%d against new %d\n", ic,current->start.x,current->end.x, x1);*/

			/* Insert new before current, clip current ?
				cccccccc
				n
			*/
			if (x1 == current->start.x) {
				DEBUG_PRINT(("  clip current start from %d,%d at %d\n", current->start.x,current->end.x, x1+1));
				draw_clip_segment(current, x1+1, &current->start);
				DEBUG_PRINT(("  insert new %d,%d at %d\n", x1,x2, ic));
				draw_insert_segment(segment, y,ic, x1,x2);
				return;
			}

			/* Clip current, insert new after current ?
				cccccccc
				       n
			*/
			if (x2 == current->end.x) {
				DEBUG_PRINT(("  clip current end from %d,%d at %d\n", current->start.x,current->end.x, x2-1));
				draw_clip_segment(current, x2-1, &current->end);

				DEBUG_PRINT(("  insert new %d,%d at %d\n", x1,x2, ic+1));
				draw_insert_segment(segment, y,ic+1, x1,x2);
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
			draw_insert_segment(current, y,ic+1, x1+1,current->end.x);

			draw_clip_segment(current, x1-1, &current->end);

			draw_insert_segment(segment, y,ic+1, x1,x2);
			return;
		}

		/* Z check for multiple pixels
			ccccccccc
		1       nnnnn
		2	   nnnnn
		3	    nnnnnnnnnn
		4	        nnnnnn
		*/
		clip_x1 = SEG_MAX(x1, current->start.x);
		clip_x2 = SEG_MIN(x2, current->end.x);

		if (clip_x1==clip_x2) {
			DEBUG_PRINT((" Zcheck multiple pixels, single pixel common zone\n"));
			/* Skip if new behind current */
			if (calc_w(current, clip_x1) > calc_w(segment, clip_x1)) {
				DEBUG_PRINT(("  new behind current, continue from %d\n", clip_x1+1));
				x1 = clip_x1+1;
				continue;
			}

			DEBUG_PRINT(("  clip current end from %d,%d at %d\n", current->start.x,current->end.x, clip_x1-1));

			/* Clip current if behind new */
			draw_clip_segment(current, clip_x1-1, &current->end);
			x1 = clip_x1;
			continue;
		}

		clip_seg = check_behind(current,segment, clip_x1,clip_x2, &clip_pos);

		switch(clip_seg) {
			case SEG1_BEHIND:
				DEBUG_PRINT(("- %d behind new (common from %d->%d)\n", ic,clip_x1,clip_x2));
				current_end = current->end.x;

				if (clip_x1 == current->start.x) {
					if (current_end <= clip_x2) {
						DEBUG_PRINT((" new replace current from %d->%d\n", current->start.x,current_end));
						/* Replace current by new */
						draw_push_segment(segment, y,ic, current->start.x,current_end);
					} else {
						DEBUG_PRINT((" clip current start from %d to %d\n", current->start.x,clip_x2+1));
						/* Clip current on the right */
						draw_clip_segment(current, clip_x2+1, &current->start);

						DEBUG_PRINT((" insert new from %d->%d\n", clip_x1,clip_x2));
						/* Insert new before current */
						draw_insert_segment(segment, y,ic, clip_x1,clip_x2);
					}
				} else {
					/* Insert current after clip_x2 ? */
					if (clip_x2 < current_end) {
						DEBUG_PRINT((" split current from %d->%d\n", clip_x2+1, current->end.x));
						draw_insert_segment(current, y,ic+1, clip_x2+1,current->end.x);
					}

					DEBUG_PRINT((" insert new from %d->%d\n", clip_x1,clip_x2));
					/* Insert new */
					draw_insert_segment(segment, y,ic+1, clip_x1,clip_x2);

					DEBUG_PRINT((" clip current end from %d to %d\n", current_end, clip_x1-1));
					/* Clip current before clip_x1 */
					draw_clip_segment(current, clip_x1-1, &current->end);
				}

				/* Continue with remaining */
				x1 = current_end+1;
				break;
			case SEG1_FRONT:
				DEBUG_PRINT(("- %d in front of new\n", ic));
				/* Continue with remaining part */
				x1 = current->end.x+1;
				break;
			case SEG1_CLIP_LEFT:
				/*if ((clip_pos<clip_x1) || (clip_pos>clip_x2)) {
					printf("- check Z for %d:%d->%d against %d->%d (%d->%d at %d)\n",ic,
						current->start.x,current->end.x, x1,x2, clip_x1,clip_x2, clip_pos
					);
				}*/

				DEBUG_PRINT(("- keep left of %d against new at pos %d\n", ic, clip_pos));

				current_end = current->end.x;

				/* Insert right part of current, after common zone */
				if (x2 < current->end.x) {
					DEBUG_PRINT(("  insert right part of current %d (%d,%d)\n", ic, x2+1,current->end.x));
					draw_insert_segment(current, y,ic+1, x2+1,current->end.x);
				}

				/* Clip current before clip_pos */
				DEBUG_PRINT(("  clip end of %d (%d,%d) at %d\n", ic, current->start.x,current->end.x, clip_pos-1));
				draw_clip_segment(current, clip_pos-1, &current->end);

				/* Continue with remaining part */
				x1 = clip_pos;
				break;
			case SEG1_CLIP_RIGHT:
				/*if ((clip_pos<clip_x1) || (clip_pos>clip_x2))*/ {
					DEBUG_PRINT(("- check Z for %d:%d->%d against %d->%d (%d->%d at %d)\n",ic,
						current->start.x,current->end.x, x1,x2, clip_x1,clip_x2, clip_pos
					));
				}

				DEBUG_PRINT(("- keep right of %d against new at pos %d\n", ic, clip_pos));

				current_end = current->end.x;

				/* Insert left part of current, before common zone */
				if (current->start.x < x1) {
					DEBUG_PRINT(("  insert left part of current %d (%d,%d)\n", ic, current->start.x,x1-1));
					if (draw_insert_segment(current, y,ic, current->start.x,x1-1))
					{
						++ic;
					}

					current = &sbuffer_rows[y].segment[ic];
				}
				/* Clip current */
				DEBUG_PRINT(("  clip start of %d (%d,%d) at %d\n", ic, current->start.x,current->end.x, clip_pos+1));
				draw_clip_segment(current, clip_pos+1, &current->start);
				/* Insert new */
				DEBUG_PRINT(("  insert %d,%d at %d\n", x1,clip_pos, ic+1));
				draw_insert_segment(segment, y,ic, x1,clip_pos);
				/* Continue with remaining part */
				x1 = current_end+1;
				break;
		}
	}

	DEBUG_PRINT(("--remain %d,%d\n",x1,x2));
	if (x1>x2) {
		DEBUG_PRINT((" stop\n"));
		return;
	}

	/* Insert last */
	draw_insert_segment(segment, y,sbuffer_rows[y].num_segs, x1,x2);
}

/* Drawing functions */
void draw_setColor(Uint32 color)
{
	SDL_Surface *surf = video.screen;

	if (video.bpp==8) {
		Uint8 r,g,b,a;
		
		SDL_GetRGBA(color, surf->format, &r,&g,&b,&a);
		draw_color = dither_nearest_index(r,g,b);
		return;
	}

	draw_color = color;
}

void draw_setTexture(int num_pal, render_texture_t *render_tex)
{
	tex_num_pal = num_pal;
	texture = render_tex;
}

void draw_line(draw_vertex_t *v1, draw_vertex_t *v2)
{
	SDL_Surface *surf = video.screen;
	int tmp, x=0, y=0, dx,dy, sx,sy, pixx,pixy;
	Uint8 *pixel;

	int x1 = v1->x;
	int y1 = v1->y;
	int x2 = v2->x;
	int y2 = v2->y;

	x1 += video.viewport.x;
	y1 += video.viewport.y;
	x2 += video.viewport.x;
	y2 += video.viewport.y;

	if (!clip_line(&x1, &y1, &x2, &y2)) {
		return;
	}

	dx = x2 - x1;
	dy = y2 - y1;
	sx = (dx >= 0) ? 1 : -1;
	sy = (dy >= 0) ? 1 : -1;

	/* Mark dirty rectangle */
	{
		int w = dx, h = dy, rx=x1, ry=y1;
		if (dx<0) {
			rx = x2; w = -w;
		}
		if (dy<0) {
			ry = y2; h = -h;
		}
		video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb], rx,ry, w+1,h+1);
		video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb], rx,ry, w+1,h+1);
	}

	dx = sx * dx + 1;
	dy = sy * dy + 1;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((Uint8*)surf->pixels) + pixx * x1 + pixy * y1;
	pixx *= sx;
	pixy *= sy;
	if (dx < dy) {
		tmp = dx; dx = dy; dy = tmp;
		tmp = pixx; pixx = pixy; pixy = tmp;
	}

	switch(surf->format->BytesPerPixel) {
		case 1:
			for (; x < dx; x++, pixel += pixx) {
				*(Uint8*)pixel = draw_color;
				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
			break;
		case 2:
			for (; x < dx; x++, pixel += pixx) {
				*(Uint16*)pixel = draw_color;
				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
			break;
		case 3:
			/* FIXME */
			break;
		case 4:
			for (; x < dx; x++, pixel += pixx) {
				*(Uint32*)pixel = draw_color;
				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
			break;
	}
}

/* Draw horizontal line */
static void draw_hline(int x1, int x2, int y)
{
	int tmp;
	SDL_Surface *surf = video.screen;
	Uint8 *src;

	if (x1>x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	x1 += video.viewport.x;
	x2 += video.viewport.x;
	y += video.viewport.y;

	src = surf->pixels;
	src += surf->pitch * y;
	src += x1 * surf->format->BytesPerPixel;
	switch(surf->format->BytesPerPixel) {
		case 1:
			memset(src, x2-x1+1, draw_color);
			break;
		case 2:
			{
				Uint16 *src_line = (Uint16 *) src;

				for (; x1<=x2; x1++) {
					*src_line++ = draw_color;
				}
			}
			break;
		case 3:
			/* FIXME */
			break;
		case 4:
			{
				Uint32 *src_line = (Uint32 *) src;

				for (; x1<=x2; x1++) {
					*src_line++ = draw_color;
				}
			}
			break;
	}
}

/* Draw horizontal line, gouraud */
static void draw_hline_gouraud(int x1, int x2, int y, Uint32 c1, Uint32 c2)
{
	SDL_Surface *surf = video.screen;
	Uint8 *src;
	int r1,g1,b1, r2,g2,b2, dr,dg,db, dx,x;

	if (x1>x2) {
		int tmp;

		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	x1 += video.viewport.x;
	x2 += video.viewport.x;
	y += video.viewport.y;
	dx = x2-x1+1;

	r1 = (c1>>16) & 0xff;
	g1 = (c1>>8) & 0xff;
	b1 = c1 & 0xff;
	r2 = (c2>>16) & 0xff;
	g2 = (c2>>8) & 0xff;
	b2 = c2 & 0xff;
	dr = r2-r1;
	dg = g2-g1;
	db = b2-b1;

	src = surf->pixels;
	src += surf->pitch * y;
	src += x1 * surf->format->BytesPerPixel;
	switch(surf->format->BytesPerPixel) {
		case 1:
			/* TODO: gouraud using 216 palette */
			memset(src, dx, draw_color);
			break;
		case 2:
			{
				Uint16 *src_line = (Uint16 *) src;

				for (x=0; x<dx; x++) {
					int r = r1 + ((dr*x)/dx);
					int g = g1 + ((dg*x)/dx);
					int b = b1 + ((db*x)/dx);
					*src_line++ = SDL_MapRGB(surf->format, r,g,b);
				}
			}
			break;
		case 3:
			/* FIXME */
			break;
		case 4:
			{
				Uint32 *src_line = (Uint32 *) src;

				for (x=0; x<dx; x++) {
					int r = r1 + ((dr*x)/dx);
					int g = g1 + ((dg*x)/dx);
					int b = b1 + ((db*x)/dx);
					*src_line++ = SDL_MapRGB(surf->format, r,g,b);
				}
			}
			break;
	}
}

/* Draw horizontal line, textured */
static void draw_hline_tex(int x1, int x2, int y, float tu1, float tv1, float tu2, float tv2)
{
	SDL_Surface *surf = video.screen;
	Uint8 *src;
	float du,dv;
	int dx,x;

	if (x1>x2) {
		int tmp;

		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	x1 += video.viewport.x;
	x2 += video.viewport.x;
	y += video.viewport.y;
	dx = x2-x1+1;

	du = tu2-tu1;
	dv = tv2-tv1;

	src = surf->pixels;
	src += surf->pitch * y;
	src += x1 * surf->format->BytesPerPixel;
	switch(surf->format->BytesPerPixel) {
		case 1:
			/* TODO */
			break;
		case 2:
			{
				Uint16 *src_line = (Uint16 *) src;

				if (texture->paletted) {
					Uint32 *palette = texture->palettes[tex_num_pal];
					for (x=0; x<dx; x++) {
						int u = (int) (tu1 + ((du*x)/dx));
						int v = (int) (tv1 + ((dv*x)/dx));
						*src_line++ = palette[texture->pixels[v*texture->pitchw + u]];
					}
				} else {
					Uint16 *tex_pixels = (Uint16 *) texture->pixels;
				
					for (x=0; x<dx; x++) {
						int u = (int) (tu1 + ((du*x)/dx));
						int v = (int) (tv1 + ((dv*x)/dx));
						*src_line++ = tex_pixels[v*texture->pitchw + u];
					}
				}
			}
			break;
		case 3:
			/* FIXME */
			break;
		case 4:
			{
				Uint32 *src_line = (Uint32 *) src;

				if (texture->paletted) {
					Uint32 *palette = texture->palettes[tex_num_pal];
					for (x=0; x<dx; x++) {
						int u = (int) (tu1 + ((du*x)/dx));
						int v = (int) (tv1 + ((dv*x)/dx));
						*src_line++ = palette[texture->pixels[v*texture->pitchw + u]];
					}
				} else {
					Uint32 *tex_pixels = (Uint32 *) texture->pixels;

					for (x=0; x<dx; x++) {
						int u = (int) (tu1 + ((du*x)/dx));
						int v = (int) (tv1 + ((dv*x)/dx));
						*src_line++ = tex_pixels[v*texture->pitchw + u];
					}
				}
			}
			break;
	}
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

static int clip_line(int *x1, int *y1, int *x2, int *y2)
{
	int left = video.viewport.x;
	int top = video.viewport.y;
	int right = left+video.viewport.w-1;
	int bottom = top+video.viewport.h-1;

	int draw = 0;
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

	return draw;
}

void draw_triangle(draw_vertex_t v[3])
{
	draw_line(&v[0], &v[1]);
	draw_line(&v[1], &v[2]);
	draw_line(&v[2], &v[0]);
}

void draw_quad(draw_vertex_t v[4])
{
	draw_line(&v[0], &v[1]);
	draw_line(&v[1], &v[2]);
	draw_line(&v[2], &v[3]);
	draw_line(&v[3], &v[0]);
}

void draw_poly_sbuffer(vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2;
	sbuffer_segment_t segment;

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

	/*printf("triangle\n");*/

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy;
		int num_array = 1; /* max */
		float w1, w2;

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

	segment.id = sbuffer_seg_id++;
	segment.tex_num_pal = tex_num_pal;
	segment.texture = texture;

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

void draw_poly_fill(vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2;

	if (video.viewport.h>size_poly_minmaxx) {
		poly_hlines = realloc(poly_hlines, sizeof(poly_hline_t) * video.viewport.h);
		size_poly_minmaxx = video.viewport.h;
	}

	if (!poly_hlines) {
		fprintf(stderr, "Not enough memory for poly rendering\n");
		return;
	}

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy, tmp;
		int num_array = 1; /* max */
		float w1, w2;

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		w1 = vtx[p1].pos[3] / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];
		w2 = vtx[p2].pos[3] / vtx[p2].pos[2];

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
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

		dy = y2 - y1;
		if (dy>0) {
			int dx = x2 - x1;
			float dw = w2 - w1;
			for (y=0; y<dy; y++) {
				int px;
				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}
				/* Clip line horizontally */
				px = x1 + ((dx*y)/dy);
				if (px<0) {
					px = 0;
				}
				if (px>=video.viewport.w) {
					px = video.viewport.w-1;
				}
				poly_hlines[y1].w[num_array] = w1 + ((dw*y)/dy);
				poly_hlines[y1++].x[num_array] = px;
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
	
	for (y=miny; y<maxy; y++) {
		int pminx = poly_hlines[y].x[0];
		int pmaxx = poly_hlines[y].x[1];
		if (pminx<minx) {
			minx = pminx;
		}
		if (pmaxx>maxx) {
			maxx = pmaxx;
		}
		draw_hline(pminx, pmaxx, y);
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

void draw_poly_gouraud(vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2;

	if (video.viewport.h>size_poly_minmaxx) {
		poly_hlines = realloc(poly_hlines, sizeof(poly_hline_t) * video.viewport.h);
		size_poly_minmaxx = video.viewport.h;
	}

	if (!poly_hlines) {
		fprintf(stderr, "Not enough memory for poly rendering\n");
		return;
	}

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy;
		int num_array = 1; /* max */
		float w1, w2;

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		w1 = vtx[p1].pos[3] / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];
		w2 = vtx[p2].pos[3] / vtx[p2].pos[2];

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			int tmp;
			float tmpz;

			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			tmpz = w1; w1 = w2; w2 = tmpz;
			num_array = 0; /* min */
			v1 = p2;
			v2 = p1;
		}
		if (y1 < miny) {
			miny = y1;
		}
		if (y2 > maxy) {
			maxy = y2;
		}

		dy = y2 - y1;
		if (dy>0) {
			int dx = x2 - x1;
			int r1 = vtx[v1].col[0];
			int dr = vtx[v2].col[0] - vtx[v1].col[0];
			int g1 = vtx[v1].col[1];
			int dg = vtx[v2].col[1] - vtx[v1].col[1];
			int b1 = vtx[v1].col[2];
			int db = vtx[v2].col[2] - vtx[v1].col[2];
			float dw = w2 - w1;
			/*printf("vtx from %d,%d,%d to %d,%d,%d\n",r1,g1,b1,r1+dr,g1+dg,b1+db);*/
			for (y=0; y<dy; y++) {
				int r,g,b, px;
				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}
				/* Clip line horizontally */
				px = x1 + ((dx*y)/dy);
				if (px<0) {
					px = 0;
				}
				if (px>=video.viewport.w) {
					px = video.viewport.w-1;
				}
				r = (r1 + ((dr*y)/dy)) & 0xff;
				g = (g1 + ((dg*y)/dy)) & 0xff;
				b = (b1 + ((db*y)/dy)) & 0xff;
				/* TODO: calc rgb if px<>x1 */
				poly_hlines[y1].c[num_array] = (r<<16)|(g<<8)|b;
				poly_hlines[y1].w[num_array] = w1 + ((dw*y)/dy);
				poly_hlines[y1++].x[num_array] = px;
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
	
	for (y=miny; y<maxy; y++) {
		int pminx = poly_hlines[y].x[0];
		int pmaxx = poly_hlines[y].x[1];
		if (pminx<minx) {
			minx = pminx;
		}
		if (pmaxx>maxx) {
			maxx = pmaxx;
		}
		draw_hline_gouraud(pminx, pmaxx, y, poly_hlines[y].c[0], poly_hlines[y].c[1]);
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

void draw_poly_tex(vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2;

	if (!texture) {
		fprintf(stderr, "No active texture\n");
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

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy;
		int num_array = 1; /* max */
		float w1, w2;

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		w1 = vtx[p1].pos[3] / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];
		w2 = vtx[p2].pos[3] / vtx[p2].pos[2];

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			int tmp;
			float tmpz;

			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			tmpz = w1; w1 = w2; w2 = tmpz;
			num_array = 0; /* min */
			v1 = p2;
			v2 = p1;
		}
		if (y1 < miny) {
			miny = y1;
		}
		if (y2 > maxy) {
			maxy = y2;
		}

		dy = y2 - y1;
		if (dy>0) {
			int dx = x2 - x1;
			float tu1 = vtx[v1].tx[0];
			float du = vtx[v2].tx[0] - vtx[v1].tx[0];
			float tv1 = vtx[v1].tx[1];
			float dv = vtx[v2].tx[1] - vtx[v1].tx[1];
			float dw = w2 - w1;
			for (y=0; y<dy; y++) {
				int px;
				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}
				/* Clip line horizontally */
				px = x1 + ((dx*y)/dy);
				if (px<0) {
					px = 0;
				}
				if (px>=video.viewport.w) {
					px = video.viewport.w-1;
				}
				/* TODO: calc uv if px<>x1 */
				poly_hlines[y1].tu[num_array] = tu1 + ((du*y)/dy);
				poly_hlines[y1].tv[num_array] = tv1 + ((dv*y)/dy);
				poly_hlines[y1].w[num_array] = w1 + ((dw*y)/dy);
				poly_hlines[y1++].x[num_array] = px;
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
	
	for (y=miny; y<maxy; y++) {
		int pminx = poly_hlines[y].x[0];
		int pmaxx = poly_hlines[y].x[1];
		if (pminx<minx) {
			minx = pminx;
		}
		if (pmaxx>maxx) {
			maxx = pmaxx;
		}
		draw_hline_tex(
			pminx, pmaxx, y,
			poly_hlines[y].tu[0], poly_hlines[y].tv[0],
			poly_hlines[y].tu[1], poly_hlines[y].tv[1]
		);
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}
