/*
	RE2 PRI
	Background masking

	Copyright (C) 2009-2013	Patrice Mandin

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

#include "../log.h"

#include "../g_common/room.h"

#include "../r_common/render.h"

#include "rdt.h"
#include "rdt_rid.h"
#include "rdt_pri.h"

/*--- Functions ---*/

void rdt2_pri_initMasks(room_t *this, int num_camera)
{
	rdt2_header_t *rdt_header;
	Uint32 offset;
	rdt2_rid_t *cam_array;
	rdt2_pri_header_t *mask_hdr;
	rdt2_pri_offset_t *mask_offsets;
	int num_offset, count_offsets;
	render_mask_t *rdr_mask;

	if (num_camera>=this->num_cameras) {
		return;
	}

	if (this->bg_mask==NULL) {
		return;
	}

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_CAMERAS]);
	cam_array = (rdt2_rid_t *) &((Uint8 *) this->file)[offset];

	offset = SDL_SwapLE32(cam_array[num_camera].masks_offset);
	if (offset == 0xffffffffUL) {
		return;
	}

	this->rdr_mask = render.createMask(this->bg_mask);
	if (!this->rdr_mask) {
		return;
	}
	rdr_mask = this->rdr_mask;

	mask_hdr = (rdt2_pri_header_t *) &((Uint8 *) this->file)[offset];
	offset += sizeof(rdt2_pri_header_t);

	mask_offsets = (rdt2_pri_offset_t *) &((Uint8 *) this->file)[offset];
	count_offsets = (Sint16) SDL_SwapLE16(mask_hdr->num_offset);
	if (count_offsets < 0) {
		return;
	}

	offset += sizeof(rdt2_pri_offset_t) * count_offsets;

	for (num_offset=0; num_offset<SDL_SwapLE16(mask_hdr->num_offset); num_offset++) {
		int num_mask;
		
		for (num_mask=0; num_mask<SDL_SwapLE16(mask_offsets->count); num_mask++) {
			rdt2_pri_square_t *square_mask;
			int src_x, src_y, width, height, depth;
			int dst_x = SDL_SwapLE16(mask_offsets->dst_x);
			int dst_y = SDL_SwapLE16(mask_offsets->dst_y);

			square_mask = (rdt2_pri_square_t *) &((Uint8 *) this->file)[offset];
			if (square_mask->size == 0) {
				/* Rect mask */
				rdt2_pri_rect_t *rect_mask = (rdt2_pri_rect_t *) square_mask;

				src_x = rect_mask->src_x;
				src_y = rect_mask->src_y;
				dst_x += rect_mask->dst_x;
				dst_y += rect_mask->dst_y;
				width = SDL_SwapLE16(rect_mask->width);
				height = SDL_SwapLE16(rect_mask->height);
				depth = SDL_SwapLE16(rect_mask->depth);

				offset += sizeof(rdt2_pri_rect_t);
			} else {
				/* Square mask */

				src_x = square_mask->src_x;
				src_y = square_mask->src_y;
				dst_x += square_mask->dst_x;
				dst_y += square_mask->dst_y;
				width = height = SDL_SwapLE16(square_mask->size);
				depth = SDL_SwapLE16(square_mask->depth);

				offset += sizeof(rdt2_pri_square_t);
			}

			rdr_mask->addZone(rdr_mask,
				src_x,src_y, width,height,
				dst_x,dst_y, 32*depth);
		}

		mask_offsets++;
	}

	rdr_mask->finishedZones(rdr_mask);
}

void rdt2_pri_drawMasks(room_t *this, int num_camera)
{
	if (!this->rdr_mask) {
		return;
	}

	this->rdr_mask->drawMask(this->rdr_mask);
}
