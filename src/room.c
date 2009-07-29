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
#include <math.h>

#include <SDL.h>

#include "video.h"
#include "render.h"
#include "room.h"
#include "state.h"

/*--- Defines ---*/

#define MAP_COLOR_CAMERA_DISABLED	0x00666666
#define MAP_COLOR_CAMERA_ENABLED	0x00ffffff
#define MAP_COLOR_CAMSWITCH_DISABLED	0x00664433
#define MAP_COLOR_CAMSWITCH_ENABLED	0x00ffcc88
#define MAP_COLOR_BOUNDARY_DISABLED	0x00660000
#define MAP_COLOR_BOUNDARY_ENABLED	0x00ff0000
#define MAP_COLOR_PLAYER		0x0000ff00

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
	/*printf("init map %d,%d %d,%d\n",minx,maxx,minz,maxz);*/
	room_map_minMaxCamswitches(this);
	/*printf("init map %d,%d %d,%d\n",minx,maxx,minz,maxz);*/
	room_map_minMaxBoundaries(this);
	/*printf("init map %d,%d %d,%d\n",minx,maxx,minz,maxz);*/

	/* Add 5% around */
	range = maxx-minx;

	v = minx -(range*5)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	minx = v;

	v = maxx + (range*5)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	maxx = v;

	range = maxz-minz;

	v = minz - (range*5)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	minz = v;

	v = maxz + (range*5)/100;
	if (v<-32768) {
		v = -32768;
	} else if (v>32767) {
		v = 32767;
	}
	maxz = v;

	/*printf("init map %d,%d %d,%d\n",minx,maxx,minz,maxz);*/
}

