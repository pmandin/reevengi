/*
	2D drawing functions
	Simple renderer

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

/*--- Types ---*/

typedef struct {
	int x[2];	/* x 0:min, 1:max */
	Uint32 c[2];	/* color 0:min, 1:max */
	float tu[2];	/* texture 0:min, 1:max */
	float tv[2];
	float w[2];	/* 1/z, 0:min, 1:max */
} poly_hline_t;

/*--- Variables ---*/

/* for poly rendering */
static int size_poly_minmaxx = 0;
static poly_hline_t *poly_hlines = NULL;

/*--- Functions prototypes ---*/

static void draw_shutdown(draw_t *this);

static void draw_resize(draw_t *this, int w, int h);
static void draw_startFrame(draw_t *this);
static void draw_endFrame(draw_t *this);

static void draw_line(draw_t *this, draw_vertex_t *v1, draw_vertex_t *v2);
static void draw_triangle(draw_t *this, draw_vertex_t v[3]);
static void draw_quad(draw_t *this, draw_vertex_t v[4]);

static void draw_poly_line(draw_t *this, vertexf_t *vtx, int num_vtx);
static void draw_poly_fill(draw_t *this, vertexf_t *vtx, int num_vtx);
static void draw_poly_gouraud(draw_t *this, vertexf_t *vtx, int num_vtx);
static void draw_poly_tex(draw_t *this, vertexf_t *vtx, int num_vtx);

static void draw_hline(draw_t *this, int x1, int x2, int y);
static void draw_hline_gouraud(draw_t *this, int x1, int x2, int y, Uint32 c1, Uint32 c2);
static void draw_hline_tex(draw_t *this, int x1, int x2, int y, float tu1, float tv1, float tu2, float tv2);

static int clipEncode (int x, int y, int left, int top, int right, int bottom);
static int clip_line(int *x1, int *y1, int *x2, int *y2);

/*--- Functions ---*/

void draw_init_simple(draw_t *draw)
{
	draw->shutdown = draw_shutdown;

	draw->resize = draw_resize;
	draw->startFrame = draw_startFrame;
	draw->endFrame = draw_endFrame;

	draw->line = draw_line;
	draw->triangle = draw_triangle;
	draw->quad = draw_quad;

	draw->polyLine = draw_poly_line;
	draw->polyFill = draw_poly_fill;
	draw->polyGouraud = draw_poly_gouraud;
	draw->polyTexture = draw_poly_tex;
}

static void draw_shutdown(draw_t *this)
{
	if (poly_hlines) {
		free(poly_hlines);
		poly_hlines = NULL;
	}
	size_poly_minmaxx = 0;
}

static void draw_resize(draw_t *this, int w, int h)
{
}

static void draw_startFrame(draw_t *this)
{
}

static void draw_endFrame(draw_t *this)
{
}

