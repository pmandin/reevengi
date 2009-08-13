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
#include "parameters.h"

/*--- Defines ---*/

#define MAP_COLOR_CAMERA_DISABLED	0x00666666
#define MAP_COLOR_CAMERA_ENABLED	0x00ffffff
#define MAP_COLOR_CAMSWITCH_DISABLED	0x00664433
#define MAP_COLOR_CAMSWITCH_ENABLED	0x00ffcc88
#define MAP_COLOR_BOUNDARY_DISABLED	0x00660000
#define MAP_COLOR_BOUNDARY_ENABLED	0x00ff0000
#define MAP_COLOR_DOOR			0x0000cccc
#define MAP_COLOR_OBSTACLE		0x00ffcc00
#define MAP_COLOR_ITEM			0x00ffff00
#define MAP_COLOR_WALLS			0x00ff00ff
#define MAP_COLOR_PLAYER		0x0000ff00

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

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
static void room_map_drawDoors(room_t *this);
static void room_map_drawObstacles(room_t *this);
static void room_map_drawItems(room_t *this);

static void addDoor(room_t *this, room_door_t *door);
static room_door_t *enterDoor(room_t *this, Sint16 x, Sint16 y);

static void addObstacle(room_t *this, room_obstacle_t *obstacle);

static void addItem(room_t *this, room_item_t *obstacle);

static Uint8 *scriptPrivFirstInst(room_t *this);
static int scriptPrivGetInstLen(room_t *this);
static void scriptPrivExecInst(room_t *this);
static void scriptPrivPrintInst(room_t *this);

static Uint8 *scriptNextInst(room_t *this);
static void scriptDump(room_t *this);
static void scriptExec(room_t *this);

/*--- Functions ---*/

room_t *room_create(void *room_file, Uint32 length)
{
	room_t *this = calloc(1, sizeof(room_t));
	if (!this) {
		return NULL;
	}

	this->file = room_file;
	this->file_length = length;

	this->shutdown = shutdown;

	this->addDoor = addDoor;
	this->enterDoor = enterDoor;

	this->addObstacle = addObstacle;

	this->addItem = addItem;

	this->scriptPrivFirstInst = scriptPrivFirstInst;
	this->scriptPrivGetInstLen = scriptPrivGetInstLen;
	this->scriptPrivExecInst = scriptPrivExecInst;
	this->scriptPrivPrintInst = scriptPrivPrintInst;

	this->scriptDump = scriptDump;
	this->scriptExec = scriptExec;

	return this;
}

static void shutdown(room_t *this)
{
	if (this) {
		if (this->doors) {
			free(this->doors);
			this->doors = NULL;
		}
		if (this->obstacles) {
			free(this->obstacles);
			this->obstacles = NULL;
		}
		if (this->items) {
			free(this->items);
			this->items = NULL;
		}
		if (this->file) {
			free(this->file);
			this->file = NULL;
		}
		free(this);
	}
}

/* Map functions */

