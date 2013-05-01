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

#include "../parameters.h"
#include "../log.h"

#include "game.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

game_t game;

/*--- Variables ---*/

static const char *game_version="Unknown version";

/*--- Functions prototypes ---*/

static void game_shutdown(game_t *this);

static void game_load_font(void);
static void game_get_char(int ascii, int *x, int *y, int *w, int *h);

/*--- Functions ---*/

void game_init(game_t *this)
{
	logMsg(2, "game: init\n");

	memset(this, 0, sizeof(game_t));

	this->shutdown = game_shutdown;

	this->load_font = game_load_font;
	this->get_char = game_get_char;

	this->major = GAME_UNKNOWN;
	this->name = game_version;

	this->num_stage = params.stage;
	this->num_room = params.room;
	this->num_camera = params.camera;

	player_init(&this->player);
	room_init(&this->room);
}

void game_shutdown(game_t *this)
{
	logMsg(2, "game: shutdown\n");

	this->player.shutdown(&this->player);
	this->room.shutdown(&this->room);
}

int game_file_exists(const char *filename)
{
	char *filenamedir;
	int detected = 0;

	filenamedir = malloc(strlen(params.basedir)+strlen(filename)+4);
	if (filenamedir) {
		PHYSFS_file	*curfile;

		sprintf(filenamedir, "%s/%s", params.basedir, filename);

		logMsg(2, "fs: Checking %s file\n", filename);

		curfile = PHYSFS_openRead(filename);
		if (curfile) {
			char dummy;

			if (PHYSFS_read(curfile, &dummy, 1, 1)>0) {
				detected = 1;
			}

			PHYSFS_close(curfile);
		}

		/* Try in upper case */
		if (!detected) {
			int i;

			for (i=0; i<strlen(filenamedir); i++) {
				filenamedir[i] = toupper(filenamedir[i]);
			}

			curfile = PHYSFS_openRead(filename);
			if (curfile) {
				char dummy;

				if (PHYSFS_read(curfile, &dummy, 1, 1)>0) {
					detected = 1;
				}

				PHYSFS_close(curfile);
			}
		}

		free(filenamedir);
	}

	return detected;
}

static void game_load_font(void)
{
}

static void game_get_char(int ascii, int *x, int *y, int *w, int *h)
{
}

