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
#include "background_bss.h"

#include "../g_common/room.h"
#include "../g_common/player.h"
#include "../g_common/game.h"

#include "game_re1.h"
#include "emd.h"

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

static const char *is_shock = "";

/*--- Functions prototypes ---*/

static char *getFilename(room_t *this, int num_stage, int num_room, int num_camera);

static int get_row_offset(int re1_stage, int num_stage, int num_room, int num_camera);

static void load_background(room_t *this, int num_stage, int num_room, int num_camera);

static render_skel_t *load_model(player_t *this, int num_model);

static void load_font(game_t *this);

/*--- Functions ---*/

game_t *game_re1ps1_ctor(game_t *this)
{
	if (this->minor == GAME_RE1_PS1_DEMO) {
		this->movies_list = (char **) re1ps1demo_movies;
	} else {
		this->movies_list = (char **) re1ps1game_movies;
	}

	this->player->load_model = load_model;

	this->load_font = load_font;

	if (game->minor == GAME_RE1_PS1_SHOCK) {
		is_shock = "usa";
	}

	return this;
}

void room_re1ps1_init(room_t *this)
{
	this->getFilename = getFilename;

	this->load_background = load_background;
/*	this->load_bgmask = load_bgmask;*/
}

static char *getFilename(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;

	filepath = malloc(strlen(re1ps1_room)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re1ps1_room, is_shock, num_stage, num_stage, num_room);

	return filepath;
}

static int get_row_offset(int re1_stage, int num_stage, int num_room, int num_camera)
{
	int row_offset = 0;

	if (re1_stage == 2) {
		if (num_room==0) {
			if (num_camera==0) {
				row_offset = -4;
			}
		}
	} else if (re1_stage == 3) {
		if (num_room==6) {
			if (num_camera!=2) {
				row_offset = -4;
			}
		} else if (num_room==7) {
			/* All cameras angles for this room */
			row_offset = -4;
		} else if (num_room==0x0b) {
			/* All cameras angles for this room */
			row_offset = -4;
		} else if (num_room==0x0f) {
			/* All cameras angles for this room */
			row_offset = -4;
		}
	} else if (re1_stage == 5) {
		if (num_room==0x0d) {
			row_offset = -4;
		}
		if (num_room==0x15) {
			row_offset = -4;
		}
	}

	return row_offset;
}

static void load_background(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;
	int re1_stage = (num_stage>5 ? num_stage-5 : num_stage);
	int row_offset = get_row_offset(re1_stage, num_stage, num_room, num_camera);

	filepath = malloc(strlen(re1ps1_bg)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_bg, is_shock, re1_stage, re1_stage, num_room);

	logMsg(1, "bss: Start loading %s ...\n", filepath);

	logMsg(1, "bss: %s loading %s ...\n",
		background_bss_load(filepath, CHUNK_SIZE, row_offset) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static render_skel_t *load_model(player_t *this, int num_model)
{
	char *filepath;
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

static void load_font(game_t *this)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
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
