/*
	RE3
	PC

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#ifdef ENABLE_SDLIMAGE
#include <SDL_image.h>
#endif

#include "filesystem.h"
#include "state.h"
#include "re3_pc.h"
#include "parameters.h"
#include "video.h"
#include "render.h"
#include "model_emd3.h"
#include "log.h"
#include "room_rdt2.h"

/*--- Defines ---*/

#define MAX_MODELS_DEMO	16
#define MAX_MODELS_GAME 65

/*--- Types ---*/

/*--- Constant ---*/

static const char *re3pc_bg = "data_a/bss/r%d%02x%02x.jpg";
static const char *re3pc_room = "data_%c/rdt/r%d%02x.rdt";
static const char *rofs_dat = "%s/rofs%d.dat";
static const char *rofs_cap_dat = "%s/Rofs%d.dat";
static const char *re3pc_model = "room/emd/em%02x.%s";

static const int map_models_demo[MAX_MODELS_DEMO]={
	0x10,	0x11,	0x12,	0x14,	0x17,	0x1c,	0x1d,	0x1e,
	0x1f,	0x20,	0x2d,	0x2f,	0x34,	0x53,	0x54,	0x58
};

static const int map_models_game[MAX_MODELS_GAME]={
	0x10,	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,
	0x18,	0x19,	0x1a,	0x1b,	0x1c,	0x1d,	0x1e,	0x1f,
	0x20,	0x21,	0x22,	0x23,	0x24,	0x25,	0x26,	0x27,
	0x28,	0x2c,	0x2d,	0x2e,	0x2f,	0x30,	0x32,	0x33,
	0x34,	0x35,	0x36,	0x37,	0x38,	0x39,	0x3a,	0x3b,
	0x3e,	0x3f,	0x40,	0x50,	0x51,	0x52,	0x53,	0x54,
	0x55,	0x56,	0x57,	0x58,	0x59,	0x5a,	0x5b,	0x5c,
	0x5d,	0x5e,	0x5f,	0x60,	0x61,	0x62,	0x63,	0x64,
	0x65
};

static const char *re3pcdemo_movies[] = {
	"zmovie/opn.dat",
	"zmovie/roopne.dat",
	"zmovie/ins01.dat",
	NULL
};

static const char *re3pcgame_movies[] = {
	"zmovie/Eidos.dat",
	"zmovie/opn.dat",
	"zmovie/roop.dat",
	"zmovie/roopne.dat",
	"zmovie/ins01.dat",
	"zmovie/ins02.dat",
	"zmovie/ins03.dat",
	"zmovie/ins04.dat",
	"zmovie/ins05.dat",
	"zmovie/ins06.dat",
	"zmovie/ins07.dat",
	"zmovie/ins08.dat",
	"zmovie/ins09.dat",
	"zmovie/enda.dat",
	"zmovie/endb.dat",
	NULL
};

/*--- Variables ---*/

static int game_lang = 'u';

/*--- Functions prototypes ---*/

static void re3pc_shutdown(void);

static void re3pc_loadbackground(void);
static int re3pc_load_jpg_bg(const char *filename);

static void re3pc_loadroom(void);
static int re3pc_loadroom_rdt(const char *filename);

static render_skel_t *re3pc_load_model(int num_model);

/*--- Functions ---*/

void re3pc_init(state_t *game_state)
{
	int i;
	char rofsfile[1024];

	for (i=1;i<16;i++) {
		sprintf(rofsfile, rofs_dat, params.basedir, i);
		if (FS_AddArchive(rofsfile)==0) {
			continue;
		}
		/* Try with cap letter */
		if (game_state->version==GAME_RE3_PC_GAME) {
			sprintf(rofsfile, rofs_cap_dat, params.basedir, i);
			FS_AddArchive(rofsfile);
		}
	}

	game_state->priv_load_background = re3pc_loadbackground;
	game_state->priv_load_room = re3pc_loadroom;
	game_state->priv_shutdown = re3pc_shutdown;

	switch(game_state->version) {
		case GAME_RE3_PC_DEMO:
			game_state->movies_list = (char **) re3pcdemo_movies;
			break;
		case GAME_RE3_PC_GAME:
			game_state->movies_list = (char **) re3pcgame_movies;
			game_lang = 'f';
			break;
	}

	game_state->priv_load_model = re3pc_load_model;
}

void re3pc_shutdown(void)
{
}

void re3pc_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re3pc_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3pc_bg, game_state.num_stage, game_state.num_room, game_state.num_camera);

	logMsg(1, "jpg: Start loading %s ...\n", filepath);

	logMsg(1, "jpg: %s loading %s ...\n",
		re3pc_load_jpg_bg(filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

int re3pc_load_jpg_bg(const char *filename)
{
#ifdef ENABLE_SDLIMAGE
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		SDL_Surface *image = IMG_Load_RW(src, 0);
		if (image) {
			game_state.background = render.createTexture(RENDER_TEXTURE_CACHEABLE);
			if (game_state.background) {
				game_state.background->load_from_surf(game_state.background, image);
				retval = 1;
			}

			SDL_FreeSurface(image);
		}

		SDL_RWclose(src);
	}

	return retval;
#else
	return 0;
#endif
}

static void re3pc_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re3pc_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3pc_room, game_lang, game_state.num_stage, game_state.num_room);

	logMsg(1, "rdt: Start loading %s ...\n", filepath);

	logMsg(1, "rdt: %s loading %s ...\n",
		re3pc_loadroom_rdt(filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int re3pc_loadroom_rdt(const char *filename)
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

	room_rdt2_init(game_state.room);

	return 1;
}

render_skel_t *re3pc_load_model(int num_model)
{
	char *filepath;
	render_skel_t *model = NULL;
	void *emd, *tim;
	PHYSFS_sint64 emd_length, tim_length;

	switch(game_state.version) {
		case GAME_RE3_PC_DEMO:
			if (num_model>=MAX_MODELS_DEMO) {
				num_model = MAX_MODELS_DEMO-1;
			}
			num_model = map_models_demo[num_model];
			break;
		case GAME_RE3_PC_GAME:
			if (num_model>=MAX_MODELS_GAME) {
				num_model = MAX_MODELS_GAME-1;
			}
			num_model = map_models_game[num_model];
			break;
		default:
			return NULL;
	}

	filepath = malloc(strlen(re3pc_model)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re3pc_model, num_model, "emd");

	logMsg(1, "emd: Start loading model %s ...\n", filepath);

	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		sprintf(filepath, re3pc_model, num_model, "tim");
		tim = FS_Load(filepath, &tim_length);
		if (tim) {
			model = model_emd3_load(emd, tim, emd_length, tim_length);

			free(tim);
		}
		free(emd);
	}	

	logMsg(1, "emd: %s loading model %s\n",
		model ? "Done" : "Failed",
		filepath);

	free(filepath);
	return model;
}
