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

#include "../log.h"

#include "room.h"
#include "game.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void room_shutdown(room_t *this);

static void room_load(void);
static void room_loadbackground(void);
static void room_loadbgmask(void);

static void room_unload(void);
static void room_unloadbackground(void);
static void room_unloadbgmask(void);

static void room_priv_shutdown(void);
static void room_priv_load(void);
static void room_priv_load_background(void);
static void room_priv_load_bgmask(void);

/*--- Functions ---*/

void room_init(room_t *this)
{
	logMsg(2, "room: init\n");

	memset(this, 0, sizeof(room_t));

	this->shutdown = room_shutdown;

	this->load = room_load;
	this->load_background = room_loadbackground;
	this->load_bgmask = room_loadbgmask;

	this->priv_shutdown = room_priv_shutdown;
	this->priv_load = room_priv_load;
	this->priv_load_background = room_priv_load_background;
	this->priv_load_bgmask = room_priv_load_bgmask;

	room_map_init(&this->room_map);
}

static void room_shutdown(room_t *this)
{
	logMsg(2, "room: shutdown\n");

	room_unloadbackground();
	room_unloadbgmask();

	room_unload();

	this->priv_shutdown();

	this->room_map.shutdown(&this->room_map);
}

static void room_load(void)
{
	room_unload();

	game.room.priv_load();
}

static void room_loadbackground(void)
{
	room_unloadbackground();

	game.room.priv_load_background();
}

static void room_loadbgmask(void)
{
	room_unloadbgmask();

	game.room.priv_load_bgmask();
}

static void room_unload(void)
{
	room_t *this = &game.room;

	logMsg(2, "room: unload\n");

	if (this->file) {
		free(this->file);
		this->file=NULL;
	}
}

static void room_unloadbackground(void)
{
	room_t *this = &game.room;

	logMsg(2, "room: unloadbackground\n");

	if (this->background) {
		free(this->background);
		this->background=NULL;
	}
}

static void room_unloadbgmask(void)
{
	room_t *this = &game.room;

	logMsg(2, "room: unloadbgmask\n");

	if (this->bg_mask) {
		free(this->bg_mask);
		this->bg_mask=NULL;
	}
	if (this->rdr_mask) {
		free(this->rdr_mask);
		this->rdr_mask=NULL;
	}
}

static void room_priv_shutdown(void)
{
}

static void room_priv_load(void)
{
}

static void room_priv_load_background(void)
{
}

static void room_priv_load_bgmask(void)
{
}
