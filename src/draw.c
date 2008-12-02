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

/*--- Functions prototypes ---*/

static int clipEncode (int x, int y, int left, int top, int right, int bottom);
static int clip_line(int *x1, int *y1, int *x2, int *y2);

/*--- Functions ---*/

void draw_setColor(Uint32 color)
{
	SDL_Surface *surf = video.screen;

	draw_color = SDL_MapRGBA(surf->format,
		(color>>16) & 0xff, (color>>8) & 0xff,
		color & 0xff, (color>>24) & 0xff);
}

void draw_line(int x1, int y1, int x2, int y2)
{
	SDL_Surface *surf = video.screen;
	int tmp, x=0, y=0, dx,dy, sx,sy, pixx,pixy;
	Uint8 *pixel;

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

	/* Mark dirty rectangle */
	{
		int w = dx, h = dy;
		if (w<0) {
			x1 = x2; w = -w;
		}
		if (h<0) {
			y1 = y2; h = -h;
		}
		video.dirty_rects->setDirty(video.dirty_rects, x1,y1, w,h);
		video.upload_rects->setDirty(video.upload_rects, x1,y1, w,h);
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
