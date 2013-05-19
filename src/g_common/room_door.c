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

#include <string.h>
#include <SDL.h>

#include "../log.h"
#include "../parameters.h"
#include "../filesystem.h"

#include "room.h"
#include "room_door.h"

/*--- Functions prototypes ---*/

static void addDoor(room_t *this, room_door_t *door);
static room_door_t *enterDoor(room_t *this, Sint16 x, Sint16 y);

/*--- Functions ---*/

void room_door_init(room_t *this)
{
	this->addDoor = addDoor;
	this->enterDoor = enterDoor;
}

static void addDoor(room_t *this, room_door_t *door)
{
	this->doors = realloc(this->doors, (this->num_doors+1) * sizeof(room_door_t));
	if (!this->doors) {
		logMsg(0, "room_door: Can not allocate memory for door\n");
		return;
	}

	memcpy(&this->doors[this->num_doors], door, sizeof(room_door_t));
	logMsg(1, "room_door: Adding door %d\n", this->num_doors);

	++this->num_doors;
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
