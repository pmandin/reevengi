/*
	Room door

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

#ifndef ROOM_DOOR_H
#define ROOM_DOOR_H 1

/*--- Types ---*/

typedef struct room_door_s room_door_t;

struct room_door_s {
	Sint16 x,y,w,h;

	Sint16 next_x,next_y,next_z,next_dir;
	Uint8 next_stage,next_room,next_camera;
};

/*--- Functions ---*/

void room_door_init(room_t *this);

#endif /* ROOM_DOOR_H */
