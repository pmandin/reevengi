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
#include "log.h"

/*--- Types ---*/

typedef struct {
	Sint32	x,y,z;
	Uint32	unknown[2];
} rdt_header_part_t;

typedef struct {
	Uint8	unknown0;
	Uint8	num_cameras;
	Uint8	unknown1[4];
	Uint16	unknown2[3];
	rdt_header_part_t	unknown3[3];
} rdt_header_t;

typedef struct {
	Uint32 unknown0;
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

/*--- Functions prototypes ---*/

static void rdt_getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static int rdt_getNumCamswitches(room_t *this);
static void rdt_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

static int rdt_getNumBoundaries(room_t *this);
static void rdt_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_camswitch);

/*--- Functions ---*/

void room_rdt_init(room_t *this)
{
	rdt_header_t *rdt_header = (rdt_header_t *) this->file;

	if (this->file_length>4) {
		this->num_cameras = rdt_header->num_cameras;
		this->num_camswitches = rdt_getNumCamswitches(this);
		this->num_boundaries = rdt_getNumBoundaries(this);

		this->getCamera = rdt_getCamera;
		this->getCamswitch = rdt_getCamswitch;
		this->getBoundary = rdt_getBoundary;
	}

	logMsg(2, "%d cameras angles, %d camera switches, %d boundaries\n",
		this->num_cameras, this->num_camswitches, this->num_boundaries);
}

static void rdt_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
	rdt_camera_pos_t *cam_array;
	
	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[sizeof(rdt_header_t)+19*4];

	room_camera->from_x = SDL_SwapLE32(cam_array[num_camera].camera_from_x);
	room_camera->from_y = SDL_SwapLE32(cam_array[num_camera].camera_from_y);
	room_camera->from_z = SDL_SwapLE32(cam_array[num_camera].camera_from_z);
	room_camera->to_x = SDL_SwapLE32(cam_array[num_camera].camera_to_x);
	room_camera->to_y = SDL_SwapLE32(cam_array[num_camera].camera_to_y);
	room_camera->to_z = SDL_SwapLE32(cam_array[num_camera].camera_to_z);
}

static int rdt_getNumCamswitches(room_t *this)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_switches = 0;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[sizeof(rdt_header_t)+0*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
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
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[sizeof(rdt_header_t)+0*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
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
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_boundaries = 0, prev_from = -1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[sizeof(rdt_header_t)+0*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
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
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[sizeof(rdt_header_t)+0*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
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
