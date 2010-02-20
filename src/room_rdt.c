/*
	Room description
	RE1 RDT manager

	Copyright (C) 2009	Patrice Mandin

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

#include "room.h"
#include "state.h"
#include "room_rdt.h"
#include "room_rdt_script.h"
#include "log.h"
#include "video.h"
#include "render.h"

/*--- Types ---*/

typedef struct {
	Uint32 masks_offset;
	Uint32 tim_offset;
	Sint32 camera_from_x;
	Sint32 camera_from_y;
	Sint32 camera_from_z;
	Sint32 camera_to_x;
	Sint32 camera_to_y;
	Sint32 camera_to_z;
	Uint32 unknown[3];
} rdt_camera_pos_t;

typedef struct {
	Uint16 to, from;
	Sint16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
} rdt_camera_switch_t;

typedef struct {
	Uint16 num_offset;
	Uint16 num_masks;
} rdt_mask_header_t;

typedef struct {
	Uint16 count;
	Uint16 unknown;
	Uint16 dst_x, dst_y;
} rdt_mask_offset_t;

typedef struct {
	Uint8 src_x, src_y;
	Uint8 dst_x, dst_y;
	Uint16 depth;
	Uint8 unknown;
	Uint8 size;
} rdt_mask_square_t;

typedef struct {
	Uint8 src_x, src_y;
	Uint8 dst_x, dst_y;
	Uint16 depth, zero;
	Uint16 width, height;
} rdt_mask_rect_t;

/*--- Functions prototypes ---*/

static void rdt_getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static int rdt_getNumCamswitches(room_t *this);
static void rdt_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

static int rdt_getNumBoundaries(room_t *this);
static void rdt_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_camswitch);

static void rdt_drawMasks(room_t *this, int num_camera);
static void rdt_loadMasks(room_t *this, int num_camera);

/*--- Functions ---*/

void room_rdt_init(room_t *this)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;

	if (this->file_length>4) {
		this->num_cameras = rdt_header->num_cameras;
		this->num_camswitches = rdt_getNumCamswitches(this);
		this->num_boundaries = rdt_getNumBoundaries(this);

		this->getCamera = rdt_getCamera;
		this->getCamswitch = rdt_getCamswitch;
		this->getBoundary = rdt_getBoundary;

		this->drawMasks = rdt_drawMasks;

		room_rdt_scriptInit(this);
	}

	logMsg(2, "%d cameras angles, %d camera switches, %d boundaries\n",
		this->num_cameras, this->num_camswitches, this->num_boundaries);
}

static void rdt_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
	rdt_camera_pos_t *cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[sizeof(rdt1_header_t)];

	room_camera->from_x = SDL_SwapLE32(cam_array[num_camera].camera_from_x);
	room_camera->from_y = SDL_SwapLE32(cam_array[num_camera].camera_from_y);
	room_camera->from_z = SDL_SwapLE32(cam_array[num_camera].camera_from_z);
	room_camera->to_x = SDL_SwapLE32(cam_array[num_camera].camera_to_x);
	room_camera->to_y = SDL_SwapLE32(cam_array[num_camera].camera_to_y);
	room_camera->to_z = SDL_SwapLE32(cam_array[num_camera].camera_to_z);
}

static int rdt_getNumCamswitches(room_t *this)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_CAM_SWITCHES]);
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_switches = 0;

	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].to) != 0xffff) {
		if (SDL_SwapLE16(camswitch_array[i].to) != 9) {
			++num_switches;
		}

		++i;
	}

	return num_switches;
}

static void rdt_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_CAM_SWITCHES]);
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0;

	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].to) != 0xffff) {
		if (SDL_SwapLE16(camswitch_array[i].to) != 9) {
			if (j==num_camswitch) {
				break;
			}
			
			++j;
		}

		++i;
	}

	room_camswitch->from = SDL_SwapLE16(camswitch_array[i].from);
	room_camswitch->to = SDL_SwapLE16(camswitch_array[i].to);
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}

static int rdt_getNumBoundaries(room_t *this)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_CAM_SWITCHES]);
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_boundaries = 0;

	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].to) != 0xffff) {
		if (SDL_SwapLE16(camswitch_array[i].to) == 9) {
			++num_boundaries;
		}

		++i;
	}

	return num_boundaries;
}

static void rdt_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_camswitch)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_CAM_SWITCHES]);
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0;

	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].to) != 0xffff) {
		if (SDL_SwapLE16(camswitch_array[i].to) == 9) {
			if (j==num_boundary) {
				break;
			}

			++j;
		}

		++i;
	}

	room_camswitch->from = SDL_SwapLE16(camswitch_array[i].from);
	room_camswitch->to = SDL_SwapLE16(camswitch_array[i].to);
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}