void room_map_init(room_t *this)
{
	int range, v;

	minx = minz = 32767;
	maxx = maxz = -32768;

	room_map_minMaxCameras(this);
	room_map_minMaxCamswitches(this);
	room_map_minMaxBoundaries(this);

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

static void room_map_minMaxCameras(room_t *this)
{
	int i;

	for (i=0; i<this->num_cameras; i++) {
		room_camera_t room_camera;
		Sint16 v;

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

static void room_map_minMaxCamswitches(room_t *this)
{
	int i, j;

	for (i=0; i<this->num_camswitches; i++) {
		room_camswitch_t room_camswitch;

		this->getCamswitch(this, i, &room_camswitch);

		for (j=0; j<4; j++) {
			minx = MIN(room_camswitch.x[j], minx);
			maxx = MAX(room_camswitch.x[j], maxx);

			minz = MIN(room_camswitch.y[j], minz);
			maxz = MAX(room_camswitch.y[j], maxz);
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
			minx = MIN(room_camswitch.x[j], minx);
			maxx = MAX(room_camswitch.x[j], maxx);

			minz = MIN(room_camswitch.y[j], minz);
			maxz = MAX(room_camswitch.y[j], maxz);
		}
	}
}

void room_map_draw(room_t *this)
{
	render.set_texture(0, NULL);
	render.push_matrix();

	/*printf("draw map %d,%d %d,%d\n",minx,maxx,minz,maxz);*/

	/* Set ortho projection */
	render.set_ortho(minx*0.5f,maxx*0.5f, minz*0.5f,maxz*0.5f, -1.0f,1.0f);
	/*render.set_identity();*/

	room_map_drawBoundaries(this);
	room_map_drawCamswitches(this);
	room_map_drawCameras(this);

	room_map_drawObstacles(this);
	room_map_drawItems(this);
	room_map_drawDoors(this);

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
	}
}

static void room_map_drawCamswitches(room_t *this)
{
	int i, j, prev_from=-1;

	for (i=0; i<this->num_camswitches; i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[4];
		int boundary = 0;

		this->getCamswitch(this, i, &room_camswitch);

		render.set_color((room_camswitch.from==game_state.num_camera) ?
			MAP_COLOR_CAMSWITCH_ENABLED :
			MAP_COLOR_CAMSWITCH_DISABLED);

		for (j=0; j<4; j++) {
			v[j].x = room_camswitch.x[j] * 0.5f;
			v[j].y = room_camswitch.y[j] * 0.5f;
			v[j].z = 1.0f;
		}

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
	}
}

static void room_map_drawBoundaries(room_t *this)
{
	int i, j, prev_from=-1;

	for (i=0; i<this->num_boundaries; i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[4];
		int boundary = 0;

		this->getBoundary(this, i, &room_camswitch);

		render.set_color((room_camswitch.from==game_state.num_camera) ?
			MAP_COLOR_BOUNDARY_ENABLED :
			MAP_COLOR_BOUNDARY_DISABLED);

		for (j=0; j<4; j++) {
			v[j].x = room_camswitch.x[j] * 0.5f;
			v[j].y = room_camswitch.y[j] * 0.5f;
			v[j].z = 1.0f;
		}

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
	}
}

static void room_map_drawDoors(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_DOOR);

	for (i=0; i<this->num_doors; i++) {
		room_door_t	*door = &this->doors[i];
		vertex_t v[4];

#if 1
		v[0].x = door->x * 0.5f;
		v[0].y = door->y * 0.5f;
		v[0].z = 1;

		v[1].x = (door->x+door->w) * 0.5f;
		v[1].y = door->y * 0.5f;
		v[1].z = 1;

		render.line(&v[0], &v[1]);

		v[0].x = (door->x+door->w) * 0.5f;
		v[0].y = (door->y+door->h) * 0.5f;
		v[0].z = 1;

		render.line(&v[0], &v[1]);

		v[1].x = door->x * 0.5f;
		v[1].y = (door->y+door->h) * 0.5f;
		v[1].z = 1;

		render.line(&v[0], &v[1]);

		v[0].x = door->x * 0.5f;
		v[0].y = door->y * 0.5f;
		v[0].z = 1;

		render.line(&v[0], &v[1]);
#else
		v[0].x = door->x * 0.5f;
		v[0].y = door->y * 0.5f;
		v[0].z = 1;

		v[1].x = (door->x+door->w) * 0.5f;
		v[1].y = door->y * 0.5f;
		v[1].z = 1;

		v[2].x = (door->x+door->w) * 0.5f;
		v[2].y = (door->y+door->h) * 0.5f;
		v[2].z = 1;

		v[3].x = door->x * 0.5f;
		v[3].y = (door->y+door->h) * 0.5f;
		v[3].z = 1;

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
#endif
	}
}

static void room_map_drawObstacles(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_OBSTACLE);

	for (i=0; i<this->num_obstacles; i++) {
		room_obstacle_t	*obstacle = &this->obstacles[i];
		vertex_t v[4];

#if 1
		v[0].x = obstacle->x * 0.5f;
		v[0].y = obstacle->y * 0.5f;
		v[0].z = 1;

		v[1].x = (obstacle->x+obstacle->w) * 0.5f;
		v[1].y = obstacle->y * 0.5f;
		v[1].z = 1;

		render.line(&v[0], &v[1]);

		v[0].x = (obstacle->x+obstacle->w) * 0.5f;
		v[0].y = (obstacle->y+obstacle->h) * 0.5f;
		v[0].z = 1;

		render.line(&v[0], &v[1]);

		v[1].x = obstacle->x * 0.5f;
		v[1].y = (obstacle->y+obstacle->h) * 0.5f;
		v[1].z = 1;

		render.line(&v[0], &v[1]);

		v[0].x = obstacle->x * 0.5f;
		v[0].y = obstacle->y * 0.5f;
		v[0].z = 1;

		render.line(&v[0], &v[1]);
#else
		v[0].x = obstacle->x * 0.5f;
		v[0].y = obstacle->y * 0.5f;
		v[0].z = 1;

		v[1].x = (obstacle->x+obstacle->w) * 0.5f;
		v[1].y = obstacle->y * 0.5f;
		v[1].z = 1;

		v[2].x = (obstacle->x+obstacle->w) * 0.5f;
		v[2].y = (obstacle->y+obstacle->h) * 0.5f;
		v[2].z = 1;

		v[3].x = obstacle->x * 0.5f;
		v[3].y = (obstacle->y+obstacle->h) * 0.5f;
		v[3].z = 1;

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
#endif
	}
}

