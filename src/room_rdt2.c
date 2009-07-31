/*
	Room description
	RE2 RDT manager

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
#include "room_rdt2_items.h"
#include "log.h"

/*--- Types ---*/

typedef struct {
	Uint16 unk0;
	Uint16 const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	Sint32 camera_from_x;
	Sint32 camera_from_y;
	Sint32 camera_from_z;
	Sint32 camera_to_x;
	Sint32 camera_to_y;
	Sint32 camera_to_z;
	Uint32 offset;
} rdt_camera_pos_t;

typedef struct {
	Uint16 const0; /* 0xff01 */
	Uint8 from,to;
	Sint16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
} rdt_camera_switch_t;

/*--- Functions prototypes ---*/

static void rdt2_getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static int rdt2_getNumCamswitches(room_t *this);
static void rdt2_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

static int rdt2_getNumBoundaries(room_t *this);
static void rdt2_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_camswitch);

/*--- Functions ---*/

void room_rdt2_init(room_t *this)
{
	Uint8 *rdt_header = (Uint8 *) this->file;

	if (this->file_length > 4) {
		this->num_cameras = rdt_header[1];
		this->num_camswitches = rdt2_getNumCamswitches(this);
		this->num_boundaries = rdt2_getNumBoundaries(this);

		this->getCamera = rdt2_getCamera;
		this->getCamswitch = rdt2_getCamswitch;
		this->getBoundary = rdt2_getBoundary;

		room_rdt2_listItems(this);

		this->drawItems = room_rdt2_drawItems;
	}

	logMsg(2, "%d cameras angles, %d camera switches, %d boundaries\n",
		this->num_cameras, this->num_camswitches, this->num_boundaries);
}

static void rdt2_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
	Uint32 *cams_offset, offset;
	rdt_camera_pos_t *cam_array;
	
	cams_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+7*4]);
	offset = SDL_SwapLE32(*cams_offset);
	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[offset];

	room_camera->from_x = SDL_SwapLE32(cam_array[num_camera].camera_from_x);
	room_camera->from_y = SDL_SwapLE32(cam_array[num_camera].camera_from_y);
	room_camera->from_z = SDL_SwapLE32(cam_array[num_camera].camera_from_z);
	room_camera->to_x = SDL_SwapLE32(cam_array[num_camera].camera_to_x);
	room_camera->to_y = SDL_SwapLE32(cam_array[num_camera].camera_to_y);
	room_camera->to_z = SDL_SwapLE32(cam_array[num_camera].camera_to_z);
}

static int rdt2_getNumCamswitches(room_t *this)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_switches = 0, prev_from = -1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary=0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			/* boundary, not a switch */
		} else {
			num_switches++;
		}

		++i;
	}

	return num_switches;
}

static void rdt2_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary = 0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			/* boundary, not a switch */
		} else {
			if (j==num_camswitch) {
				break;
			}

			++j;
		}

		++i;
	}

	room_camswitch->from = camswitch_array[i].from;
	room_camswitch->to = camswitch_array[i].to;
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}

static int rdt2_getNumBoundaries(room_t *this)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_boundaries = 0, prev_from = -1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary=0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			++num_boundaries;
		} else {
			/* switch, not a boundary */
		}

		++i;
	}

	return num_boundaries;
}

static void rdt2_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_camswitch)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary = 0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			if (j==num_boundary) {
				break;
			}

			++j;
		} else {
			/* switch, not a boundary */
		}

		++i;
	}

	room_camswitch->from = camswitch_array[i].from;
	room_camswitch->to = camswitch_array[i].to;
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}