static void draw_line(draw_t *this, draw_vertex_t *v1, draw_vertex_t *v2)
{
	SDL_Surface *surf = video.screen;
	int tmp, x=0, y=0, dx,dy, sx,sy, pixx,pixy;
	Uint8 *pixel;
	Uint32 draw_color = render.color;

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
			for (; x < dx; x++, pixel += pixx) {
				Uint8 *dst = pixel;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				*dst++ = draw_color>>16;
				*dst++ = draw_color>>8;
				*dst++ = draw_color;
#else
				*dst++ = draw_color;
				*dst++ = draw_color>>8;
				*dst++ = draw_color>>16;
#endif

				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
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
static void draw_hline(draw_t *this, int x1, int x2, int y)
{
	SDL_Surface *surf = video.screen;
	Uint8 *src;
	Uint32 draw_color = render.color;

	x1 += video.viewport.x;
	x2 += video.viewport.x;
	y += video.viewport.y;

	src = surf->pixels;
	src += surf->pitch * y;
	src += x1 * surf->format->BytesPerPixel;
	switch(surf->format->BytesPerPixel) {
		case 1:
			memset(src, draw_color, x2-x1+1);
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
			{
				Uint8 *src_line = (Uint8 *) src;

				for (; x1<=x2; x1++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
					*src_line++ = draw_color>>16;
					*src_line++ = draw_color>>8;
					*src_line++ = draw_color;
#else
					*src_line++ = draw_color;
					*src_line++ = draw_color>>8;
					*src_line++ = draw_color>>16;
#endif
				}
			}
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
static void draw_hline_gouraud(draw_t *this, int x1, int x2, int y, Uint32 c1, Uint32 c2)
{
	SDL_Surface *surf = video.screen;
	Uint8 *src;
	int r1,g1,b1, r2,g2,b2, dr,dg,db, dx,x;

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
			{
				Uint8 *src_line = (Uint8 *) src;

				for (x=0; x<dx; x++) {
					int r = r1 + ((dr*x)/dx);
					int g = g1 + ((dg*x)/dx);
					int b = b1 + ((db*x)/dx);
					*src_line++ = dither_nearest_index(r,g,b);
				}
			}
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
			{
				Uint8 *src_line = (Uint8 *) src;

				for (x=0; x<dx; x++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
					*src_line++ = r1 + ((dr*x)/dx);
					*src_line++ = g1 + ((dg*x)/dx);
					*src_line++ = b1 + ((db*x)/dx);
#else
					*src_line++ = b1 + ((db*x)/dx);
					*src_line++ = g1 + ((dg*x)/dx);
					*src_line++ = r1 + ((dr*x)/dx);
#endif
				}
			}
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
static void draw_hline_tex(draw_t *this, int x1, int x2, int y, float tu1, float tv1, float tu2, float tv2)
{
	SDL_Surface *surf = video.screen;
	Uint8 *src;
	float du,dv;
	int dx,x, tex_num_pal = render.tex_pal;
	render_texture_t *texture = render.texture; 

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
			{
				Uint8 *src_line = (Uint8 *) src;

				if (texture->paletted) {
					Uint32 *palette = texture->palettes[tex_num_pal];
					for (x=0; x<dx; x++) {
						int u = (int) (tu1 + ((du*x)/dx));
						int v = (int) (tv1 + ((dv*x)/dx));
						*src_line++ = palette[texture->pixels[v*texture->pitchw + u]];
					}
				} else {
					/* TODO */
				}
			}
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
			{
				Uint8 *src_line = (Uint8 *) src;
				Uint32 color;

				if (texture->paletted) {
					Uint32 *palette = texture->palettes[tex_num_pal];
					for (x=0; x<dx; x++) {
						int u = (int) (tu1 + ((du*x)/dx));
						int v = (int) (tv1 + ((dv*x)/dx));
						color = palette[texture->pixels[v*texture->pitchw + u]];
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
						*src_line++ = color>>16;
						*src_line++ = color>>8;
						*src_line++ = color;
#else
						*src_line++ = color;
						*src_line++ = color>>8;
						*src_line++ = color>>16;
#endif
					}
				} else {
					Uint16 *tex_pixels = (Uint16 *) texture->pixels;
				
					for (x=0; x<dx; x++) {
						int u = (int) (tu1 + ((du*x)/dx));
						int v = (int) (tv1 + ((dv*x)/dx));
						color = tex_pixels[v*texture->pitchw + u];
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
						*src_line++ = color>>16;
						*src_line++ = color>>8;
						*src_line++ = color;
#else
						*src_line++ = color;
						*src_line++ = color>>8;
						*src_line++ = color>>16;
#endif
					}
				}
			}
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

static void draw_triangle(draw_t *this, draw_vertex_t v[3])
{
	draw_line(this, &v[0], &v[1]);
	draw_line(this, &v[1], &v[2]);
	draw_line(this, &v[2], &v[0]);
}

static void draw_quad(draw_t *this, draw_vertex_t v[4])
{
	draw_line(this, &v[0], &v[1]);
	draw_line(this, &v[1], &v[2]);
	draw_line(this, &v[2], &v[3]);
	draw_line(this, &v[3], &v[0]);
}

static void draw_poly_line(draw_t *this, vertexf_t *vtx, int num_vtx)
{
	/* TODO */
}

static void draw_poly_fill(draw_t *this, vertexf_t *vtx, int num_vtx)
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

	if (SDL_MUSTLOCK(video.screen)) {
		SDL_LockSurface(video.screen);
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
		if (pminx>pmaxx) {
			continue;
		}
		draw_hline(this, pminx, pmaxx, y);
	}

	if (SDL_MUSTLOCK(video.screen)) {
		SDL_UnlockSurface(video.screen);
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

static void draw_poly_gouraud(draw_t *this, vertexf_t *vtx, int num_vtx)
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
	
	if (SDL_MUSTLOCK(video.screen)) {
		SDL_LockSurface(video.screen);
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
		if (pminx>pmaxx) {
			continue;
		}
		draw_hline_gouraud(this,
			pminx, pmaxx, y,
			poly_hlines[y].c[0], poly_hlines[y].c[1]);
	}

	if (SDL_MUSTLOCK(video.screen)) {
		SDL_UnlockSurface(video.screen);
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

static void draw_poly_tex(draw_t *this, vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2;

	if (!render.texture) {
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
	
	if (SDL_MUSTLOCK(video.screen)) {
		SDL_LockSurface(video.screen);
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
		if (pminx>pmaxx) {
			continue;
		}
		draw_hline_tex(this,
			pminx, pmaxx, y,
			poly_hlines[y].tu[0], poly_hlines[y].tv[0],
			poly_hlines[y].tu[1], poly_hlines[y].tv[1]
		);
	}

	if (SDL_MUSTLOCK(video.screen)) {
		SDL_UnlockSurface(video.screen);
	}

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}
