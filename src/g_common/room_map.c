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

#include <string.h>

#include "../log.h"

#include "room_map.h"

/*--- Functions prototypes ---*/

static void room_map_shutdown(room_map_t *this);

/*--- Functions ---*/

void room_map_init(room_map_t *this)
{
	logMsg(2, "room_map: init\n");

	memset(this, 0, sizeof(room_map_t));

	this->shutdown = room_map_shutdown;
}

static void room_map_shutdown(room_map_t *this)
{
	logMsg(2, "room_map: shutdown\n");
}
