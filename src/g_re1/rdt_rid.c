/*
	RE1 RID
	Camera positions

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

#include "../g_common/room.h"

#include "rdt.h"
#include "rdt_rid.h"

/*--- Functions ---*/

int rdt1_rid_getNumCameras(room_t *this)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;

	if ((!this->file) || (this->file_length<4)) {
		return 0;
	}

	return rdt_header->num_cameras;
}

void rdt1_rid_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
	rdt1_rid_t *cam_array;

	if ((!this->file) || (this->file_length<4)) {
		return;
	}

	cam_array = (rdt1_rid_t *) &((Uint8 *) this->file)[sizeof(rdt1_header_t)];

	room_camera->from_x = SDL_SwapLE32(cam_array[num_camera].camera_from_x);
	room_camera->from_y = SDL_SwapLE32(cam_array[num_camera].camera_from_y);
	room_camera->from_z = SDL_SwapLE32(cam_array[num_camera].camera_from_z);
	room_camera->to_x = SDL_SwapLE32(cam_array[num_camera].camera_to_x);
	room_camera->to_y = SDL_SwapLE32(cam_array[num_camera].camera_to_y);
	room_camera->to_z = SDL_SwapLE32(cam_array[num_camera].camera_to_z);
}
