/*
	Room description

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

#include <stdlib.h>
#include <SDL.h>

#include "room.h"

/*--- Types ---*/

/*--- Variables ---*/

static Sint16 minx, maxx, minz, maxz;

/*--- Functions prototypes ---*/

static void shutdown(room_t *this);

/*--- Functions ---*/

room_t *room_create(void *room_file)
{
	room_t *this = calloc(1, sizeof(room_t));
	if (!this) {
		return NULL;
	}

	this->file = room_file;
	this->shutdown = shutdown;

	return this;
}

static void shutdown(room_t *this)
{
	if (this) {
		if (this->file) {
			free(this->file);
			this->file = NULL;
		}
		free(this);
	}
}

void room_map_init(room_t *this)
{
	int i, range, v;

	minx = minz = 32767;
	maxx = maxz = -32768;

	/* Found boundaries in cameras */
	for (i=0; i<this->num_cameras; i++) {
		room_camera_t room_camera;

		this->getCamera(this, i, &room_camera);

		if ((room_camera.from_x>>16) < minx) {
			minx = room_camera.from_x>>16;
		}
		if ((room_camera.from_x>>16) > maxx) {
			maxx = room_camera.from_x>>16;
		}

		if ((room_camera.to_x>>16) < minx) {
			minx = room_camera.to_x>>16;
		}
		if ((room_camera.to_x>>16) > maxx) {
			maxx = room_camera.to_x>>16;
		}

		if ((room_camera.from_z>>16) < minz) {
			minz = room_camera.from_z>>16;
		}
		if ((room_camera.from_z>>16) > maxx) {
			maxz = room_camera.from_z>>16;
		}

		if ((room_camera.to_z>>16) < minz) {
			minz = room_camera.to_z>>16;
		}
		if ((room_camera.to_z>>16) > maxx) {
			maxz = room_camera.to_z>>16;
		}
	}

	/* Found boundaries in camera switches */

	/* Found boundaries in object limits */

	/* Add 10% around */
	range = maxx-minx;

	v = minx -(range*10)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	minx = v;

	v = maxx + (range*10)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	maxx = v;

	range = maxz-minz;

	v = minz - (range*10)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	minz = v;

	v = maxz - (range*10)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	maxz = v;
}