static void room_map_drawItems(room_t *this)
{
	int i;

	render.set_color(MAP_COLOR_ITEM);

	for (i=0; i<this->num_items; i++) {
		room_item_t	*item = &this->items[i];
		vertex_t v[4];

#if 1
		v[0].x = item->x * 0.5f;
		v[0].y = item->y * 0.5f;
		v[0].z = 1;

		v[1].x = (item->x+item->w) * 0.5f;
		v[1].y = item->y * 0.5f;
		v[1].z = 1;

		render.line(&v[0], &v[1]);

		v[0].x = (item->x+item->w) * 0.5f;
		v[0].y = (item->y+item->h) * 0.5f;
		v[0].z = 1;

		render.line(&v[0], &v[1]);

		v[1].x = item->x * 0.5f;
		v[1].y = (item->y+item->h) * 0.5f;
		v[1].z = 1;

		render.line(&v[0], &v[1]);

		v[0].x = item->x * 0.5f;
		v[0].y = item->y * 0.5f;
		v[0].z = 1;

		render.line(&v[0], &v[1]);
#else
		v[0].x = item->x * 0.5f;
		v[0].y = item->y * 0.5f;
		v[0].z = 1;

		v[1].x = (item->x+item->w) * 0.5f;
		v[1].y = item->y * 0.5f;
		v[1].z = 1;

		v[2].x = (item->x+item->w) * 0.5f;
		v[2].y = (item->y+item->h) * 0.5f;
		v[2].z = 1;

		v[3].x = item->x * 0.5f;
		v[3].y = (item->y+item->h) * 0.5f;
		v[3].z = 1;

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
#endif
	}
}

