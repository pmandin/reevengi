/*
	RE1
	PS1
	Demo, Game

	Copyright (C) 2007	Patrice Mandin

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

#include "filesystem.h"
#include "log.h"
#include "render.h"
#include "parameters.h"

#include "../g_common/game.h"

#include "game_re1.h"
#include "background_bss.h"
#include "g_re1/emd.h"
#include "room_rdt.h"

/*--- Defines ---*/

#define CHUNK_SIZE 32768

/*--- Types ---*/

/*--- Constant ---*/

static const char *re1ps1_bg = "psx%s/stage%d/room%d%02x.bss";
static const char *re1ps1_room = "psx%s/stage%d/room%d%02x0.rdt";
static const char *re1ps1_model1 = "psx%s/enemy/char1%d.emd";
static const char *re1ps1_model2 = "psx%s/enemy/em10%02x.emd";
static const char *re1ps1_model3 = "psx%s/enemy/em11%02x.emd";
static const char *re1ps1_font = "psx%s/data/font.tim";

static const char *re1ps1demo_movies[] = {
	"psx/movie/capcom.str",
	"psx/movie/dm1.str",
	"psx/movie/dm2.str",
	"psx/movie/dm3.str",
	"psx/movie/oj.str",
	"psx/movie/pj.str",
	NULL
};

static const char *re1ps1game_movies[] = {
	"psx/movie/capcom.str",
	"psx/movie/dm1.str",
	"psx/movie/dm2.str",
	"psx/movie/dm3.str",
	"psx/movie/dm4.str",
	"psx/movie/dm6.str",
	"psx/movie/dm7.str",
	"psx/movie/dm8.str",
	"psx/movie/dmb.str",
	"psx/movie/dmc.str",
	"psx/movie/dmd.str",
	"psx/movie/dme.str",
	"psx/movie/dmf.str",
	"psx/movie/ed1.str",
	"psx/movie/ed2.str",
	"psx/movie/ed3.str",
	"psx/movie/ed4.str",
	"psx/movie/ed5.str",
	"psx/movie/ed6.str",
	"psx/movie/ed7.str",
	"psx/movie/ed8.str",
	"psx/movie/oj.str",
	"psx/movie/pj.str",
	"psx/movie/stfc.str",
	"psx/movie/stfj.str",
	NULL
};

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void re1ps1_loadbackground(void);

static void re1ps1_loadroom(void);
static int re1ps1_loadroom_rdt(const char *filename);

render_skel_t *re1ps1_load_model(int num_model);

static void load_font(void);

/*--- Functions ---*/

void game_re1ps1_init(game_t *this)
{
	this->room.priv_load = re1ps1_loadroom;
	this->room.priv_load_background = re1ps1_loadbackground;

	if (this->minor == GAME_RE1_PS1_DEMO) {
		this->movies_list = (char **) re1ps1demo_movies;
	} else {
		this->movies_list = (char **) re1ps1game_movies;
	}

	this->player.priv_load_model = re1ps1_load_model;

	this->load_font = load_font;
}

static void re1ps1_loadbackground(void)
{
	char *filepath;
	const char *is_shock = ((game.minor == GAME_RE1_PS1_SHOCK) ? "usa" : "");
	int re1_stage = (game.num_stage>5 ? game.num_stage-5 : game.num_stage);
	int row_offset = 0;

	if (re1_stage == 2) {
		if (game.num_room==0) {
			if (game.num_camera==0) {
				row_offset = -4;
			}
		}
	} else if (re1_stage == 3) {
		if (game.num_room==6) {
			/* All cameras angles for this room */
			row_offset = -4;
		} else if (game.num_room==7) {
			/* All cameras angles for this room */
			row_offset = -4;
		} else if (game.num_room==0x0b) {
			/* All cameras angles for this room */
			row_offset = -4;
		} else if (game.num_room==0x0f) {
			/* All cameras angles for this room */
			row_offset = -4;
		}
	} else if (re1_stage == 5) {
		if (game.num_room==0x0d) {
			row_offset = -4;
		}
		if (game.num_room==0x15) {
			row_offset = -4;
		}
	}

	filepath = malloc(strlen(re1ps1_bg)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_bg, is_shock, re1_stage, re1_stage, game.num_room);

	logMsg(1, "bss: Start loading %s ...\n", filepath);

	logMsg(1, "bss: %s loading %s ...\n",
		background_bss_load(filepath, CHUNK_SIZE, row_offset) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static void re1ps1_loadroom(void)
{
	char *filepath;
	const char *is_shock = ((game.minor == GAME_RE1_PS1_SHOCK) ? "usa" : "");

	filepath = malloc(strlen(re1ps1_room)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_room, is_shock, game.num_stage, game.num_stage, game.num_room);

	logMsg(1, "rdt: Start loading %s ...\n", filepath);

	logMsg(1, "rdt: %s loading %s ...\n",
		re1ps1_loadroom_rdt(filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int re1ps1_loadroom_rdt(const char *filename)
{
	PHYSFS_sint64 length;
	void *file;

	file = FS_Load(filename, &length);
	if (!file) {
		return 0;
	}

	game.room.file = file;
	game.room.file_length = length;

	room_rdt_init(&game.room);

	return 1;
}

render_skel_t *re1ps1_load_model(int num_model)
{
	char *filepath;
	const char *is_shock = ((game.minor == GAME_RE1_PS1_SHOCK) ? "usa" : "");
	const char *filename = re1ps1_model1;
	render_skel_t *model = NULL;
	void *emd;
	PHYSFS_sint64 emd_length;

	if (num_model>64) {
		num_model = 64;
	}

	if (num_model>0x03) {
		filename = re1ps1_model2;
		num_model -= 4;
		if (num_model>0x15) {
			num_model += 0x20-0x16;
		}
		if (num_model>0x2e) {
			num_model += 1;
		}
	}
	if (num_model>0x31) {
		filename = re1ps1_model3;
		num_model -= 0x32;
	}

	filepath = malloc(strlen(filename)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, filename, is_shock, num_model);

	logMsg(1, "emd: Start loading model %s ...\n", filepath);

	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		model = model_emd_load(emd, emd_length);
		/*free(emd);*/
	}

	logMsg(1, "emd: %s loading model %s ...\n",
		model ? "Done" : "Failed",
		filepath);

	free(filepath);
	return model;
}

static void load_font(void)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
	const char *is_shock = ((game.minor == GAME_RE1_PS1_SHOCK) ? "usa" : "");
	const char *filename = re1ps1_font;

	filepath = malloc(strlen(filename)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, filename, is_shock);

	logMsg(1, "Loading font from %s...\n", filepath);

	font_file = FS_Load(filepath, &length);
	if (font_file) {
		game.font = render.createTexture(0);
		if (game.font) {
			game.font->load_from_tim(game.font, font_file);
			retval = 1;
		}

		free(font_file);
	}

	logMsg(1, "Loading font from %s... %s\n", filepath, retval ? "Done" : "Failed");

	free(filepath);
}