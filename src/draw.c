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

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

/*--- Variables ---*/

static Uint32 draw_color = 0;

/* for poly rendering */
static int size_poly_minmaxx = 0;
static int *poly_minx = NULL;
static int *poly_maxx = NULL;
static Uint32 *poly_minc = NULL;
static Uint32 *poly_maxc = NULL;

/*--- Functions prototypes ---*/

static void draw_hline(int x1, int x2, int y);
static void draw_hline_gouraud(int x1, int x2, int y, Uint32 c1, Uint32 c2);

static int clipEncode (int x, int y, int left, int top, int right, int bottom);
static int clip_line(int *x1, int *y1, int *x2, int *y2);

/*--- Functions ---*/

void draw_init(void)
{
}

void draw_shutdown(void)
{
	if (poly_minx) {
		free(poly_minx);
		poly_minx = NULL;
	}
	if (poly_maxx) {
		free(poly_maxx);
		poly_maxx = NULL;
	}
	if (poly_minc) {
		free(poly_minc);
		poly_minc = NULL;
	}
	if (poly_maxc) {
		free(poly_maxc);
		poly_maxc = NULL;
	}
	size_poly_minmaxx = 0;
}

void draw_setColor(Uint32 color)
{
	SDL_Surface *surf = video.screen;

	if ((video.bpp==8) && params.dithering) {
		Uint8 r,g,b,a;
		
		SDL_GetRGBA(color, surf->format, &r,&g,&b,&a);
		draw_color = dither_nearest_index(r,g,b);
		return;
	}

	draw_color = color;
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
		/*printf("draw_line: dirty %d,%d %d,%d\n", x1,y1,w,h);*/
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

	/*printf("draw_line(%d,%d, %d,%d)\n", x1,y1, x2,y2);*/

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
	if (x1<0) {
		x1 = 0;
	}
	if (x2>=video.viewport.w) {
		x2 = video.viewport.w-1;
	}
	if ((y<0) || (y>=video.viewport.h)) {
		return;
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

	/*printf("from 0x%08x to 0x%08x\n", c1,c2);*/

	if (x1>x2) {
		int tmp;

		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	if (x1<0) {
		x1 = 0;
	}
	if (x2>=video.viewport.w) {
		x2 = video.viewport.w-1;
	}
	if ((y<0) || (y>=video.viewport.h)) {
		return;
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

void draw_poly_fill(vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2;

	if (video.viewport.h>size_poly_minmaxx) {
		poly_minx = realloc(poly_minx, sizeof(int) * video.viewport.h);
		poly_maxx = realloc(poly_maxx, sizeof(int) * video.viewport.h);
		poly_minc = realloc(poly_minc, sizeof(Uint32) * video.viewport.h);
		poly_maxc = realloc(poly_maxc, sizeof(Uint32) * video.viewport.h);
		size_poly_minmaxx = video.viewport.h;
	}

	if (!poly_minx || !poly_maxx) {
		fprintf(stderr, "Not enough memory for poly rendering\n");
		return;
	}

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dx,dy, tmp;
		int *array = poly_maxx;

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			array = poly_minx;
			v1 = p2;
			v2 = p1;
		}
		if (y1 < miny) {
			miny = y1;
		}
		if (y2 > maxy) {
			maxy = y2;
		}

		dx = x2 - x1;
		dy = y2 - y1;
		if (dy>0) {
			for (y=0; y<dy; y++) {
				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}
				array[y1++] = x1 + ((dx*y)/dy);
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
		if (poly_minx[y]<minx) {
			minx = poly_minx[y];
		}
		if (poly_maxx[y]>maxx) {
			maxx = poly_maxx[y];
		}
		draw_hline(poly_minx[y], poly_maxx[y], y);
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
		poly_minx = realloc(poly_minx, sizeof(int) * video.viewport.h);
		poly_maxx = realloc(poly_maxx, sizeof(int) * video.viewport.h);
		poly_minc = realloc(poly_minc, sizeof(Uint32) * video.viewport.h);
		poly_maxc = realloc(poly_maxc, sizeof(Uint32) * video.viewport.h);
		size_poly_minmaxx = video.viewport.h;
	}

	if (!poly_minx || !poly_maxx || !poly_minc || !poly_maxc)
	{
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
		int *array_x = poly_maxx;
		Uint32 *array_c = poly_maxc;

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			int tmp;

			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			array_x = poly_minx;
			array_c = poly_minc;
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
			/*printf("vtx from %d,%d,%d to %d,%d,%d\n",r1,g1,b1,r1+dr,g1+dg,b1+db);*/
			for (y=0; y<dy; y++) {
				int r,g,b;
				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}
				r = (r1 + ((dr*y)/dy)) & 0xff;
				g = (g1 + ((dg*y)/dy)) & 0xff;
				b = (b1 + ((db*y)/dy)) & 0xff;
				array_c[y1] = (r<<16)|(g<<8)|b;
				array_x[y1++] = x1 + ((dx*y)/dy);
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
		if (poly_minx[y]<minx) {
			minx = poly_minx[y];
		}
		if (poly_maxx[y]>maxx) {
			maxx = poly_maxx[y];
		}
		draw_hline_gouraud(poly_minx[y], poly_maxx[y], y, poly_minc[y], poly_maxc[y]);
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

void draw_poly_tex(vertexf_t *vtx, int num_vtx)
{
	draw_poly_fill(vtx, num_vtx);
}
