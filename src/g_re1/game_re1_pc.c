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
#include "log.h"
#include "render.h"
#include "parameters.h"

#include "../g_common/game.h"

#include "game_re1.h"
#include "depack_pak.h"
#include "video.h"
#include "g_re1/emd.h"
#include "background_tim.h"
#include "room_rdt.h"

/*--- Defines ---*/

#define NUM_COUNTRIES 6

/*--- Types ---*/

/*--- Constant ---*/

static const char *re1_country[6]={
	"horr/usa",
	"horr/ger",
	"horr/jpn",
	"usa",
	"ger",
	"jpn"
};

static const char *re1pcgame_bg = "%s/stage%d/rc%d%02x%d.pak";
/*static const char *re1pcgame_bgmask = "%s/objspr/osp%02d%02d%d.pak";*/
static const char *re1pcgame_room = "%s/stage%d/room%d%02x0.rdt";
static const char *re1pcgame_model1 = "%s/enemy/char1%d.emd";
static const char *re1pcgame_model2 = "%s/enemy/em10%02x.emd";
static const char *re1pcgame_model3 = "%s/enemy/em11%02x.emd";
static const char *re1pcgame_font = "%s/data/fontus.tim";

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

static int game_country = 0;

/*--- Functions prototypes ---*/

static void re1pcgame_loadbackground(void);
/*static void re1pcgame_loadbackground_mask(int row_offset, int re1_stage);*/
static int re1pcgame_load_pak_bg(const char *filename, int row_offset);
/*static int re1pcgame_load_pak_bgmask(const char *filename, int row_offset);*/

static void re1pcgame_loadroom(void);
static int re1pcgame_loadroom_rdt(const char *filename);

static render_skel_t *re1pcgame_load_model(int num_model);

static void load_font(void);

/*--- Functions ---*/

void game_re1pc_init(game_t *this)
{
	int i;
	char filename[32];

	for (i=0; i<NUM_COUNTRIES; i++) {
		sprintf(filename, "%s/data/capcom.ptc", re1_country[i]);
		if (game_file_exists(filename)) {
			game_country = i;
			break;
		}
	}

	game.room.priv_load_background = re1pcgame_loadbackground;
	game.room.priv_load = re1pcgame_loadroom;

	game.movies_list = (char **) re1pcgame_movies;

	game.player.priv_load_model = re1pcgame_load_model;

	game.load_font = load_font;
}

void re1pcgame_loadbackground(void)
{
	char *filepath;
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
			if (game.num_camera!=2) {
				row_offset = -4;
			}
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

	filepath = malloc(strlen(re1pcgame_bg)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_bg, re1_country[game_country], re1_stage, re1_stage,
		game.num_room, game.num_camera);

	logMsg(1, "pak: Start loading %s ...\n", filepath);

	logMsg(1, "pak: %s loading %s ...\n",
		re1pcgame_load_pak_bg(filepath, row_offset) ? "Done" : "Failed",
		filepath);

	free(filepath);

	/*re1pcgame_loadbackground_mask(row_offset, re1_stage);*/
}

#if 0
static void re1pcgame_loadbackground_mask(int row_offset, int re1_stage)
{
	char *filepath;

	filepath = malloc(strlen(re1pcgame_bgmask)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_bgmask, re1_country[game_country], re1_stage-1,
		game_state.num_room, game_state.num_camera);

	logMsg(1, "pak: Start loading %s ...\n", filepath);

	logMsg(1, "pak: %s loading %s ...\n",
		re1pcgame_load_pak_bgmask(filepath, row_offset) ? "Done" : "Failed",
		filepath);

	free(filepath);
}
#endif

int re1pcgame_load_pak_bg(const char *filename, int row_offset)
{
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		pak_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			SDL_RWops *tim_src = SDL_RWFromMem(dstBuffer, dstBufLen);
			if (tim_src) {
				SDL_Surface *image = background_tim_load(tim_src, row_offset);
				if (image) {
					game.room.background = render.createTexture(RENDER_TEXTURE_CACHEABLE);
					if (game.room.background) {
						game.room.background->load_from_surf(game.room.background, image);
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

#if 0
int re1pcgame_load_pak_bgmask(const char *filename, int row_offset)
{
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		pak_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			/* FIXME: take into account row_offset to match background */

			/*SDL_RWops *tim_src = SDL_RWFromMem(dstBuffer, dstBufLen);
			if (tim_src) {
				SDL_Surface *image = background_tim_load(tim_src, row_offset);
				if (image) {*/
					game_state.bg_mask = render.createTexture(RENDER_TEXTURE_MUST_POT);
					if (game_state.bg_mask) {
						/*game_state.bg_mask->load_from_surf(game_state.bg_mask, image);*/
						game_state.bg_mask->load_from_tim(game_state.bg_mask, dstBuffer);
						retval = 1;
					}
				/*	SDL_FreeSurface(image);
				}
				SDL_FreeRW(tim_src);
			}*/
			free(dstBuffer);
		}
		SDL_RWclose(src);
	}

	return retval;
}
#endif

static void re1pcgame_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re1pcgame_room)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_room, re1_country[game_country],
		game.num_stage, game.num_stage, game.num_room);

	logMsg(1, "rdt: Start loading %s ...\n", filepath);

	logMsg(1, "rdt: %s loading %s ...\n",
		re1pcgame_loadroom_rdt(filepath) ? "Done" : "Failed",
		filepath);

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

	game.room.file = file;
	game.room.file_length = length;

	room_rdt_init(&game.room);

	return 1;
}

render_skel_t *re1pcgame_load_model(int num_model)
{
	char *filepath;
	const char *filename = re1pcgame_model1;
	render_skel_t *model = NULL;
	void *emd;
	PHYSFS_sint64 emd_length;

	if (num_model>66) {
		num_model = 66;
	}

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

	filepath = malloc(strlen(filename)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, filename, re1_country[game_country], num_model);

	logMsg(1, "emd: Start loading model %s...\n", filepath);

	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		model = model_emd_load(emd, emd_length);
		/*free(emd);*/
	}

	logMsg(1, "emd: %s loading model %s...\n",
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

	logMsg(1, "Loading font from %s...\n", re1pcgame_font);

	filepath = malloc(strlen(re1pcgame_font)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_font, re1_country[game_country]);

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
