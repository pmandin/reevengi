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

	return mask;
}

static void shutdown(render_mask_t *this)
{
	assert(this);

	if (this->texture) {
		this->texture->shutdown(this->texture);
		this->texture = NULL;
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

	assert(this);
	assert(this->texture);
	assert((dstY>=0) && (dstY+h<RENDER_MASK_HEIGHT) && (dstX>=0) && (dstX+w<RENDER_MASK_WIDTH));

	tex = this->texture;
	if (tex->bpp > 1) {
		fprintf(stderr, "Need paletted mask\n");
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

		/* Draw till EOL */
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
}

static void drawMask(render_mask_t *this)
{
	int x,y;

	assert(this);

	if (this->miny > this->maxy) {
		return;
	}

	for (y=this->miny; y<this->maxy; y++) {
		mask_row_t	*mask_row = &(this->mask_row[y]);
		mask_seg_t	*mask_seg = mask_row->segs;

		for (x=0; x<mask_row->num_segs; x++) {
			int x1 = mask_seg->x1;
			int x2 = mask_seg->x2;
			int y1 = y;

			/* Scale x1,x2,y1 to video resolution */

			render.draw.addMaskSegment(&render.draw, y1, x1,x2, mask_seg->w);

			mask_seg++;
		}
	}
}
