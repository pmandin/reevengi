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

/*--- Variables ---*/

static Uint32 draw_color = 0;

/*--- Functions prototypes ---*/

static void clip_line(int *x1, int *y1, int *x2, int *y2);

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

	/* TODO: clip line to viewport */
	clip_line(&x1, &y1, &x2, &y2);

	x1 += video.viewport.x;
	y1 += video.viewport.y;
	x2 += video.viewport.x;
	y2 += video.viewport.y;

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

	printf("draw_line(%d,%d, %d,%d)\n", x1,y1, x2,y2);

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
static void clip_line(int *x1, int *y1, int *x2, int *y2)
{
}
