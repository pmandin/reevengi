/*
	Room
	Collision objects

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

#include <string.h>
#include <SDL.h>

#include "../log.h"
#include "../parameters.h"
#include "../filesystem.h"

#include "room.h"
#include "room_collision.h"

/*--- Functions prototypes ---*/

static int getNumCollisions(room_t *this);
static void getCollision(room_t *this, int num_collision, room_collision_t *room_collision);

/*--- Functions ---*/

void room_collision_init(room_t *this)
{
	this->getNumCollisions = getNumCollisions;
	this->getCollision = getCollision;
}

static int getNumCollisions(room_t *this)
{
	return 0;
}

static void getCollision(room_t *this, int num_collision, room_collision_t *room_collision)
{
}
