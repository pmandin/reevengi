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
#include "../clock.h"

#include "room.h"
#include "room_map.h"
#include "room_camswitch.h"
#include "room_door.h"
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

#define MAP_TRANSITION_TIME	1000

/*--- Variables ---*/

static Sint16 minx, maxx, minz, maxz;
static Uint32 clock_start;
static float mtx_proj_from[4][4], mtx_proj_to[4][4], mtx_proj_cur[4][4];
static float mtx_model_from[4][4], mtx_model_to[4][4], mtx_model_cur[4][4];

/*--- Functions prototypes ---*/

static void minMaxCameras(room_t *this);
static void minMaxCamswitches(room_t *this);
static void minMaxBoundaries(room_t *this);

static void initMatrix2D(room_t *this, float mtx_proj[4][4], float mtx_model[4][4]);
static void initMatrix3D(room_t *this, float mtx_proj[4][4], float mtx_model[4][4]);
static void calcMatrix(Uint32 elapsed);

static void toggleMapModePrev(room_t *this);
static void toggleMapModeNext(room_t *this);
static void drawMap(room_t *this);

static void drawCameras(room_t *this);

static void drawDoors(room_t *this);
/*static void room_map_drawObstacles(room_t *this);
static void room_map_drawItems(room_t *this);*/

static void drawPlayer(player_t *this);

/*--- Functions ---*/

void room_map_init(room_t *this)
{
	this->map_mode = ROOM_MAP_OFF;

	this->drawMap = drawMap;
	this->toggleMapModePrev = toggleMapModePrev;
	this->toggleMapModeNext = toggleMapModeNext;
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

static void toggleMapModePrev(room_t *this)
{
	switch(this->map_mode) {
		case ROOM_MAP_OFF:
			this->map_mode = ROOM_MAP_3D;
			logMsg(1, "map: ROOM_MAP_3D\n");
			break;
		case ROOM_MAP_2D:
			this->map_mode = ROOM_MAP_OFF;
			logMsg(1, "map: ROOM_MAP_OFF\n");
			break;
		case ROOM_MAP_3D:
			this->map_mode = ROOM_MAP_3D_TO_2D_INIT;
			logMsg(1, "map: ROOM_MAP_3D_TO_2D_INIT\n");
			break;
	}
}

static void toggleMapModeNext(room_t *this)
{
	switch(this->map_mode) {
		case ROOM_MAP_OFF:
			this->map_mode = ROOM_MAP_2D;
			logMsg(1, "map: ROOM_MAP_2D\n");
			break;
		case ROOM_MAP_2D:
			this->map_mode = ROOM_MAP_2D_TO_3D_INIT;
			logMsg(1, "map: ROOM_MAP_2D_TO_3D_INIT\n");
			break;
		case ROOM_MAP_3D:
			this->map_mode = ROOM_MAP_OFF;
			logMsg(1, "map: ROOM_MAP_OFF\n");
			break;
	}
}

static void initMatrix2D(room_t *this, float mtx_proj[4][4], float mtx_model[4][4])
{
	player_t *player=game->player;

	render.set_ortho(-20000.0f, 20000.0f, -20000.0f, 20000.0f, -1.0f, 1.0f);
	render.get_proj_matrix(mtx_proj);

	render.translate(-player->x, -player->z, 0.0f);
	render.get_model_matrix(mtx_model);
}

static void initMatrix3D(room_t *this, float mtx_proj[4][4], float mtx_model[4][4])
{
	room_camera_t room_camera;

	this->getCamera(this, game->num_camera, &room_camera);

	render.set_projection(60.0f, 4.0f/3.0f, RENDER_Z_NEAR, RENDER_Z_FAR);
	render.get_proj_matrix(mtx_proj);

	render.set_modelview(
		room_camera.from_x, room_camera.from_y, room_camera.from_z,
		room_camera.to_x, room_camera.to_y, room_camera.to_z,
		0.0f, -1.0f, 0.0f
	);
	render.get_model_matrix(mtx_model);
}

static void calcMatrix(Uint32 elapsed)
{
	float pos = (float) elapsed / (float) MAP_TRANSITION_TIME;
	int i,j;

	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			mtx_proj_cur[i][j]=mtx_proj_from[i][j]+
				((mtx_proj_to[i][j]-mtx_proj_from[i][j])*pos);
			mtx_model_cur[i][j]=mtx_model_cur[i][j]+
				((mtx_model_cur[i][j]-mtx_model_cur[i][j])*pos);
		}
	}
}

