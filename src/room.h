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
 
#ifndef ROOM_H
#define ROOM_H 1

/*--- Types ---*/

typedef struct {
	Sint32 from_x;	/* Camera is there */
	Sint32 from_y;
	Sint32 from_z;
	Sint32 to_x;	/* looking at this point */
	Sint32 to_y;
	Sint32 to_z;
} room_camera_t;

typedef struct {
	Uint8 from, to;	/* camera to switch from/to, to=0 for boundary */
	Sint16 x[4];	/* Quad zone to check when player enters/exits it */
	Sint16 y[4];
} room_camswitch_t;

typedef struct {
	Sint16 x,y,w,h;

	Sint16 next_x,next_y,next_z,next_dir;
	Uint8 next_stage,next_room,next_camera;
} room_door_t;

typedef struct {
	Sint16 x,y,w,h;
} room_obstacle_t;

typedef struct {
	Sint16 x,y,w,h;
} room_item_t;

typedef struct room_s room_t;

struct room_s {
	void *file;	/* RDT Room data file */
	Uint32 file_length;

	void (*shutdown)(room_t *this);

	/*--- Camera, switches and boundaries ---*/
	int num_cameras;
	int num_camswitches;
	int num_boundaries;

	void (*getCamera)(room_t *this, int num_camera, room_camera_t *room_camera);
	void (*getCamswitch)(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);
	void (*getBoundary)(room_t *this, int num_boundary, room_camswitch_t *room_camswitch);

	/*--- Doors ---*/
	int num_doors;
	room_door_t *doors;

	void (*addDoor)(room_t *this, room_door_t *door);

	/* Return door entered, of NULL if none entered */
	room_door_t *(*enterDoor)(room_t *this, Sint16 x, Sint16 y);

	/*--- Items ---*/
	int num_items;
	room_item_t *items;
	
	void (*addItem)(room_t *this, room_item_t *item);

	/*--- Obstacles ---*/
	int num_obstacles;
	room_obstacle_t *obstacles;
	
	void (*addObstacle)(room_t *this, room_obstacle_t *obstacle);

	/*--- Script execution ---*/
	Uint8 *cur_inst;
	int cur_inst_offset;
	int script_length;

	Uint8 *(*scriptPrivFirstInst)(room_t *this);
	int (*scriptPrivGetInstLen)(room_t *this);
	void (*scriptPrivExecInst)(room_t *this);
	void (*scriptPrivPrintInst)(room_t *this);

	void (*scriptDump)(room_t *this);
	void (*scriptExec)(room_t *this);
};

/*--- Functions ---*/

room_t *room_create(void *room_file, Uint32 length);

void room_map_init(room_t *this);
void room_map_draw(room_t *this);
void room_map_drawPlayer(float x, float y, float angle);

/* Return 1 if player crossed boundary */
int room_checkBoundary(room_t *this, int num_camera, float x, float y);	

/* Return -1 if no cam switch, or new num camera */
int room_checkCamswitch(room_t *this, int num_camera, float x, float y);

#endif /* ROOM_H */
