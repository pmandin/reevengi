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
#include "../r_soft/matrix.h"

#include "room.h"
#include "room_map.h"
#include "room_camswitch.h"
#include "room_door.h"
#include "room_item.h"
#include "player.h"
#include "game.h"

/*--- Defines ---*/

#define MAP_COLOR_CAMERA	0x00ffffff
#define MAP_COLOR_CAMSWITCH	0x00ffcc88
#define MAP_COLOR_BOUNDARY	0x00ff0000
#define MAP_COLOR_GRID		0x00202020

#define MAP_COLOR_DOOR			0x0000cccc
#define MAP_COLOR_ITEM			0x00ffff00
#define MAP_COLOR_PLAYER		0x0000ff00
#define MAP_COLOR_COLLISION		0x00ffc0ff

#define MAP_COLOR_OBSTACLE		0x00ffcc00
#define MAP_COLOR_WALLS			0x00ff00ff

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

static void setProjection2D(room_t *this);
static void setProjection3D(room_t *this);
static void initMatrix(room_t *this, float mtx_proj[4][4], float mtx_model[4][4]);
static void calcMatrix(Uint32 elapsed);

static void toggleMapModePrev(room_t *this);
static void toggleMapModeNext(room_t *this);
static void drawMap(room_t *this);

static void drawCameras(room_t *this);

static void drawDoors(room_t *this);
/*static void room_map_drawObstacles(room_t *this);*/
static void drawItems(room_t *this);

static void drawPlayer(player_t *this);

static void drawOrigin(void);
static void drawGrid(void);

static void drawCollisions(room_t *this);

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

static void setProjection2D(room_t *this)
{
	player_t *player=game->player;

	render.set_ortho(-20000.0f, 20000.0f, -20000.0f, 20000.0f, -20000.0f, 20000.0f);

	render.set_identity();
	render.rotate(270.0f, 1.0f,0.0f,0.0f);
	render.translate(-player->x, 0.0f, -player->z);
}

static void setProjection3D(room_t *this)
{
	player_t *player=game->player;
	room_camera_t room_camera;

	this->getCamera(this, game->num_camera, &room_camera);

	render.set_projection(60.0f, 4.0f/3.0f, RENDER_Z_NEAR, RENDER_Z_FAR);

	render.set_modelview(
		room_camera.from_x, room_camera.from_y, room_camera.from_z,
		room_camera.to_x, room_camera.to_y, room_camera.to_z,
		0.0f, -1.0f, 0.0f
	);
	render.scale(1.0f, -1.0f, 1.0f);
	/*render.translate(0.0f, player->y+2000.0f, 0.0f);*/
}

static void initMatrix(room_t *this, float mtx_proj[4][4], float mtx_model[4][4])
{
	render.get_proj_matrix(mtx_proj);
	render.get_model_matrix(mtx_model);

	logMsg(1, "map: init: projection\n");
	mtx_print(mtx_proj);
	logMsg(1, "map: init: modelview\n");
	mtx_print(mtx_model);
}

static void calcMatrix(Uint32 elapsed)
{
	float pos = (float) elapsed / (float) MAP_TRANSITION_TIME;
	int i,j;

	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			mtx_proj_cur[i][j]=mtx_proj_from[i][j]+
				((mtx_proj_to[i][j]-mtx_proj_from[i][j])*pos);
			mtx_model_cur[i][j]=mtx_model_from[i][j]+
				((mtx_model_to[i][j]-mtx_model_from[i][j])*pos);
		}
	}

	render.set_proj_matrix(mtx_proj_cur);
	render.set_model_matrix(mtx_model_cur);

	logMsg(1, "map: calc: projection\n");
	mtx_print(mtx_proj_cur);
	logMsg(1, "map: calc: modelview\n");
	mtx_print(mtx_model_cur);
}

