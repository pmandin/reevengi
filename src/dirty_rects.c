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

#include <stdlib.h>

#include "dirty_rects.h"

/*--- Functions prototypes ---*/

static void resize(dirty_rects_t *this, int w, int h);
static void set_dirty(dirty_rects_t *this, int x, int y, int w, int h);
static void clear(dirty_rects_t *this);

/*--- Functions ---*/

dirty_rects_t *dirty_rects_create(int w, int h)
{
	dirty_rects_t *this = (dirty_rects_t *) malloc(sizeof(dirty_rects_t));
	if (!this) {
		return NULL;
	}

	this->width = 0;
	this->height = 0;

	this->resize = resize;
	this->set_dirty = set_dirty;
	this->clear = clear;

	this->resize(this, w,h);
}

void dirty_rects_destroy(dirty_rects_t *this)
{
	if (this) {
		if (this->markers) {
			free(this->markers);
			this->markers = NULL;
		}
		this->width = 0;
		this->height = 0;
	}
}

/*--- Private functions ---*/

static void resize(dirty_rects_t *this, int w, int h)
{
	if (!this) {
		return;
	}

	if (w & 15) {
		w = (w|15)+1;
	}
	w >>= 4;

	if (h & 15) {
		h = (h|15)+1;
	}
	h>>= 4;

	this->width = w;
	this->height = h;

	if (this->markers) {
		free(this->markers);
	}
	this->markers = (Uint8 *) malloc(w*h*sizeof(Uint8));

	this->set_dirty(this, 0,0,w<<4,h<<4);
}

static void set_dirty(dirty_rects_t *this, int x, int y, int w, int h)
{
	int x1=x>>4, y1=y>>4, x2=x+w, y2=y+h;

	if (!this) {
		return;
	}
	if (!this->markers) {
		return;
	}

	if (x2 & 15) {
		x2 = (x2|15)+1;
	}
	x2>>=4;

	if (y2 & 15) {
		y2 = (y2|15)+1;
	}
	y2>>=4;

	for (y=y1; y<y2; y++) {
		for(x=x1; x<x2; x++) {
			if ((x>=0) && (x<this->width) && (y>=0) && (y<this->height)) {
				this->markers[y*this->width+x]=1;
			}
		}
	}
}

static void clear(dirty_rects_t *this)
{
	if (this) {
		if (this->markers) {
			memset(this->markers, 0, this->width*this->height*sizeof(Uint8));
		}
	}
}
