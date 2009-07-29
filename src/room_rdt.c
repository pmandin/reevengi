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

/*--- Types ---*/

typedef struct {
	Sint32 camera_from_x;
	Sint32 camera_from_y;
	Sint32 camera_from_z;
	Sint32 camera_to_x;
	Sint32 camera_to_y;
	Sint32 camera_to_z;
	Uint32 unknown[5];
} rdt_camera_pos_t;

/*--- Functions prototypes ---*/

static void rdt_getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

/*--- Functions ---*/

void room_rdt_init(room_t *this)
{
	Uint8 *rdt_header = (Uint8 *) this->file;

	this->num_cameras = rdt_header[1];

	this->getCamera = rdt_getCamera;
}

static void rdt_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
	rdt_camera_pos_t *cam_array;
	
	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[0x9c];

	room_camera->from_x = SDL_SwapLE32(cam_array[num_camera].camera_from_x);
	room_camera->from_y = SDL_SwapLE32(cam_array[num_camera].camera_from_y);
	room_camera->from_z = SDL_SwapLE32(cam_array[num_camera].camera_from_z);
	room_camera->to_x = SDL_SwapLE32(cam_array[num_camera].camera_to_x);
	room_camera->to_y = SDL_SwapLE32(cam_array[num_camera].camera_to_y);
	room_camera->to_z = SDL_SwapLE32(cam_array[num_camera].camera_to_z);
}