static void drawMap(room_t *this)
{
	player_t *player=game->player;
	Uint32 elapsed;
	static int first_print=1;
/*	float angle;

	angle = (clockGet() & 2047) * 360.0f / 2048.0f;
	angle = 270.0f;*/

	switch(this->map_mode) {
		case ROOM_MAP_2D:
			{
				setProjection2D(this);

				if (first_print) {
					render.get_proj_matrix(mtx_proj_cur);
					render.get_model_matrix(mtx_model_cur);

					logMsg(1, "map: 2d: projection\n");
					mtx_print(mtx_proj_cur);
					logMsg(1, "map: 2d: modelview\n");
					mtx_print(mtx_model_cur);
					
					first_print=0;
				}

				render.set_depth(0);
			}
			break;
		case ROOM_MAP_2D_TO_3D_INIT:
			{
				clock_start = clockGet();
				this->map_mode = ROOM_MAP_2D_TO_3D_CALC;
				setProjection2D(this);
				initMatrix(this, mtx_proj_from, mtx_model_from);
				setProjection3D(this);
				initMatrix(this, mtx_proj_to, mtx_model_to);
				logMsg(1, "map: ROOM_MAP_2D_TO_3D_CALC\n");
			}
			break;
		case ROOM_MAP_2D_TO_3D_CALC:
			{
				elapsed = clockGet() - clock_start;
				if (elapsed > MAP_TRANSITION_TIME) {
					this->map_mode = ROOM_MAP_3D;
					logMsg(1, "map: ROOM_MAP_3D\n");

					first_print=1;
				} else {
					calcMatrix(elapsed);
				}
			}
			break;
		case ROOM_MAP_3D:
			{
				setProjection3D(this);

				if (first_print) {
					render.get_proj_matrix(mtx_proj_cur);
					render.get_model_matrix(mtx_model_cur);

					logMsg(1, "map: 2d: projection\n");
					mtx_print(mtx_proj_cur);
					logMsg(1, "map: 2d: modelview\n");
					mtx_print(mtx_model_cur);
					
					first_print=0;
				}
			}
			break;		
		case ROOM_MAP_3D_TO_2D_INIT:
			{
				clock_start = clockGet();
				this->map_mode = ROOM_MAP_3D_TO_2D_CALC;
				setProjection3D(this);
				initMatrix(this, mtx_proj_from, mtx_model_from);
				setProjection2D(this);
				initMatrix(this, mtx_proj_to, mtx_model_to);
				logMsg(1, "map: ROOM_MAP_3D_TO_2D_CALC\n");
			}
			break;
		case ROOM_MAP_3D_TO_2D_CALC:
			{
				elapsed = clockGet() - clock_start;
				if (elapsed > MAP_TRANSITION_TIME) {
					this->map_mode = ROOM_MAP_2D;
					logMsg(1, "map: ROOM_MAP_2D\n");

					first_print=1;
				} else {
					calcMatrix(elapsed);
				}
			}
			break;
	}

	render.set_texture(0, NULL);

	render.set_color(MAP_COLOR_GRID);
	drawGrid();

	render.set_color(MAP_COLOR_BOUNDARY);
	this->drawBoundaries(this);

	render.set_color(MAP_COLOR_CAMSWITCH);
	this->drawCamSwitches(this);

	drawOrigin();

	switch(this->map_mode) {
		case ROOM_MAP_2D:
		case ROOM_MAP_3D_TO_2D_INIT:
		case ROOM_MAP_3D_TO_2D_CALC:
			drawCameras(this);
			/*drawObstacles(this);*/
			drawCollisions(this);
			drawItems(this);
			drawDoors(this);
			drawPlayer(player);
			break;
		case ROOM_MAP_3D:
		case ROOM_MAP_2D_TO_3D_INIT:
		case ROOM_MAP_2D_TO_3D_CALC:
			/*drawObstacles(this);*/
			drawCollisions(this);
			drawItems(this);
			drawDoors(this);
			break;
		default:
			break;
	}

	render.set_depth(1);
}

static void drawCameras(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_CAMERA);

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

		v[0].x = room_camera.from_x;
		v[0].y = 0.0f;
		v[0].z = room_camera.from_z;

		v[1].x = room_camera.from_x + radius * cos(((angle+30.0f)*M_PI)/180.0f);
		v[1].y = 0.0f;
		v[1].z = room_camera.from_z + radius * sin(((angle+30.0f)*M_PI)/180.0f);

		render.line(&v[0], &v[1]);

		v[1].x = room_camera.from_x + radius * cos(((angle-30.0f)*M_PI)/180.0f);
		v[1].y = 0.0f;
		v[1].z = room_camera.from_z + radius * sin(((angle-30.0f)*M_PI)/180.0f);

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

		v[0].x = door->x;
		v[0].y = 0.0f;
		v[0].z = door->y;

		v[1].x = door->x+door->w;
		v[1].y = 0.0f;
		v[1].z = door->y;

		v[2].x = door->x+door->w;
		v[2].y = 0.0f;
		v[2].z = door->y+door->h;

		v[3].x = door->x;
		v[3].y = 0.0f;
		v[3].z = door->y+door->h;

		render.quad_wf(&v[3], &v[2], &v[1], &v[0]);
	}
}

static void drawItems(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_ITEM);

	for (i=0; i<this->num_items; i++) {
		room_item_t *item = &this->items[i];
		vertex_t v[4];

		v[0].x = item->x;
		v[0].y = 0.0f;
		v[0].z = item->y;

		v[1].x = item->x+item->w;
		v[1].y = 0.0f;
		v[1].z = item->y;

		v[2].x = item->x+item->w;
		v[2].y = 0.0f;
		v[2].z = item->y+item->h;

		v[3].x = item->x;
		v[3].y = 0.0f;
		v[3].z = item->y+item->h;

		render.quad_wf(&v[3], &v[2], &v[1], &v[0]);
	}
}

