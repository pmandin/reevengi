/*
	Draw background image mask
	Software renderer

	Copyright (C) 2010-2013	Patrice Mandin

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

#ifndef RENDER_MASK_SOFT_H
#define RENDER_MASK_SOFT_H 1

/*--- External types ---*/

struct render_texture_s;
struct render_mask_s;

/*--- Types ---*/

typedef struct {
	Uint16 x1, x2;
	float w;
} mask_seg_t;

typedef struct {
	Uint16 num_segs;
	mask_seg_t	segs[RENDER_MASK_SEGS];	
} mask_row_t;

typedef struct {
	Uint16 x1, x2;
} mask_dirty_seg_t;

typedef struct {
	Uint16 num_segs;
	mask_dirty_seg_t	segs[RENDER_MASK_SEGS];	
} mask_dirty_row_t;

typedef struct render_mask_soft_s render_mask_soft_t;

struct render_mask_soft_s {
	struct render_mask_s	render_mask;

	/* Sbuffer like structure for masking segments */
	int miny, maxy;
	mask_row_t	mask_row[RENDER_MASK_HEIGHT];	

	/* Sbuffer like structure for dirty rectangles */
	int dirty_miny, dirty_maxy;
	mask_dirty_row_t	mask_dirty_row[RENDER_MASK_HEIGHT/16];
};

/*--- Functions ---*/

render_mask_t *render_mask_soft_create(struct render_texture_s *texture);

#endif /* RENDER_MASK_SOFT_H */