void room_map_drawPlayer(float x, float y, float angle)
{
	vertex_t v[2];
	const float radius = 2000.0f;

	render.set_texture(0, NULL);
	render.push_matrix();

	/* Set ortho projection */
	render.set_ortho(minx * 0.5f,maxx * 0.5f, minz * 0.5f,maxz * 0.5f, -1.0f,1.0f);

	render.set_color(MAP_COLOR_PLAYER);

	v[0].x = (x - radius * cos((-angle * M_PI) / 180.0f) * 0.5f) * 0.5f;
	v[0].y = (y - radius * sin((-angle * M_PI) / 180.0f) * 0.5f) * 0.5f;
	v[0].z = 1.0f;

	v[1].x = (x + radius * cos((-angle * M_PI) / 180.0f) * 0.5f) * 0.5f;
	v[1].y = (y + radius * sin((-angle * M_PI) / 180.0f) * 0.5f) * 0.5f;
	v[1].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle-20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].y = (y + radius * sin((-(angle-20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);

	v[0].x = (x + radius * cos((-(angle+20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].y = (y + radius * sin((-(angle+20.0f) * M_PI) / 180.0f) * 0.5f * 0.80f) * 0.5f;
	v[0].z = 1.0f;

	render.line(&v[0], &v[1]);

	render.pop_matrix();
}

int room_checkBoundary(room_t *this, int num_camera, float x, float y)
{
	int i,j;

	if (!this) {
		return 0;
	}

	for (i=0; i<this->num_boundaries; i++) {
		room_camswitch_t room_camswitch;
		int is_inside = 1;

		this->getBoundary(this, i, &room_camswitch);

		if (room_camswitch.from != num_camera) {
			continue;
		}

		for (j=0; j<4; j++) {
			float dx1,dy1,dx2,dy2;

			dx1 = room_camswitch.x[(j+1) & 3] - room_camswitch.x[j];
			dy1 = room_camswitch.y[(j+1) & 3] - room_camswitch.y[j];

			dx2 = x - room_camswitch.x[j];
			dy2 = y - room_camswitch.y[j];

			if (dx1*dy2-dy1*dx2 >= 0) {
				is_inside = 0;
			}
		}

		if (!is_inside) {
			return 1;
		}
	}

	return 0;
}

int room_checkCamswitch(room_t *this, int num_camera, float x, float y)
{
	int i,j;

	if (!this) {
		return -1;
	}

	for (i=0; i<this->num_camswitches; i++) {
		room_camswitch_t room_camswitch;
		int is_inside = 1;

		this->getCamswitch(this, i, &room_camswitch);

		if (room_camswitch.from != num_camera) {
			continue;
		}

		for (j=0; j<4; j++) {
			float dx1,dy1,dx2,dy2;

			dx1 = room_camswitch.x[(j+1) & 3] - room_camswitch.x[j];
			dy1 = room_camswitch.y[(j+1) & 3] - room_camswitch.y[j];

			dx2 = x - room_camswitch.x[j];
			dy2 = y - room_camswitch.y[j];

			if (dx1*dy2-dy1*dx2 >= 0) {
				is_inside = 0;
			}
		}

		if (is_inside) {
			return room_camswitch.to;
		}
	}

	return -1;
}

/* Door functions */

static void addDoor(room_t *this, room_door_t *door)
{
	++this->num_doors;

	this->doors = realloc(this->doors, this->num_doors * sizeof(room_door_t));
	if (!this->doors) {
		logMsg(0, "Can not allocate memory for door\n");
		return;
	}

	memcpy(&this->doors[this->num_doors-1], door, sizeof(room_door_t));
	logMsg(1, "Adding door %d\n", this->num_doors-1);
}

static room_door_t *enterDoor(room_t *this, Sint16 x, Sint16 y)
{
	int i;

	for (i=0; i<this->num_doors; i++) {
		room_door_t *door = &this->doors[i];

		if ((x >= door->x) && (x <= door->x+door->w) &&
		    (y >= door->y) && (y <= door->y+door->h))
		{
			return door;
		}
	}

	return NULL;
}

/* Obstacles functions */

static void addObstacle(room_t *this, room_obstacle_t *obstacle)
{
	++this->num_obstacles;

	this->obstacles = realloc(this->obstacles, this->num_obstacles * sizeof(room_obstacle_t));
	if (!this->obstacles) {
		logMsg(0, "Can not allocate memory for obstacle\n");
		return;
	}

	memcpy(&this->obstacles[this->num_obstacles-1], obstacle, sizeof(room_obstacle_t));
	logMsg(1, "Adding obstacle %d\n", this->num_obstacles-1);
}

/* Items functions */

static void addItem(room_t *this, room_item_t *item)
{
	++this->num_items;

	this->items = realloc(this->items, this->num_items * sizeof(room_item_t));
	if (!this->items) {
		logMsg(0, "Can not allocate memory for obstacle\n");
		return;
	}

	memcpy(&this->items[this->num_items-1], item, sizeof(room_item_t));
	logMsg(1, "Adding item %d\n", this->num_items-1);
}

/* Script functions */

static Uint8 *scriptPrivFirstInst(room_t *this)
{
	return NULL;
}

static int scriptPrivGetInstLen(room_t *this)
{
	return 0;
}

static void scriptPrivExecInst(room_t *this)
{
}

static void scriptPrivPrintInst(room_t *this)
{
}

static Uint8 *scriptNextInst(room_t *this)
{
	int inst_len;
	Uint8 *cur_inst;

	if (!this) {
		return NULL;
	}
	if (!this->cur_inst) {
		return NULL;
	}

	inst_len = this->scriptPrivGetInstLen(this);
	if (inst_len == 0) {
		return NULL;
	}

	this->cur_inst_offset += inst_len;	
	if (this->script_length>0) {
		if (this->cur_inst_offset>= this->script_length) {
			logMsg(1, "End of script reached\n");
			return NULL;
		}
	}

	cur_inst = this->cur_inst;

	this->cur_inst = &cur_inst[inst_len];
	return this->cur_inst;
}

static void scriptDump(room_t *this)
{
	Uint8 *inst;

	inst = this->scriptPrivFirstInst(this);
	while (inst) {
		if (params.verbose>=2) {
			int i, inst_len;

			inst_len = this->scriptPrivGetInstLen(this);
			if (inst_len==0) {
				inst_len = 16;
			}
			logMsg(2, "0x%08x: ", this->cur_inst_offset);
			for (i=0; i<inst_len; i++) {
				logMsg(2, " %02x", this->cur_inst[i]);
			}
			logMsg(2, "\n");
		}

		this->scriptPrivPrintInst(this);
		inst = scriptNextInst(this);
	}
}

static void scriptExec(room_t *this)
{
	Uint8 *inst;

	inst = this->scriptPrivFirstInst(this);
	while (inst) {
		this->scriptPrivExecInst(this);
		inst = scriptNextInst(this);
	}
}
