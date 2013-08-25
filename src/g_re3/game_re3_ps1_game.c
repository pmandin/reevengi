/*
	RE3
	PS1
	Game

	Copyright (C) 2007-2010	Patrice Mandin

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

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "../filesystem.h"
#include "../parameters.h"
#include "../log.h"
#include "../background_bss.h"

#include "../g_common/room.h"
#include "../g_common/player.h"
#include "../g_common/game.h"

#include "../r_common/render.h"

#include "game_re3.h"
#include "ard.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Constant ---*/

static const char *re3ps1game_bg = "cd_data/stage%d/r%d%02x.bss";
static const char *re3ps1game_room = "cd_data/stage%d/r%d%02x.ard";
static const char *re3ps1game_font = "cd_data/etc/sele_ob%c.tim";

static const char *re3ps1game_movies[] = {
	"cd_data/zmovie/enda.str",
	"cd_data/zmovie/endb.str",
	"cd_data/zmovie/ins01.str",
	"cd_data/zmovie/ins02.str",
	"cd_data/zmovie/ins03.str",
	"cd_data/zmovie/ins04.str",
	"cd_data/zmovie/ins05.str",
	"cd_data/zmovie/ins06.str",
	"cd_data/zmovie/ins07.str",
	"cd_data/zmovie/ins08.str",
	"cd_data/zmovie/ins09.str",
	"cd_data/zmovie/opn.str",
	"cd_data/zmovie/roopne.str",
	NULL
};

/*--- Variables ---*/

static int game_lang = 'u';

/*--- Functions prototypes ---*/

static void load_background(room_t *this, int num_stage, int num_room, int num_camera);

static void load_room(room_t *this);
static int loadroom_ard(room_t *this, const char *filename);

static void load_font(game_t *this);

/*--- Functions ---*/

game_t *game_re3ps1game_ctor(game_t *this)
{
	this->movies_list = (char **) re3ps1game_movies;

	if (game_file_exists("cd_data/etc/sele_obf.tim")) {
		game_lang = 'f';
	}

	this->load_font = load_font;

	return this;
}

void room_re3ps1game_init(room_t *this)
{
	this->loadFile = load_room;
	this->load_background = load_background;
}

static void load_background(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_bg, num_stage, num_stage, num_room);

	logMsg(1, "bss: Start loading %s ...\n", filepath);

	logMsg(1, "bss: %s loading %s ...\n",
		background_bss_load(filepath, CHUNK_SIZE, 0) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static void load_room(room_t *this)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_room, this->num_stage, this->num_stage, this->num_room);

	logMsg(1, "ard: Start loading %s ...\n", filepath);

	logMsg(1, "ard: %s loading %s ...\n",
		loadroom_ard(this, filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int loadroom_ard(room_t *this, const char *filename)
{
	void *file;
	int len;

	file = ard_loadRdtFile(filename, &len);
	if (!file) {
		return 0;
	}

	this->file = file;
	this->file_length = len;

	return 1;
}

static void load_font(game_t *this)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
	const char *filename = re3ps1game_font;

	filepath = malloc(strlen(filename)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, filename, game_lang);

	logMsg(1, "Loading font from %s...\n", filepath);

	font_file = FS_Load(filepath, &length);
	if (font_file) {
		this->font = render.createTexture(0);
		if (this->font) {
			this->font->load_from_tim(this->font, font_file);
			retval = 1;
		}

		free(font_file);
	}

	logMsg(1, "Loading font from %s... %s\n", filepath, retval ? "Done" : "Failed");

	free(filepath);
}
