/*
	RE1
	PC
	Game

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
#include "state.h"
#include "depack_pak.h"
#include "model.h"
#include "re1_pc_game.h"
#include "parameters.h"
#include "video.h"
#include "model_emd.h"
#include "log.h"
#include "background_tim.h"
#include "room_rdt.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- Constant ---*/

static const char *re1pcgame_bg = "horr/usa/stage%d/rc%d%02x%d.pak";
static const char *re1pcgame_room = "horr/usa/stage%d/room%d%02x0.rdt";
static const char *re1pcgame_model1 = "horr/usa/enemy/char1%d.emd";
static const char *re1pcgame_model2 = "horr/usa/enemy/em10%02x.emd";
static const char *re1pcgame_model3 = "horr/usa/enemy/em11%02x.emd";

static const char *re1pcgame_movies[] = {
	"horr/usa/movie/capcom.avi",
	"horr/usa/movie/dm1.avi",
	"horr/usa/movie/dm2.avi",
	"horr/usa/movie/dm3.avi",
	"horr/usa/movie/dm4.avi",
	"horr/usa/movie/dm6.avi",
	"horr/usa/movie/dm7.avi",
	"horr/usa/movie/dm8.avi",
	"horr/usa/movie/dmb.avi",
	"horr/usa/movie/dmc.avi",
	"horr/usa/movie/dmd.avi",
	"horr/usa/movie/dme.avi",
	"horr/usa/movie/dmf.avi",
	"horr/usa/movie/ed1.avi",
	"horr/usa/movie/ed2.avi",
	"horr/usa/movie/ed3.avi",
	"horr/usa/movie/ed6.avi",
	"horr/usa/movie/ed7.avi",
	"horr/usa/movie/ed8.avi",
	"horr/usa/movie/eu4.avi",
	"horr/usa/movie/eu5.avi",
	"horr/usa/movie/ou.avi",
	"horr/usa/movie/pu.avi",
	"horr/usa/movie/staf_r.avi",
	"horr/usa/movie/stfc_r.avi",
	"horr/usa/movie/stfj_r.avi",
	"horr/usa/movie/stfz_r.avi",
	NULL
};

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void re1pcgame_shutdown(void);

static void re1pcgame_loadbackground(void);
static int re1pcgame_load_pak_bg(const char *filename);

static void re1pcgame_loadroom(void);
static int re1pcgame_loadroom_rdt(const char *filename);

static model_t *re1pcgame_load_model(int num_model);

/*--- Functions ---*/

void re1pcgame_init(state_t *game_state)
{
	game_state->priv_load_background = re1pcgame_loadbackground;
	game_state->priv_load_room = re1pcgame_loadroom;
	game_state->priv_shutdown = re1pcgame_shutdown;

	game_state->movies_list = (char **) re1pcgame_movies;

	game_state->priv_load_model = re1pcgame_load_model;
}

void re1pcgame_shutdown(void)
{
}

void re1pcgame_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re1pcgame_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_bg, game_state.num_stage, game_state.num_stage,
		game_state.num_room, game_state.num_camera);

	logMsg(1, "pak: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re1pcgame_load_pak_bg(filepath) ? "done" : "failed");

	free(filepath);
}

int re1pcgame_load_pak_bg(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		pak_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			SDL_RWops *tim_src;
			/*game_state.num_cameras = 8;*/
			
			tim_src = SDL_RWFromMem(dstBuffer, dstBufLen);
			if (tim_src) {
				SDL_Surface *image;

				image = background_tim_load(tim_src);
				if (image) {
					game_state.back_surf = video.createSurfaceSu(image);
					if (game_state.back_surf) {
						video.convertSurface(game_state.back_surf);
						retval = 1;
					}
					SDL_FreeSurface(image);
				}

				SDL_FreeRW(tim_src);
			}

			free(dstBuffer);
		}

		SDL_RWclose(src);
	}

	return retval;
}

static void re1pcgame_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re1pcgame_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_room, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "rdt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re1pcgame_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re1pcgame_loadroom_rdt(const char *filename)
{
	PHYSFS_sint64 length;
	void *file;

	file = FS_Load(filename, &length);
	if (!file) {
		return 0;
	}

	game_state.room = room_create(file);
	if (!game_state.room) {
		free(file);
		return 0;
	}

	room_rdt_init(game_state.room);

	return 1;
}

model_t *re1pcgame_load_model(int num_model)
{
	char *filepath;
	const char *filename = re1pcgame_model1;
	model_t *model = NULL;
	void *emd;
	PHYSFS_sint64 emd_length;

	if (num_model>0x03) {
		filename = re1pcgame_model2;
		num_model -= 4;
		if (num_model>0x15) {
			num_model += 0x20-0x16;
		}
		if (num_model>0x2e) {
			num_model += 1;
		}
	}
	if (num_model>0x33) {
		filename = re1pcgame_model3;
		num_model -= 0x34;
	}
	if (num_model>0x15) {
		num_model = 0x15;
	}

	filepath = malloc(strlen(filename)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, filename, num_model);

	logMsg(1, "Loading model %s...", filepath);
	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		model = model_emd_load(emd, emd_length);
	}	
	logMsg(1, "%s\n", model ? "done" : "failed");

	free(filepath);
	return model;
}