static void room_map_minMaxCameras(room_t *this)
{
	int i;

	for (i=0; i<this->num_cameras; i++) {
		room_camera_t room_camera;
		Sint16 v;

		this->getCamera(this, i, &room_camera);

		/*printf("cam %d: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
			room_camera.from_x, room_camera.from_z,
			room_camera.to_x, room_camera.to_z);*/

		v = room_camera.from_x;
		if (v < minx) {
			minx = v;
		}
		if (v > maxx) {
			maxx = v;
		}

		v = room_camera.to_x;
		if (v < minx) {
			minx = v;
		}
		if (v > maxx) {
			maxx = v;
		}

		v = room_camera.from_z;
		if (v < minz) {
			minz = v;
		}
		if (v > maxz) {
			maxz = v;
		}

		v = room_camera.to_z;
		if (v < minz) {
			minz = v;
		}
		if (v > maxz) {
			maxz = v;
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
	render.set_texture(0, NULL);
	render.push_matrix();

	/*printf("draw map %d,%d %d,%d\n",minx,maxx,minz,maxz);*/

	/* Set ortho projection */
	render.set_ortho(minx,maxx, minz,maxz, -1.0f,1.0f);
	/*render.set_identity();*/

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

	for (i=0; i<this->num_cameras; i++) {
		room_camera_t room_camera;
		vertex_t v[2];
		float dx, dy, adx, ady, angle, radius;

		this->getCamera(this, i, &room_camera);

		dx = room_camera.to_x - room_camera.from_x;
		dy = room_camera.to_z - room_camera.from_z;
		adx = abs(dx);
		ady = abs(dy);
		radius = sqrt(dx*dx+dy*dy);
		if (adx>ady) {
			angle = atan(ady/adx);
		} else {
			angle = (M_PI/2.0f) - atan(adx/ady);
		}
		if (dx<0) {
			angle = M_PI - angle;
		}
		if (dy<0) {
			angle = M_PI*2.0f - angle;
		}
		angle = (angle * 180.0f) / M_PI;

		render.set_color((i==game_state.num_camera) ?
			MAP_COLOR_CAMERA_ENABLED :
			MAP_COLOR_CAMERA_DISABLED);

		v[0].x = room_camera.from_x;
		v[0].y = room_camera.from_z;
		v[0].z = 1.0f;

		v[1].x = room_camera.from_x + radius * cos(((angle+30.0f)*M_PI)/180.0f);
		v[1].y = room_camera.from_z + radius * sin(((angle+30.0f)*M_PI)/180.0f);
		v[1].z = 1.0f;

		render.line(&v[0], &v[1]);

		v[1].x = room_camera.from_x + radius * cos(((angle-30.0f)*M_PI)/180.0f);
		v[1].y = room_camera.from_z + radius * sin(((angle-30.0f)*M_PI)/180.0f);
		v[1].z = 1.0f;

		render.line(&v[0], &v[1]);
	}
}

static void room_map_drawCamswitches(room_t *this)
{
	int i, j, prev_from=-1;

	for (i=0; i<this->num_camswitches; i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[2];
		int boundary = 0;

		this->getCamswitch(this, i, &room_camswitch);

		render.set_color((room_camswitch.from==game_state.num_camera) ?
			MAP_COLOR_CAMSWITCH_ENABLED :
			MAP_COLOR_CAMSWITCH_DISABLED);

		for (j=0; j<4; j++) {
			v[0].x = room_camswitch.x[j];
			v[0].y = room_camswitch.y[j];
			v[0].z = 1.0f;

			v[1].x = room_camswitch.x[(j+1) & 3];
			v[1].y = room_camswitch.y[(j+1) & 3];
			v[1].z = 1.0f;

			render.line(&v[0], &v[1]);
		}
	}
}

static void room_map_drawBoundaries(room_t *this)
{
	int i, j, prev_from=-1;

	for (i=0; i<this->num_boundaries; i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[2];
		int boundary = 0;

		this->getBoundary(this, i, &room_camswitch);

		render.set_color((room_camswitch.from==game_state.num_camera) ?
			MAP_COLOR_BOUNDARY_ENABLED :
			MAP_COLOR_BOUNDARY_DISABLED);

		for (j=0; j<4; j++) {
			v[0].x = room_camswitch.x[j];
			v[0].y = room_camswitch.y[j];
			v[0].z = 1.0f;

			v[1].x = room_camswitch.x[(j+1) & 3];
			v[1].y = room_camswitch.y[(j+1) & 3];
			v[1].z = 1.0f;

			render.line(&v[0], &v[1]);
		}
	}
}

void room_map_drawPlayer(float x, float y, float angle)
{
	vertex_t v[2];
	const float radius = 2000.0f;

	render.set_texture(0, NULL);
	render.push_matrix();

	/* Set ortho projection */
	render.set_ortho(minx,maxx, minz,maxz, -1.0f,1.0f);

	render.set_color(MAP_COLOR_PLAYER);

	v[0].x = x - radius * cos((-angle * M_PI) / 180.0f) * 0.5f;
	v[0].y = y - radius * sin((-angle * M_PI) / 180.0f) * 0.5f;
	v[0].z = 1.0f;

	v[1].x = x + radius * cos((-angle * M_PI) / 180.0f) * 0.5f;
	v[1].y = y + radius * sin((-angle * M_PI) / 180.0f) * 0.5f;
	v[1].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = x + radius * cos((-(angle-20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f;
	v[0].y = y + radius * sin((-(angle-20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f;
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = x + radius * cos((-(angle+20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f;
	v[0].y = y + radius * sin((-(angle+20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f;
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);

	render.pop_matrix();
}

int room_checkBoundary(room_t *this, float x, float y)
{
	int i;

	for (i=0; i<this->num_boundaries; i++) {
		room_camswitch_t room_camswitch;

		this->getBoundary(this, i, &room_camswitch);
	}

	return 0;
}

int room_checkCamswitch(room_t *this, float x, float y)
{
	int i;

	for (i=0; i<this->num_camswitches; i++) {
		room_camswitch_t room_camswitch;

		this->getCamswitch(this, i, &room_camswitch);
	}

	return -1;
}
