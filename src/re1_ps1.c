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

#include "filesystem.h"
#include "model.h"
#include "state.h"
#include "re1_ps1.h"
#include "background_bss.h"
#include "parameters.h"
#include "model_emd.h"
#include "log.h"
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

static void re1ps1_shutdown(void);

static void re1ps1_loadbackground(void);

static void re1ps1_loadroom(void);
static int re1ps1_loadroom_rdt(const char *filename);

model_t *re1ps1_load_model(int num_model);

/*--- Functions ---*/

void re1ps1_init(state_t *game_state)
{
	game_state->priv_load_background = re1ps1_loadbackground;
	game_state->priv_load_room = re1ps1_loadroom;
	game_state->priv_shutdown = re1ps1_shutdown;

	if (game_state->version == GAME_RE1_PS1_DEMO) {
		game_state->movies_list = (char **) re1ps1demo_movies;
	} else {
		game_state->movies_list = (char **) re1ps1game_movies;
	}

	game_state->priv_load_model = re1ps1_load_model;
}

static void re1ps1_shutdown(void)
{
}

static void re1ps1_loadbackground(void)
{
	char *filepath;
	const char *is_shock = ((game_state.version == GAME_RE1_PS1_SHOCK) ? "usa" : "");
	int re1_stage = (game_state.num_stage>5 ? game_state.num_stage-5 : game_state.num_stage);
	int row_offset = 0;

	if (re1_stage == 2) {
		if (game_state.num_room==0) {
			row_offset = -4;
		}
	}

	filepath = malloc(strlen(re1ps1_bg)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_bg, is_shock, re1_stage, re1_stage, game_state.num_room);

	logMsg(1, "bss: Loading %s ... ", filepath);
	logMsg(1, "%s\n", background_bss_load(filepath, CHUNK_SIZE, row_offset) ? "done" : "failed");

	free(filepath);
}

static void re1ps1_loadroom(void)
{
	char *filepath;
	const char *is_shock = ((game_state.version == GAME_RE1_PS1_SHOCK) ? "usa" : "");

	filepath = malloc(strlen(re1ps1_room)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_room, is_shock, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "rdt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re1ps1_loadroom_rdt(filepath) ? "done" : "failed");

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

	game_state.room = room_create(file, length);
	if (!game_state.room) {
		free(file);
		return 0;
	}

	room_rdt_init(game_state.room);

	return 1;
}

model_t *re1ps1_load_model(int num_model)
{
	char *filepath;
	const char *is_shock = ((game_state.version == GAME_RE1_PS1_SHOCK) ? "usa" : "");
	const char *filename = re1ps1_model1;
	model_t *model = NULL;
	void *emd;
	PHYSFS_sint64 emd_length;

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
	if (num_model>0x15) {
		num_model = 0x15;
	}

	filepath = malloc(strlen(filename)+16);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, filename, is_shock, num_model);

	logMsg(1, "Loading model %s...", filepath);
	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		model = model_emd_load(emd, emd_length);
	}	
	logMsg(1, "%s\n", model ? "done" : "failed");

	free(filepath);
	return model;
}
