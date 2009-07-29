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

#include "video.h"
#include "render.h"
#include "room.h"

/*--- Defines ---*/

#define MAP_COLOR_CAMERA	0x00ffffff
#define MAP_COLOR_CAMSWITCH	0x00cc8844
#define MAP_COLOR_BOUNDARY	0x00ff0000

/*--- Types ---*/

/*--- Variables ---*/

static Sint16 minx, maxx, minz, maxz;

/*--- Functions prototypes ---*/

static void shutdown(room_t *this);

static void room_map_minMaxCameras(room_t *this);
static void room_map_minMaxCamswitches(room_t *this);
static void room_map_minMaxBoundaries(room_t *this);

static void room_map_drawCameras(room_t *this);
static void room_map_drawCamswitches(room_t *this);
static void room_map_drawBoundaries(room_t *this);

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
	int range, v;

	minx = minz = 32767;
	maxx = maxz = -32768;

	room_map_minMaxCameras(this);
	room_map_minMaxCamswitches(this);
	room_map_minMaxBoundaries(this);

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

static void room_map_minMaxCameras(room_t *this)
{
	int i;

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
		if ((room_camera.from_z>>16) > maxz) {
			maxz = room_camera.from_z>>16;
		}

		if ((room_camera.to_z>>16) < minz) {
			minz = room_camera.to_z>>16;
		}
		if ((room_camera.to_z>>16) > maxz) {
			maxz = room_camera.to_z>>16;
		}
	}
}

static void room_map_minMaxCamswitches(room_t *this)
{
	int i, j;

	for (i=0; i<this->num_camswitches; i++) {
		room_camswitch_t room_camswitch;

		this->getCamswitch(this, i, &room_camswitch);

		for (j=0; j<4; j++) {
			if (room_camswitch.x[j] < minx) {
				minx = room_camswitch.x[j];
			}
			if (room_camswitch.x[j] > maxx) {
				maxx = room_camswitch.x[j];
			}

			if (room_camswitch.y[j] < minz) {
				minz = room_camswitch.y[j];
			}
			if (room_camswitch.y[j] > maxz) {
				maxz = room_camswitch.y[j];
			}
		}
	}
}

static void room_map_minMaxBoundaries(room_t *this)
{
	int i, j;

	for (i=0; i<this->num_boundaries; i++) {
		room_camswitch_t room_camswitch;

		this->getBoundary(this, i, &room_camswitch);

		for (j=0; j<4; j++) {
			if (room_camswitch.x[j] < minx) {
				minx = room_camswitch.x[j];
			}
			if (room_camswitch.x[j] > maxx) {
				maxx = room_camswitch.x[j];
			}

			if (room_camswitch.y[j] < minz) {
				minz = room_camswitch.y[j];
			}
			if (room_camswitch.y[j] > maxz) {
				maxz = room_camswitch.y[j];
			}
		}
	}
}

void room_map_draw(room_t *this)
{
	render.push_matrix();

	/* Set ortho projection */
	render.set_identity();
	render.scale(2.0f / (maxx - minx), 2.0f / (maxz - minz), 2.0f / (1.0f - -1.0f));

	room_map_drawBoundaries(this);
	room_map_drawCamswitches(this);
	room_map_drawCameras(this);

	/* Draw doors */

	/* Draw objects */

	/* Draw enemies */

	/* Draw player */

	render.pop_matrix();
}

static void room_map_drawCameras(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_CAMERA);
	for (i=0; i<this->num_cameras; i++) {
		room_camera_t room_camera;
		vertex_t v[2];

		this->getCamera(this, i, &room_camera);

		v[0].x = room_camera.from_x>>16;
		v[0].y = room_camera.from_z>>16;
		v[0].z = 0;

		v[1].x = room_camera.to_x>>16;
		v[1].y = room_camera.to_z>>16;
		v[1].z = 0;

		render.line(&v[0], &v[1]);
	}
}

static void room_map_drawCamswitches(room_t *this)
{
	int i, j;

	render.set_color(MAP_COLOR_CAMSWITCH);
	for (i=0; i<this->num_camswitches; i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[2];

		this->getCamswitch(this, i, &room_camswitch);

		for (j=0; j<4; j++) {
			v[0].x = room_camswitch.x[j];
			v[0].y = room_camswitch.y[j];
			v[0].z = 0;

			v[1].x = room_camswitch.x[(j+1) & 3];
			v[1].y = room_camswitch.y[(j+1) & 3];
			v[1].z = 0;

			render.line(&v[0], &v[1]);
		}
	}
}

static void room_map_drawBoundaries(room_t *this)
{
	int i, j;

	render.set_color(MAP_COLOR_BOUNDARY);
	for (i=0; i<this->num_boundaries; i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[2];

		this->getBoundary(this, i, &room_camswitch);

		for (j=0; j<4; j++) {
			v[0].x = room_camswitch.x[j];
			v[0].y = room_camswitch.y[j];
			v[0].z = 0;

			v[1].x = room_camswitch.x[(j+1) & 3];
			v[1].y = room_camswitch.y[(j+1) & 3];
			v[1].z = 0;

			render.line(&v[0], &v[1]);
		}
	}
}