void drawPlayer(player_t *player)
{
	vertex_t v[2];
	const float radius = 2000.0f;
	float x = player->x;
	float y = player->z;
	float angle = player->a;

	render.set_color(MAP_COLOR_PLAYER);

	v[0].x = (x - radius * cos((-angle * M_PI) / 2048.0f) * 0.5f);
	v[0].y = 0.0f;
	v[0].z = (y - radius * sin((-angle * M_PI) / 2048.0f) * 0.5f);

	v[1].x = (x + radius * cos((-angle * M_PI) / 2048.0f) * 0.5f);
	v[1].y = 0.0f;
	v[1].z = (y + radius * sin((-angle * M_PI) / 2048.0f) * 0.5f);

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);
	v[0].y = 0.0f;
	v[0].z = (y + radius * sin((-(angle-224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);
	v[0].y = 0.0f;
	v[0].z = (y + radius * sin((-(angle+224.0f) * M_PI) / 2048.0f) * 0.5f * 0.80f);

	render.line(&v[0], &v[1]);
}

static void drawOrigin(void)
{
	vertex_t v[2];

	v[0].x = v[0].y = v[0].z = 0;
	v[1].x = 3000; v[1].y = v[1].z = 0;

	render.set_color(0x00ff0000); /* x red */
	render.line(&v[0], &v[1]);

	v[1].y = 3000; v[1].x = v[1].z = 0;
	render.set_color(0x0000ff00);	/* y green */
	render.line(&v[0], &v[1]);

	v[1].z = 3000; v[1].x = v[1].y = 0;
	render.set_color(0x000000ff);	/* z blue */
	render.line(&v[0], &v[1]);
}

static void drawGrid(void)
{
	float i;
	float px, pz;
	vertex_t v[2];
	player_t *player = game->player;

	px = ((int) (player->x / 2000)) * 2000.0f;
	pz = ((int) (player->z / 2000)) * 2000.0f;
/*	logMsg(1, "map: %f,%f\n",px,pz);*/

	/*render.push_matrix();*/
	/*render.translate(px, 0.0f, pz);*/

	for (i=-20000.0f; i<=20000.0f; i+=2000.0f) {
		v[0].x = px-20000.0f;
		v[0].y = 0.0f;
		v[0].z = pz+i;

		v[1].x = px+20000.0f;
		v[1].y = 0.0f;
		v[1].z = pz+i;

		render.line(&v[0], &v[1]);

		v[0].x = px+i;
		v[0].y = 0.0f;
		v[0].z = pz-20000.0f;

		v[1].x = px+i;
		v[1].y = 0.0f;
		v[1].z = pz+20000.0f;

		render.line(&v[0], &v[1]);
	}

	/*render.pop_matrix();*/
}

static void drawCollisions(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_COLLISION);

	for (i=0; i<this->getNumCollisions(this); i++) {
		this->drawMapCollision(this, i);
	}
}

/*
	3d gl

[    5.533] map: 2d: projection
(   1.299          0.000           0.000           0.000)
(   0.000          1.732           0.000           0.000)
(   0.000          0.000          -1.000         -32.008)
(   0.000          0.000          -1.000           0.000)
[    5.534] map: 2d: modelview
(   0.975          0.000          -0.222        -4315.459)
(   0.062         -0.960           0.274        -4495.813)
(  -0.213         -0.281          -0.936        -8811.254)
(   0.000          0.000           0.000           1.000)	

	2d gl

[   66.128] map: 2d: projection
(   0.000          0.000           0.000          -0.000)
(   0.000          0.000           0.000          -0.000)
(   0.000          0.000           0.000          -0.000)
(   0.000          0.000           0.000           1.000)
[   66.128] map: 2d: modelview
(   1.000          0.000           0.000        -6246.000)
(   0.000         -0.000           1.000        -7992.000)
(   0.000         -1.000          -0.000           0.000)
(   0.000          0.000           0.000           1.000)

	3d soft

[    8.393] map: 2d: projection
( 473.515         89.808         207.168        813752.375)
(  25.245        466.341         110.853        2883009.250)
(   0.213          0.281           0.936        8120.893)
(   0.213          0.281           0.936        8148.921)
[    8.394] map: 2d: modelview
(   1.000          0.000           0.000           0.000)
(   0.000          1.000           0.000        2360.000)
(   0.000          0.000           1.000           0.000)
(   0.000          0.000           0.000           1.000)

	2d soft, nok

[   53.668] map: 2d: projection
(   0.016          0.000           0.000         320.000)
(   0.000         -0.012           0.000         240.000)
(   0.000          0.000           0.000           0.000)
(   0.000          0.000           0.000           1.000)
[   53.668] map: 2d: modelview
(   1.000          0.000           0.000        -6246.000)
(   0.000         -0.000           1.000        -7992.000)
(   0.000         -1.000          -0.000           0.000)
(   0.000          0.000           0.000           1.000)

*/
