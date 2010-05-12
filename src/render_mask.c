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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <SDL.h>

#include "video.h"
#include "render.h"
#include "render_mask.h"

/*--- Functions prototypes ---*/

static void shutdown(render_mask_t *this);

static void addZone(render_mask_t *this,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth);

static void addMaskSegment(render_mask_t *this, int y, int x1, int x2, int depth);

static void finishedZones(render_mask_t *this);

static void drawMask(render_mask_t *this);

/*--- Functions ---*/

render_mask_t *render_mask_soft_create(render_texture_t *texture)
{
	render_mask_t *mask;
	int y;

	mask = calloc(1, sizeof(render_mask_t));
	if (!mask) {
		fprintf(stderr, "Can not allocate memory for render_mask\n");
		return NULL;
	}

	mask->shutdown = shutdown;
	mask->addZone = addZone;
	mask->finishedZones = finishedZones;
	mask->drawMask = drawMask;

	mask->texture = texture;

	mask->miny = RENDER_MASK_HEIGHT;
	mask->maxy = 0;
	for (y=0; y<RENDER_MASK_HEIGHT; y++) {
		mask->mask_row[y].num_segs = 0;
	}

	mask->dirty_miny = RENDER_MASK_HEIGHT/16;
	mask->dirty_maxy = 0;
	for (y=0; y<RENDER_MASK_HEIGHT/16; y++) {
		mask->mask_dirty_row[y].num_segs = 0;
	}

	return mask;
}

static void shutdown(render_mask_t *this)
{
	if (!this) {
		return;
	}

	free(this);
}

static void addZone(render_mask_t *this,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth)
{
	int x,y;
	render_texture_t *tex;
	Uint8 *alpha_pal, *src_line;

	/* Clip to dest */
	if (dstX<0) {
		w += dstX;
		dstX = 0;
	}
	if (dstY<0) {
		h += dstY;
		dstY = 0;
	}
	if (dstX+w >= RENDER_MASK_WIDTH) {
		w = RENDER_MASK_WIDTH-dstX;
	}
	if (dstY+h >= RENDER_MASK_HEIGHT) {
		h = RENDER_MASK_HEIGHT-dstY;
	}

	assert(this);
	assert(this->texture);
	assert((dstY>=0) && (dstY+h<=RENDER_MASK_HEIGHT) && (dstX>=0) && (dstX+w<=RENDER_MASK_WIDTH));

	tex = this->texture;
	if (tex->bpp > 1) {
		fprintf(stderr, "render_mask: Need paletted texture as mask\n");
		return;
	}
	alpha_pal = tex->alpha_palettes[0];
	src_line = tex->pixels;
	src_line += srcY * tex->pitch;

	if (dstY < this->miny) {
		this->miny = dstY;
	}
	if (dstY+h > this->maxy) {
		this->maxy = dstY+h;
	}

	for (y=0; y<h; y++) {
		int startx = -1;
		Uint8 *src_col = &src_line[srcX];

		for (x=0; x<w; x++) {
			if (!alpha_pal[*src_col++]) {
				/* Transparent pixel, add previous opaque segment */
				if (startx>=0) {
					addMaskSegment(this, dstY+y, startx, dstX+x-1, depth);
					startx = -1;
				}
			} else {
				/* Opaque, set start */
				if (startx<0) {
					startx = dstX+x;
				}
			}
		}

		/* Add segment till EOL */
		if (startx>=0) {
			addMaskSegment(this, dstY+y, startx, dstX+w-1, depth);
		}

		src_line += tex->pitch;
	}
}

static void addMaskSegment(render_mask_t *this, int y, int x1, int x2, int depth)
{
	mask_row_t *mask_row = &(this->mask_row[y]);
	mask_seg_t *mask_seg = &(mask_row->segs[mask_row->num_segs]);
	float w = 1.0f / ((float) depth);

	if (mask_row->num_segs == RENDER_MASK_SEGS) {
		return;
	}

	mask_seg->x1 = x1;
	mask_seg->x2 = x2;
	mask_seg->w = w;
	++mask_row->num_segs;
}

