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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <SDL.h>
#include <math.h>

#include "../video.h"

#include "../g_common/game.h"
#include "../g_common/room.h"

#include "../r_common/render.h"

#include "draw.h"
#include "render_mask.h"
#include "render.h"

/*--- Functions prototypes ---*/

static void addZone(render_mask_t *this, int num_camera,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth);

static void addMaskSegment(render_mask_t *this, int y, int x1, int x2, int depth);

static void finishedZones(render_mask_t *this);

static void drawMask(render_mask_t *this);

/*--- Functions ---*/

render_mask_t *render_mask_soft_create(render_texture_t *texture)
{
	render_mask_soft_t *soft_mask;
	render_mask_t *mask;
	int y;

	soft_mask = calloc(1, sizeof(render_mask_soft_t));
	if (!soft_mask) {
		fprintf(stderr, "Can not allocate memory for render_mask\n");
		return NULL;
	}

	mask = (render_mask_t *) soft_mask;
	render_mask_init(mask, texture);
	mask->addZone = addZone;
	mask->finishedZones = finishedZones;
	mask->drawMask = drawMask;

	soft_mask->miny = RENDER_MASK_HEIGHT;
	soft_mask->maxy = 0;
	for (y=0; y<RENDER_MASK_HEIGHT; y++) {
		soft_mask->mask_row[y].num_segs = 0;
	}

	soft_mask->dirty_miny = RENDER_MASK_HEIGHT/16;
	soft_mask->dirty_maxy = 0;
	for (y=0; y<RENDER_MASK_HEIGHT/16; y++) {
		soft_mask->mask_dirty_row[y].num_segs = 0;
	}

	return mask;
}

static void addZone(render_mask_t *this, int num_camera,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth)
{
	render_mask_soft_t *soft_mask;
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

	soft_mask = (render_mask_soft_t *) this;

	if (dstY < soft_mask->miny) {
		soft_mask->miny = dstY;
	}
	if (dstY+h > soft_mask->maxy) {
		soft_mask->maxy = dstY+h;
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
	render_mask_soft_t *soft_mask;
	mask_row_t *mask_row;
	mask_seg_t *mask_seg;

	assert(this);
	soft_mask = (render_mask_soft_t *) this;
	mask_row = &(soft_mask->mask_row[y]);
	mask_seg = &(mask_row->segs[mask_row->num_segs]);

	if (mask_row->num_segs == RENDER_MASK_SEGS) {
		return;
	}

	mask_seg->x1 = x1;
	mask_seg->x2 = x2;
	mask_seg->depth = (float) depth;
	mask_seg->w = 1.0f / ((float) depth);	/* FIXME, depend on projection matrix */
	++mask_row->num_segs;
}

static void finishedZones(render_mask_t *this)
{
	render_mask_soft_t *soft_mask;
	int x,y,z;

	assert(this);
	soft_mask = (render_mask_soft_t *) this;

	/* Defrag consecutive segments with same depth */
	for (y=soft_mask->miny; y<soft_mask->maxy; y++) {
		mask_row_t *mask_row = &(soft_mask->mask_row[y]);

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
	soft_mask->dirty_miny = soft_mask->miny>>4;
	soft_mask->dirty_maxy = soft_mask->maxy>>4;
	if (soft_mask->maxy & 15) {
		++soft_mask->dirty_maxy;
	}
	for (y=soft_mask->miny; y<soft_mask->maxy; y++) {
		mask_row_t *mask_row = &(soft_mask->mask_row[y]);
		mask_dirty_row_t *mask_dirty_row = &(soft_mask->mask_dirty_row[y>>4]);

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
	render_mask_soft_t *soft_mask;
	int x,y, dstYstart, dstYend;
#if 1
	float va, vb, vc, vd, ve, vf, veclen;
	room_t *room = game->room;
	room_camera_t room_camera;
#endif

	assert(this);
	soft_mask = (render_mask_soft_t *) this;

	if ((soft_mask->miny > soft_mask->maxy) || !draw.addMaskSegment) {
		return;
	}

#if 1
	room->getCamera(room, game->num_camera, &room_camera);

	/*printf("camera: from %d,%d,%d to %d,%d,%d\n",
		room_camera.from_x, room_camera.from_y, room_camera.from_z,
		room_camera.to_x, room_camera.to_y, room_camera.to_z);*/

	va=room_camera.to_x-room_camera.from_x;
	vb=room_camera.from_x;
	vc=room_camera.to_y-room_camera.from_y;
	vd=room_camera.from_y;
	ve=room_camera.to_z-room_camera.from_z;
	vf=room_camera.from_z;

	veclen = sqrt(va*va + vc*vc + ve*ve);

	/*printf("camera: from-to dist: %f\n", veclen);*/
/*
	x = (to-from) k + from
	y = (to-from) k + from
	z = (to-from) k + from

	k^2 = (depth^2 + va^2) / veclen^2

	calc point between from and to, for depth
	setup projection
	calc point projection with its w */
#endif

	dstYstart = (soft_mask->miny * video.viewport.h) / RENDER_MASK_HEIGHT;
	dstYend = (soft_mask->maxy * video.viewport.h) / RENDER_MASK_HEIGHT;

	for (y=dstYstart; y<dstYend; y++) {
		int x, srcY = (y * RENDER_MASK_HEIGHT) / video.viewport.h;
		mask_row_t *mask_row = &(soft_mask->mask_row[srcY]);
		mask_seg_t *mask_seg = mask_row->segs;

		for (x=0; x<mask_row->num_segs; x++) {
			int dstXstart = (mask_seg->x1 * video.viewport.w ) / RENDER_MASK_WIDTH;
			int dstXend = ((mask_seg->x2+1) * video.viewport.w ) / RENDER_MASK_WIDTH;
#if 1
			float vx,vy,vz, new_w, vk;
			vertex_t v1;
			vertexf_t poly[16];
			
			vk = sqrt(mask_seg->depth*mask_seg->depth + va*va) / veclen;
		
			/*printf("vk: %f -> %f,%f,%f\n", vk, vk*va+vb, vk*vc+vd, vk*ve+vf);*/

			vx = vk*va+vb;
			vy = vk*vc+vd;
			vz = vk*ve+vf;

			v1.x = (Sint16) vx;
			v1.y = (Sint16) vy;
			v1.z = (Sint16) vz;
			v1.u = v1.v = 0;

			project_point(&v1, poly);

			/*printf(">%d,%f 1:% 9.3f,% 9.3f,% 9.3f -> %f,%f,%f %f\n",
				(int) mask_seg->depth, mask_seg->w,
				vx,vy,vz,
				poly[0].pos[0]/poly[0].pos[3],
				poly[0].pos[1]/poly[0].pos[3],
				poly[0].pos[2]/poly[0].pos[3],
				poly[0].pos[3]/poly[0].pos[2]);*/

			/*	calc point projection with its w */
			new_w = poly[0].pos[3] / poly[0].pos[2];

			draw.addMaskSegment(&draw, y,
				dstXstart,dstXend-1,
				new_w);
#else
			draw.addMaskSegment(&draw, y,
				dstXstart,dstXend-1,
				mask_seg->w);
#endif
/*assert(0);*/

			++mask_seg;
		}
	}

	/*assert(0);*/

	/* Mark dirty rectangles */
	for (y=soft_mask->dirty_miny; y<soft_mask->dirty_maxy; y++) {
		mask_dirty_row_t *mask_dirty_row = &(soft_mask->mask_dirty_row[y]);
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
