/*
	Game state

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
#include <physfs.h>
#include <SDL.h>
#include <assert.h>

#include "../parameters.h"
#include "../log.h"

#include "room.h"
#include "room_map.h"
#include "player.h"
#include "menu.h"
#include "game.h"
#include "fs_ignorecase.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/

static const char *game_version="Unknown version";

/*--- Functions prototypes ---*/

static void dtor(game_t *this);

static void load_font(game_t *this);
static void get_char(game_t *this, int ascii, int *x, int *y, int *w, int *h);

static void setRoom(game_t *this, int num_stage, int num_room);
static room_t *game_room_ctor(game_t *this, int num_stage, int num_room);

static void prev_stage(game_t *this);
static void next_stage(game_t *this);
static void reset_stage(game_t *this);

static void prev_room(game_t *this);
static void next_room(game_t *this);
static void reset_room(game_t *this);

static void prev_camera(game_t *this);
static void next_camera(game_t *this);
static void reset_camera(game_t *this);

static void prev_movie(game_t *this);
static void next_movie(game_t *this);
static void reset_movie(game_t *this);
static void switch_movie(game_t *this);

static int getnummovies(game_t *this);

/*--- Functions ---*/

game_t *game_ctor(void)
{
	game_t *this;

	logMsg(2, "game: ctor\n");

	this = (game_t *) calloc(1, sizeof(game_t));
	if (!this) {
		return NULL;
	}

	this->dtor = dtor;

	this->load_font = load_font;
	this->get_char = get_char;

	this->major = GAME_UNKNOWN;
	this->name = game_version;

	this->num_stage = params.stage;
	this->num_room = params.room;
	this->num_camera = params.camera;

	this->prev_stage = prev_stage;
	this->next_stage = next_stage;
	this->reset_stage = reset_stage;

	this->prev_room = prev_room;
	this->next_room = next_room;
	this->reset_room = reset_room;

	this->prev_camera = prev_camera;
	this->next_camera = next_camera;
	this->reset_camera = reset_camera;

	this->prev_movie = prev_movie;
	this->next_movie = next_movie;
	this->reset_movie = reset_movie;
	this->switch_movie = switch_movie;

	this->player = player_ctor();
	/*this->room = room_ctor();*/
	this->menu = menu_ctor();

	this->setRoom = setRoom;
	this->room_ctor = game_room_ctor;

	return this;
}

static void dtor(game_t *this)
{
	logMsg(2, "game: dtor\n");

	if (this->player) {
		this->player->dtor(this->player);
		this->player=NULL;
	}

	if (this->room) {
		this->room->dtor(this->room);
		this->room=NULL;
	}

	if (this->menu) {
		this->menu->dtor(this->menu);
		this->menu=NULL;
	}
}

int game_file_exists(const char *filename)
{
	char *filename2;

	filename2 = strdup(filename);
	logMsg(2, "fs: Checking %s file\n", filename);

	return (PHYSFSEXT_locateCorrectCase(filename2) == 0);
}

static void load_font(game_t *this)
{
}

static void get_char(game_t *this, int ascii, int *x, int *y, int *w, int *h)
{
}

static room_t *game_room_ctor(game_t *this, int num_stage, int num_room)
{
	return NULL;
}

static void setRoom(game_t *this, int new_stage, int new_room)
{
	room_t *room;

	if (this->room) {
		this->room->dtor(this->room);
		this->room = NULL;
	}

	room = game->room_ctor(game, new_stage, new_room);
	if (!room) {
		return;
	}

	room->loadFile(room);
	if (!room->file) {
		room->dtor(room);
		return;
	}

	room->postLoad(room);

	room_map_init_data(room);

	room->num_cameras = room->getNumCameras(room);

	logMsg(1, "room: %d cameras angles, %d camera switches, %d boundaries\n",
		room->num_cameras, room->getNumCamSwitches(room), room->getNumBoundaries(room));

	room->scriptExec(room, ROOM_SCRIPT_INIT);
	room->scriptExec(room, ROOM_SCRIPT_RUN);

	this->room = room;
}

static void prev_stage(game_t *this)
{
	--this->num_stage;
	if (this->num_stage < 1) {
		this->num_stage = 7;
	}
}

static void next_stage(game_t *this)
{
	++this->num_stage;
	if (this->num_stage > 7) {
		this->num_stage = 1;
	}
}

static void reset_stage(game_t *this)
{
	this->num_stage = 1;
}

static void prev_room(game_t *this)
{
	--this->num_room;
	if (this->num_room < 0) {
		this->num_room = 0x1c;
	}
}

static void next_room(game_t *this)
{
	++this->num_room;
	if (this->num_room > 0x1c) {
		this->num_room = 0;
	}
}

static void reset_room(game_t *this)
{
	this->num_room = 0;
}

static void prev_camera(game_t *this)
{
	assert(this->room);

	--this->num_camera;
	if (this->num_camera<0) {
		if (this->room->num_cameras>0) {
			this->num_camera = this->room->num_cameras-1;
		} else {
			this->num_camera = 0;
		}
	}
}

static void next_camera(game_t *this)
{
	assert(this->room);

	++this->num_camera;
	if (this->num_camera>=this->room->num_cameras) {
		this->num_camera = 0;
	}
}

static void reset_camera(game_t *this)
{
	this->num_camera = 0;
}

static void prev_movie(game_t *this)
{
	--this->num_movie;
	if (this->num_movie<0) {
		this->num_movie=0;
	}

	this->switch_movie(this);
}

static void next_movie(game_t *this)
{
	int max_num_movies = getnummovies(this);

	++this->num_movie;
	if (this->num_movie>=max_num_movies) {
		this->num_movie = max_num_movies-1;
	}

	this->switch_movie(this);
}

static void reset_movie(game_t *this)
{
	this->num_movie = 0;

	this->switch_movie(this);
}

static int getnummovies(game_t *this)
{
	char **movie = this->movies_list;
	int i;

	for (i=0; movie[i]; i++) {
	}
	return i;
}

static void switch_movie(game_t *this)
{
	char **movie = this->movies_list;
	int i;

	for (i=0; movie[i]; i++) {
		if (i==this->num_movie) {
			/*sprintf(game_t *this->cur_movie, "%s/%s", params.basedir, movie[i]);*/
			this->cur_movie = movie[i];
			break;
		}
	}
}
