/*
	Room data

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

#include "game.h"
#include "room.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void room_shutdown(room_t *this);
static void room_load(room_t *this);
static void room_loadbackground(room_t *this);
static void room_loadbgmask(room_t *this);

static void room_unload(room_t *this);
static void room_unloadbackground(room_t *this);
static void room_unloadbgmask(room_t *this);

/*--- Functions ---*/

void room_init(void)
{
	memset(&game.room, 0, sizeof(room_t));

	game.room.init = room_init;
	game.room.shutdown = room_shutdown;

	game.room.load = room_load;
	game.room.load_background = room_loadbackground;
	game.room.load_bgmask = room_loadbgmask;
}

static void room_shutdown(room_t *this)
{
	room_unloadbackground(this);
	room_unloadbgmask(this);

	room_unload(this);
}

static void room_load(room_t *this)
{
	room_unload(this);
}

static void room_loadbackground(room_t *this)
{
	room_unloadbackground(this);
}

static void room_loadbgmask(room_t *this)
{
	room_unloadbgmask(this);
}

static void room_unload(room_t *this)
{
	if (this->file) {
		free(this->file);
		this->file=NULL;
	}
}

static void room_unloadbackground(room_t *this)
{
	if (this->background) {
		free(this->background);
		this->background=NULL;
	}
}

static void room_unloadbgmask(room_t *this)
{
	if (this->bg_mask) {
		free(this->bg_mask);
		this->bg_mask=NULL;
	}
	if (this->rdr_mask) {
		free(this->rdr_mask);
		this->rdr_mask=NULL;
	}
}
