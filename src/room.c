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
#include <SDL.h>

#include "room.h"

/*--- Functions prototypes ---*/

static void shutdown(room_t *this);

/*--- Functions ---*/

room_t *room_create(void *room_file)
{
	room_t *this = calloc(1, sizeof(room_t));
	if (!this) {
		return NULL;
	}

	this->file = room_file;
	this->shutdown = shutdown;

	return this;
}

static void shutdown(room_t *this)
{
	if (this) {
		if (this->file) {
			free(this->file);
			this->file = NULL;
		}
		free(this);
	}
}