static void rdt_drawMasks(room_t *this, int num_camera)
{
	Uint32 offset;
	rdt_camera_pos_t *cam_array;
	rdt_mask_header_t *mask_hdr;
	rdt_mask_offset_t *mask_offsets;
	int num_offset;

	if (num_camera>=this->num_cameras) {
		return;
	}

	if (game_state.bg_mask==NULL) {
		rdt_loadMasks(this, num_camera);
	}
	if (game_state.bg_mask==NULL) {
		return;
	}

	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[sizeof(rdt1_header_t)];

	offset = SDL_SwapLE32(cam_array[num_camera].masks_offset);
	if (offset == 0xffffffffUL) {
		return;
	}

	/*render.set_dithering(params.dithering);*/
	render.set_dithering(0);
	render.set_useDirtyRects(0);
	render.set_texture(0, game_state.bg_mask);
	render.bitmap.setMasking(1);
	render.bitmap.setScaler(
		game_state.bg_mask->w, game_state.bg_mask->h,
		(game_state.bg_mask->w*video.viewport.w)/320,
		(game_state.bg_mask->h*video.viewport.h)/240);
	render.set_blending(1);

	mask_hdr = (rdt_mask_header_t *) &((Uint8 *) this->file)[offset];
	offset += sizeof(rdt_mask_header_t);

	mask_offsets = (rdt_mask_offset_t *) &((Uint8 *) this->file)[offset];
	offset += sizeof(rdt_mask_offset_t) * SDL_SwapLE16(mask_hdr->num_offset);

	for (num_offset=0; num_offset<SDL_SwapLE16(mask_hdr->num_offset); num_offset++) {
		int num_mask;
		
		for (num_mask=0; num_mask<SDL_SwapLE16(mask_offsets->count); num_mask++) {
			rdt_mask_square_t *square_mask;
			int src_x, src_y, width, height, depth;
			int dst_x = SDL_SwapLE16(mask_offsets->dst_x);
			int dst_y = SDL_SwapLE16(mask_offsets->dst_y);
			int scaled_dst_x, scaled_dst_y;
			int scaled_dst_w, scaled_dst_h;

			square_mask = (rdt_mask_square_t *) &((Uint8 *) this->file)[offset];
			if (square_mask->size == 0) {
				/* Rect mask */
				rdt_mask_rect_t *rect_mask = (rdt_mask_rect_t *) square_mask;

				src_x = rect_mask->src_x;
				src_y = rect_mask->src_y;
				dst_x += rect_mask->dst_x;
				dst_y += rect_mask->dst_y;
				width = SDL_SwapLE16(rect_mask->width);
				height = SDL_SwapLE16(rect_mask->height);
				depth = SDL_SwapLE16(rect_mask->depth);

				offset += sizeof(rdt_mask_rect_t);
			} else {
				/* Square mask */

				src_x = square_mask->src_x;
				src_y = square_mask->src_y;
				dst_x += square_mask->dst_x;
				dst_y += square_mask->dst_y;
				width = height = square_mask->size;
				depth = SDL_SwapLE16(square_mask->depth);

				offset += sizeof(rdt_mask_square_t);
			}

			scaled_dst_x = (dst_x*video.viewport.w)/320;
			scaled_dst_y = (dst_y*video.viewport.h)/240;
			scaled_dst_w = (width*video.viewport.w)/320;
			scaled_dst_h = (height*video.viewport.h)/240;

			render.bitmap.clipSource(src_x,src_y,width,height);
			render.bitmap.clipDest(
				video.viewport.x+scaled_dst_x,
				video.viewport.y+scaled_dst_y,
				scaled_dst_w,scaled_dst_h);
			render.bitmap.setDepth(1, (float) depth * 16.0f);
			render.bitmap.drawImage(&video);
		}

		mask_offsets++;
	}

	render.bitmap.setDepth(0, 0.0f);
	render.bitmap.setMasking(0);
	render.set_blending(0);
	render.set_dithering(0);
}

static void rdt_loadMasks(room_t *this, int num_camera)
{
	Uint32 offset;
	rdt_camera_pos_t *cam_array;

	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[sizeof(rdt1_header_t)];

	offset = SDL_SwapLE32(cam_array[num_camera].tim_offset);
	if (offset == 0) {
		return;
	}

	game_state.bg_mask = render.createTexture(RENDER_TEXTURE_CACHEABLE|RENDER_TEXTURE_MUST_POT);
	if (game_state.bg_mask) {
		Uint8 *tim_hdr = (Uint8 *) &((Uint8 *) this->file)[offset];

		game_state.bg_mask->load_from_tim(game_state.bg_mask, tim_hdr);
		logMsg(1, "rdt: Loaded masks from embedded TIM image\n");
	}
}
