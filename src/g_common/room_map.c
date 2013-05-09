/*
	Room map

	Copyright (C) 2007-2013	Patrice Mandin

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

#include <math.h>
#include <string.h>
#include <SDL.h>

#include "../log.h"

#include "../render.h"

#include "room.h"
#include "room_map.h"
#include "player.h"
#include "game.h"

/*--- Defines ---*/

#define MAP_COLOR_CAMERA	0x00ffffff
#define MAP_COLOR_CAMSWITCH	0x00ffcc88
#define MAP_COLOR_BOUNDARY	0x00ff0000

#define MAP_COLOR_DOOR			0x0000cccc
#define MAP_COLOR_OBSTACLE		0x00ffcc00
#define MAP_COLOR_ITEM			0x00ffff00
#define MAP_COLOR_WALLS			0x00ff00ff
#define MAP_COLOR_PLAYER		0x0000ff00

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/*--- Variables ---*/

static Sint16 minx, maxx, minz, maxz;

/*--- Functions prototypes ---*/

static void minMaxCameras(room_t *this);
static void minMaxCamswitches(room_t *this);
static void minMaxBoundaries(room_t *this);

static void drawMap(room_t *this);

static void drawCameras(room_t *this);
static void drawCamswitches(room_t *this);
static void drawBoundaries(room_t *this);

/*static void room_map_drawDoors(room_t *this);
static void room_map_drawObstacles(room_t *this);
static void room_map_drawItems(room_t *this);*/

static void drawPlayer(player_t *this);

/*--- Functions ---*/

void room_map_init(room_t *this)
{
	this->drawMap = drawMap;
}

void room_map_init_data(room_t *this)
{
	int range, v;

	minx = minz = 32767;
	maxx = maxz = -32768;

	minMaxCameras(this);
	minMaxCamswitches(this);
	minMaxBoundaries(this);

	/* Add 5% around */
	range = maxx-minx;

	v = minx -(range*5)/100;
	minx = MIN(MAX(v,-32768),32767);
	v = maxx + (range*5)/100;
	maxx = MIN(MAX(v,-32768),32767);

	range = maxz-minz;

	v = minz - (range*5)/100;
	minz = MIN(MAX(v,-32768),32767);

	v = maxz + (range*5)/100;
	maxz = MIN(MAX(v,-32768),32767);

	if (maxz-minz > maxx-minx) {
		minx = minz;
		maxx = maxz;
	} else {
		minz = minx;
		maxz = maxx;
	}
}

static void minMaxCameras(room_t *this)
{
	int i;

	for (i=0; i<this->num_cameras; i++) {
		room_camera_t room_camera;

		this->getCamera(this, i, &room_camera);

		minx = MIN(room_camera.from_x, minx);
		maxx = MAX(room_camera.from_x, maxx);

		minx = MIN(room_camera.to_x, minx);
		maxx = MAX(room_camera.to_x, maxx);

		minz = MIN(room_camera.from_z, minz);
		maxz = MAX(room_camera.from_z, maxz);

		minz = MIN(room_camera.to_z, minz);
		maxz = MAX(room_camera.to_z, maxz);
	}
}

static void minMaxCamswitches(room_t *this)
{
	int i, j;

	for (i=0; i<this->getNumCamSwitches(this); i++) {
		room_camswitch_t room_camswitch;

		this->getCamSwitch(this, i, &room_camswitch);

		for (j=0; j<4; j++) {
			minx = MIN(room_camswitch.x[j], minx);
			maxx = MAX(room_camswitch.x[j], maxx);

			minz = MIN(room_camswitch.y[j], minz);
			maxz = MAX(room_camswitch.y[j], maxz);
		}
	}
}

static void minMaxBoundaries(room_t *this)
{
	int i, j;

	for (i=0; i<this->getNumBoundaries(this); i++) {
		room_camswitch_t room_camswitch;

		this->getBoundary(this, i, &room_camswitch);

		for (j=0; j<4; j++) {
			minx = MIN(room_camswitch.x[j], minx);
			maxx = MAX(room_camswitch.x[j], maxx);

			minz = MIN(room_camswitch.y[j], minz);
			maxz = MAX(room_camswitch.y[j], maxz);
		}
	}
}

static void drawMap(room_t *this)
{
	/* Set ortho projection */
	render.set_ortho(minx*0.5f,maxx*0.5f, minz*0.5f,maxz*0.5f, -1.0f, 1.0f);

	drawBoundaries(this);
	drawCamswitches(this);
	drawCameras(this);

	/*drawObstacles(this);
	drawItems(this);
	drawDoors(this);*/

	drawPlayer(game->player);
}

static void drawCameras(room_t *this)
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

		render.set_color(MAP_COLOR_CAMERA);

		v[0].x = room_camera.from_x * 0.5f;
		v[0].y = room_camera.from_z * 0.5f;
		v[0].z = 1.0f;

		v[1].x = (room_camera.from_x + radius * cos(((angle+30.0f)*M_PI)/180.0f)) * 0.5f;
		v[1].y = (room_camera.from_z + radius * sin(((angle+30.0f)*M_PI)/180.0f)) * 0.5f;
		v[1].z = 1.0f;

		render.line(&v[0], &v[1]);

		v[1].x = (room_camera.from_x + radius * cos(((angle-30.0f)*M_PI)/180.0f)) * 0.5f;
		v[1].y = (room_camera.from_z + radius * sin(((angle-30.0f)*M_PI)/180.0f)) * 0.5f;
		v[1].z = 1.0f;

		render.line(&v[0], &v[1]);
		/*break;*/
	}
}

static void drawCamswitches(room_t *this)
{
	int i, j;

	for (i=0; i<this->getNumCamSwitches(this); i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[4];

		this->getCamSwitch(this, i, &room_camswitch);

		render.set_color(MAP_COLOR_CAMSWITCH);

		for (j=0; j<4; j++) {
			v[j].x = room_camswitch.x[j] * 0.5f;
			v[j].y = room_camswitch.y[j] * 0.5f;
			v[j].z = 1.0f;
		}

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
	}
}

static void drawBoundaries(room_t *this)
{
	int i, j;

	for (i=0; i<this->getNumBoundaries(this); i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[4];

		this->getBoundary(this, i, &room_camswitch);

		render.set_color(MAP_COLOR_BOUNDARY);

		for (j=0; j<4; j++) {
			v[j].x = room_camswitch.x[j] * 0.5f;
			v[j].y = room_camswitch.y[j] * 0.5f;
			v[j].z = 1.0f;
		}

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
	}
}

void drawPlayer(player_t *player)
{
	vertex_t v[2];
	const float radius = 2000.0f;
	float x = player->x;
	float y = player->z;
	float angle = player->a;

	render.set_texture(0, NULL);

	/* Set ortho projection */
	render.set_ortho(minx * 0.5f,maxx * 0.5f, minz * 0.5f,maxz * 0.5f, -1.0f,1.0f);

	render.set_color(MAP_COLOR_PLAYER);

	v[0].x = (x - radius * cos((-angle * M_PI) / 2048.0f) * 0.5f) * 0.5f;
	v[0].y = (y - radius * sin((-angle * M_PI) / 2048.0f) * 0.5f) * 0.5f;
	v[0].z = 1.0f;

	v[1].x = (x + radius * cos((-angle * M_PI) / 2048.0f) * 0.5f) * 0.5f;
	v[1].y = (y + radius * sin((-angle * M_PI) / 2048.0f) * 0.5f) * 0.5f;
	v[1].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].y = (y + radius * sin((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].y = (y + radius * sin((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);
}