static void finishedZones(render_mask_t *this)
{
	int x,y,z;

	/* Defrag consecutive segments with same depth */
	for (y=this->miny; y<this->maxy; y++) {
		mask_row_t *mask_row = &(this->mask_row[y]);

		if (mask_row->num_segs < 2) {
			continue;
		}

		x = 0;
		while (x<mask_row->num_segs-1) {
			mask_seg_t *current = &(mask_row->segs[x]);
			mask_seg_t *next = &(mask_row->segs[x+1]);

			/* Different depth, skip */
			if (current->w != next->w) {
				++x;
				continue;
			}

			/* Current does not finish just before next, skip */
			if (current->x2+1 != next->x1) {
				++x;
				continue;
			}

			/* Merge segments */
			current->x2 = next->x2;
			--mask_row->num_segs;

			/* Advance all remaining segments */
			current = next;
			for (z=x+1; z<mask_row->num_segs; z++) {
				current[0].x1 = current[1].x1;
				current[0].x2 = current[1].x2;
				current[0].w = current[1].w;
				++current;
			}
		}
	}

	/* Create dirty rectangle list for mask */
	this->dirty_miny = this->miny>>4;
	this->dirty_maxy = this->maxy>>4;
	if (this->maxy & 15) {
		++this->dirty_maxy;
	}
	for (y=this->miny; y<this->maxy; y++) {
		mask_row_t *mask_row = &(this->mask_row[y]);
		mask_dirty_row_t *mask_dirty_row = &(this->mask_dirty_row[y>>4]);

		for (x=0; x<mask_row->num_segs; x++) {
			int k, must_add = 1;

			int dirty_x1 = mask_row->segs[x].x1>>4;
			int dirty_x2 = mask_row->segs[x].x2;
			if (dirty_x2 & 15) {
				dirty_x2 = (dirty_x2 | 15)+1;
				dirty_x2 = (dirty_x2>>4)-1; 
			} else {
				dirty_x2 >>= 4;
			}

			for (k=0; k<mask_dirty_row->num_segs; k++) {
				mask_dirty_seg_t *mask_dirty_seg = &(mask_dirty_row->segs[k]);

				/* New start overlap? merge */
				if ((mask_dirty_seg->x1 >= dirty_x1) && (dirty_x1 <= mask_dirty_seg->x2) && (mask_dirty_seg->x2 < dirty_x2)) {
					mask_dirty_seg->x2 = dirty_x2;
					must_add = 0;
					break;
				}

				/* New end overlap? merge */
				if ((mask_dirty_seg->x1 >= dirty_x2) && (dirty_x2 <= mask_dirty_seg->x2) && (dirty_x1 < mask_dirty_seg->x1)) {
					mask_dirty_seg->x1 = dirty_x1;
					must_add = 0;
					break;
				}

				/* New touch current ? merge */
				if (mask_dirty_seg->x2+1 == dirty_x1) {
					mask_dirty_seg->x2 = dirty_x2;
					must_add = 0;
					break;
				}

				if (dirty_x2+1 == mask_dirty_seg->x1) {
					mask_dirty_seg->x1 = dirty_x1;
					must_add = 0;
					break;
				}
			}

			/* Add at end of list */
			if (must_add && (mask_dirty_row->num_segs < RENDER_MASK_WIDTH>>4)) {
				mask_dirty_seg_t *mask_dirty_seg = &(mask_dirty_row->segs[mask_dirty_row->num_segs]);
				
				mask_dirty_seg->x1 = dirty_x1;
				mask_dirty_seg->x2 = dirty_x2;
				++mask_dirty_row->num_segs;
			}
		}
	}
}

static void drawMask(render_mask_t *this)
{
	int x,y, dstYstart, dstYend;

	assert(this);

	if ((this->miny > this->maxy) || !render.draw.addMaskSegment) {
		return;
	}

	dstYstart = (this->miny * video.viewport.h) / RENDER_MASK_HEIGHT;
	dstYend = (this->maxy * video.viewport.h) / RENDER_MASK_HEIGHT;

	for (y=dstYstart; y<dstYend; y++) {
		int x, srcY = (y * RENDER_MASK_HEIGHT) / video.viewport.h;
		mask_row_t *mask_row = &(this->mask_row[srcY]);
		mask_seg_t *mask_seg = mask_row->segs;

		for (x=0; x<mask_row->num_segs; x++) {
			int dstXstart = (mask_seg->x1 * video.viewport.w ) / RENDER_MASK_WIDTH;
			int dstXend = ((mask_seg->x2+1) * video.viewport.w ) / RENDER_MASK_WIDTH;

			render.draw.addMaskSegment(&render.draw, y,
				dstXstart,dstXend-1,
				mask_seg->w);

			++mask_seg;
		}
	}

	/* Mark dirty rectangles */
	for (y=this->dirty_miny; y<this->dirty_maxy; y++) {
		mask_dirty_row_t *mask_dirty_row = &(this->mask_dirty_row[y]);
		int recty = ((y<<4) * video.viewport.h) / RENDER_MASK_HEIGHT;
		int recth = ((1<<4) * video.viewport.h) / RENDER_MASK_HEIGHT;

		for (x=0; x<mask_dirty_row->num_segs; x++) {
			int rectx = mask_dirty_row->segs[x].x1;
			int rectw = mask_dirty_row->segs[x].x2-rectx+1;

			rectx = ((rectx<<4) * video.viewport.w ) / RENDER_MASK_WIDTH;
			rectw = ((rectw<<4) * video.viewport.w ) / RENDER_MASK_WIDTH;

			video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
				video.viewport.x+rectx, video.viewport.y+recty, rectw,recth);
			video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
				video.viewport.x+rectx, video.viewport.y+recty, rectw,recth);
		}
	}
}
