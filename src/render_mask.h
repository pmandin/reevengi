/*
	Draw background image mask

	Copyright (C) 2010	Patrice Mandin

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

#ifndef RENDER_MASK_H
#define RENDER_MASK_H 1

#include <SDL.h>

#include "video.h"
#include "render_texture.h"

/*--- Defines ---*/

#define RENDER_MASK_WIDTH 320
#define RENDER_MASK_HEIGHT 240
#define RENDER_MASK_SEGS (RENDER_MASK_WIDTH/8)	/* 320 pixels, divided by 8 pixel wide blocks */

/*--- Types ---*/

typedef struct {
	Uint16 x1, x2;
	float w;
} mask_seg_t;

typedef struct {
	Uint16 num_segs;
	mask_seg_t	segs[RENDER_MASK_SEGS];	
} mask_row_t;

typedef struct render_mask_s render_mask_t;

struct render_mask_s {
	void (*shutdown)(render_mask_t *this);

	/* Define a zone with mask in source texture, where to draw on dest */
	void (*addZone)(render_mask_t *this,
		int srcX, int srcY, int w,int h,
		int dstX, int dstY, int depth);

	/* All zones defined, can prepare more stuff before using it for drawing */
	void (*finishedZones)(render_mask_t *this);

	/* Draw mask */
	void (*drawMask)(render_mask_t *this);

	render_texture_t *texture;

	int miny, maxy;
	mask_row_t	mask_row[RENDER_MASK_HEIGHT];	
};

render_mask_t *render_mask_soft_create(render_texture_t *texture);

render_mask_t *render_mask_opengl_create(render_texture_t *texture);

#endif /* RENDER_MASK_H */