static void drawMap(room_t *this)
{
	player_t *player=game->player;
	Uint32 elapsed;
	switch(this->map_mode) {
		case ROOM_MAP_2D_TO_3D_INIT:
			{
				clock_start = clockGet();
				this->map_mode = ROOM_MAP_2D_TO_3D_CALC;
				initMatrix2D(this, mtx_proj_from, mtx_model_from);
				initMatrix3D(this, mtx_proj_to, mtx_model_to);
				logMsg(1, "map: ROOM_MAP_2D_TO_3D_CALC\n");
			}
			break;
		case ROOM_MAP_2D_TO_3D_CALC:
			{
				elapsed = clockGet() - clock_start;
				if (elapsed > MAP_TRANSITION_TIME) {
					this->map_mode = ROOM_MAP_3D;
					logMsg(1, "map: ROOM_MAP_3D\n");
				} else {
					calcMatrix(elapsed);
				}
			}
			break;
		case ROOM_MAP_3D_TO_2D_INIT:
			{
				clock_start = clockGet();
				this->map_mode = ROOM_MAP_3D_TO_2D_CALC;
				initMatrix3D(this, mtx_proj_from, mtx_model_from);
				initMatrix2D(this, mtx_proj_to, mtx_model_to);
				logMsg(1, "map: ROOM_MAP_3D_TO_2D_CALC\n");
			}
			break;
		case ROOM_MAP_3D_TO_2D_CALC:
			{
				elapsed = clockGet() - clock_start;
				if (elapsed > MAP_TRANSITION_TIME) {
					this->map_mode = ROOM_MAP_2D;
					logMsg(1, "map: ROOM_MAP_2D\n");
				} else {
					calcMatrix(elapsed);
				}
			}
			break;
	}


	switch(this->map_mode) {
		case ROOM_MAP_2D:
			/*logMsg(1, "%d %d %d %d\n",minx,maxx,minz,maxz);
			render.set_ortho(minx*0.5f,maxx*0.5f, minz*0.5f,maxz*0.5f, -1.0f, 1.0f);*/
			render.set_ortho(-20000.0f, 20000.0f, -20000.0f, 20000.0f, -1.0f, 1.0f);
			/*render.rotate((player->a * 360.0f) / 4096.0f, 0.0f,1.0f,0.0f);*/
			render.translate(-player->x, -player->z, 0.0f);
			break;
		case ROOM_MAP_3D:
			/* Keep current projection */
			break;		
	}

	render.set_color(MAP_COLOR_BOUNDARY);
	this->drawBoundaries(this);

	render.set_color(MAP_COLOR_CAMSWITCH);
	this->drawCamSwitches(this);

	if (this->map_mode == ROOM_MAP_2D) {
		drawCameras(this);
	}

	/*drawObstacles(this);
	drawItems(this);*/
	drawDoors(this);

	if (this->map_mode == ROOM_MAP_2D) {
		drawPlayer(player);
	}
}

static void drawCameras(room_t *this)
{
	int i;

	for (i=0; i<this->num_cameras; i++) {
		room_camera_t room_camera;
		vertex_t v[2];
		float dx, dy, adx, ady, angle, radius;

		if (i!=game->num_camera) {
			continue;
		}

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
		/*break;*/
	}
}

static void drawDoors(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_DOOR);

	for (i=0; i<this->num_doors; i++) {
		room_door_t *door = &this->doors[i];
		vertex_t v[4];

#if 1
		v[0].x = door->x;
		v[0].y = door->y;
		v[0].z = 1.0f;

		v[1].x = door->x+door->w;
		v[1].y = door->y;
		v[1].z = 1.0f;

		render.line(&v[0], &v[1]);

		v[0].x = door->x+door->w;
		v[0].y = door->y+door->h;
		v[0].z = 1.0f;

		render.line(&v[0], &v[1]);

		v[1].x = door->x;
		v[1].y = door->y+door->h;
		v[1].z = 1.0f;

		render.line(&v[0], &v[1]);

		v[0].x = door->x;
		v[0].y = door->y;
		v[0].z = 1.0f;

		render.line(&v[0], &v[1]);
#else
		v[0].x = door->x;
		v[0].y = door->y;
		v[0].z = 1.0f;

		v[1].x = door->x+door->w;
		v[1].y = door->y;
		v[1].z = 1.0f;

		v[2].x = door->x+door->w;
		v[2].y = door->y+door->h;
		v[2].z = 1.0f;

		v[3].x = door->x;
		v[3].y = door->y+door->h;
		v[3].z = 1.0f;

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
#endif
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

	render.set_color(MAP_COLOR_PLAYER);

	v[0].x = (x - radius * cos((-angle * M_PI) / 2048.0f) * 0.5f);
	v[0].y = (y - radius * sin((-angle * M_PI) / 2048.0f) * 0.5f);
	v[0].z = 1.0f;

	v[1].x = (x + radius * cos((-angle * M_PI) / 2048.0f) * 0.5f);
	v[1].y = (y + radius * sin((-angle * M_PI) / 2048.0f) * 0.5f);
	v[1].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);
	v[0].y = (y + radius * sin((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);
	v[0].y = (y + radius * sin((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);
}
