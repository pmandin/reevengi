/*
	RE2
	PS1
	Demo, Game Leon, Game Claire

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

#include "../filesystem.h"
#include "../parameters.h"
#include "../log.h"
#include "../background_bss.h"

#include "../g_common/player.h"
#include "../g_common/room.h"
#include "../g_common/game.h"

#include "../r_common/render_skel.h"

#include "game_re2.h"
#include "emd.h"
#include "ems.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Constant ---*/

static const char *re2ps1_bg_path1 = "common/bss/";
static const char *re2ps1_bg_path2 = "res2/zcommon/bss%s/";

static const char *re2ps1_room_path1 = "pl%d/rdt/";
static const char *re2ps1_room_path2 = "res2/zpl%d/rdt%s/";

static const char *re2ps1demo_movies[] = {
	"zmovie/capcom.str",
	"zmovie/info.str",
	"zmovie/r10b.str",
	NULL
};

static const char *re2ps1demo2_movies[] = {
	"res2/zz/virgin.str",
	NULL
};

static const char *re2ps1game_leon_movies[] = {
	"pl0/zmovie/opn1stl.str",
	"pl0/zmovie/opn2ndl.str",
	"pl0/zmovie/opn2ndrl.str",
	"pl0/zmovie/r108l.str",
	"pl0/zmovie/r204l.str",
	"pl0/zmovie/r409.str",
	"pl0/zmovie/r700l.str",
	"pl0/zmovie/r703l.str",
	"pl0/zmovie/r704l.str",
	"pl0/zmovie/titlel.str",
	"zmovie/r109.str",
	"zmovie/r10b.str",
	"zmovie/r200.str",
	"zmovie/r505.str",
	"zmovie/virgin.str",
	NULL
};

static const char *re2ps1game_claire_movies[] = {
	"pl1/zmovie/opn1stc.str",
	"pl1/zmovie/opn2ndc.str",
	"pl1/zmovie/opn2ndrc.str",
	"pl1/zmovie/r108c.str",
	"pl1/zmovie/r204c.str",
	"pl1/zmovie/r408.str",
	"pl1/zmovie/r700c.str",
	"pl1/zmovie/r703c.str",
	"pl1/zmovie/r704c.str",
	"pl1/zmovie/titlec.str",
	"zmovie/r109.str",
	"zmovie/r10b.str",
	"zmovie/r200.str",
	"zmovie/r505.str",
	"zmovie/virgin.str",
	NULL
};

/*--- Variables ---*/

static const char *re2ps1_bg_path;
static const char *re2ps1_room_path;

static int game_player;

/*--- Functions prototypes ---*/

static char *getFilename(room_t *this);

static void load_background(room_t *this, int num_stage, int num_room, int num_camera);

static render_skel_t *load_model(player_t *this, int num_model);

/*--- Functions ---*/

game_t *game_re2ps1_ctor(game_t *this)
{
	re2ps1_bg_path = re2ps1_bg_path1;
	re2ps1_room_path = re2ps1_room_path1;
	game_player = 0;

	switch(this->minor) {
		case GAME_RE2_PS1_DEMO:
			this->movies_list = (char **) re2ps1demo_movies;
			break;
		case GAME_RE2_PS1_DEMO2:
			this->movies_list = (char **) re2ps1demo2_movies;
			re2ps1_bg_path = re2ps1_bg_path2;
			re2ps1_room_path = re2ps1_room_path2;
			break;
		case GAME_RE2_PS1_GAME_LEON:
			this->movies_list = (char **) re2ps1game_leon_movies;
			break;
		case GAME_RE2_PS1_GAME_CLAIRE:
			this->movies_list = (char **) re2ps1game_claire_movies;
			game_player = 1;
			break;
	}

	this->player->load_model = load_model;

	return this;
}

void room_re2ps1_init(room_t *this)
{
	this->getFilename = getFilename;
	this->load_background = load_background;
}

static char *getFilename(room_t *this)
{
	char *filepath;
	char filename[16];

	filepath = malloc(strlen(re2ps1_room_path)+1+sizeof(filename));
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re2ps1_room_path, game_player, (this->num_stage==1) ? "" : "2");

	sprintf(filename, "room%d%02x%d.rdt", this->num_stage, this->num_room, game_player);
	strcat(filepath, filename);

	return filepath;
}

static void load_background(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;
	char filename[16];

	filepath = malloc(strlen(re2ps1_bg_path)+1+sizeof(filename));
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2ps1_bg_path, (num_stage==1) ? "" : "2");

	sprintf(filename, "room%d%02x.bss", num_stage, num_room);
	strcat(filepath, filename);

	logMsg(1, "bss: Start loading %s ...\n", filepath);

	logMsg(1, "bss: %s loading %s ...\n",
		background_bss_load(filepath, CHUNK_SIZE, 0) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

render_skel_t *load_model(player_t *this, int num_model)
{
	char *filepath;
	render_skel_t *model = NULL;
	SDL_RWops *src;
	int num_tim = -1, num_emd = -1;
	const re2ps1_ems_t *ems_array;
	Uint32 emd_offset, tim_offset;
	Uint32 emd_length, tim_length;
	void *emdBuf, *timBuf;

	ems_getModel(game, num_model, &filepath, &ems_array, &num_emd, &num_tim);
	if (!filepath || !ems_array || (num_emd==-1) || (num_tim==-1)) {
		return NULL;
	}

	emd_offset = ems_array[num_emd].offset;
	emd_length = ems_array[num_emd+1].offset - emd_offset;
	tim_offset = ems_array[num_tim].offset;
	tim_length = ems_array[num_tim+1].offset - tim_offset;

	logMsg(1, "emd: Start loading model 0x%02x from %s ...\n",
		num_model, filepath);

	src = FS_makeRWops(filepath);	
	if (src) {
		/* Read TIM file */
		SDL_RWseek(src, tim_offset, RW_SEEK_SET);
		timBuf = malloc(tim_length);
		if (timBuf) {
			SDL_RWread(src, timBuf, tim_length, 1);

			/* Read EMD file */
			SDL_RWseek(src, emd_offset, RW_SEEK_SET);
			emdBuf = malloc(emd_length);
			if (emdBuf) {
				SDL_RWread(src, emdBuf, emd_length, 1);

				model = model_emd2_load(emdBuf, timBuf, emd_length, tim_length);

				/*free(emdBuf);*/
			}
			free(timBuf);
		}

		SDL_RWclose(src);
	}

	logMsg(1, "emd: %s loading model 0x%02x from %s ...\n",
		model ? "Done" : "Failed",
		num_model, filepath);

	free(filepath);
	return model;
}
