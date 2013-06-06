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

#ifndef ROOM_MAP_H
#define ROOM_MAP_H 1

/*--- Defines ---*/

enum {
	ROOM_MAP_OFF=0,
	ROOM_MAP_2D,
	ROOM_MAP_2D_TO_3D_INIT,
	ROOM_MAP_2D_TO_3D_CALC,
	ROOM_MAP_3D,
	ROOM_MAP_3D_TO_2D_INIT,
	ROOM_MAP_3D_TO_2D_CALC
};

/*--- Functions ---*/

void room_map_init(room_t *this);

void room_map_init_data(room_t *this);

#endif /* ROOM_MAP_H */
