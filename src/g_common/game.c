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
	this->room = room_ctor();
	this->menu = menu_ctor();

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
	int i, detected = 0;
	PHYSFS_file *curfile;
	char dummy;

	logMsg(2, "fs: Checking %s file\n", filename);

#if 0
	curfile = PHYSFS_openRead(filename);
	if (curfile) {
		if (PHYSFS_read(curfile, &dummy, 1, 1)>0) {
			detected = 1;
		}

		PHYSFS_close(curfile);
	}

	/* Try in upper case */
	if (!detected) {
		filename2 = calloc(1, strlen(filename)+1);
		if (filename2) {
			for (i=0; i<strlen(filename2); i++) {
				filename2[i] = toupper(filename[i]);
			}

			curfile = PHYSFS_openRead(filename);
			if (curfile) {
				if (PHYSFS_read(curfile, &dummy, 1, 1)>0) {
					detected = 1;
				}

				PHYSFS_close(curfile);
			}

			free(filename2);
		}
	}

/*	logMsg(2, "fs:  %s.\n", detected ? "found" : "not found");*/
#else
	filename2 = strdup(filename);

	logMsg(2, "fs: Checking %s file\n", filename);
	detected = (PHYSFSEXT_locateCorrectCase(filename2) == 0);
#endif

	return detected;
}

static void load_font(game_t *this)
{
}

static void get_char(game_t *this, int ascii, int *x, int *y, int *w, int *h)
{
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
