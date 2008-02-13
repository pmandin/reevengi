/*
	Dirty rectangle for surfaces

	Copyright (C) 2007	Patrice Mandin

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

#ifndef DIRTY_RECTS
#define DIRTY_RECTS 1

#include <SDL.h>

typedef struct dirty_rects_s dirty_rects_t;

struct dirty_rects_s {
	int width, height;	/* Dimensions in 16x16 blocks */
	Uint8 *markers;

	void (*resize)(dirty_rects_t *this, int w, int h);
	void (*setDirty)(dirty_rects_t *this, int x, int y, int w, int h);
	void (*clear)(dirty_rects_t *this);
};

dirty_rects_t *dirty_rects_create(int w, int h);
void dirty_rects_destroy(dirty_rects_t *this);

#endif /* DIRTY_RECTS */
