/*
	RE1
	PC
	Game

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

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "../filesystem.h"
#include "../log.h"
#include "../render.h"
#include "../parameters.h"
#include "../background_tim.h"

#include "../g_common/player.h"
#include "../g_common/room.h"
#include "../g_common/game.h"

#include "game_re1.h"
#include "pak.h"
#include "emd.h"

/*--- Defines ---*/

#define NUM_COUNTRIES 8

/*--- Types ---*/

/*--- Constant ---*/

static const char *re1_country[NUM_COUNTRIES]={
	"horr/usa",
	"horr/ger",
	"horr/jpn",
	"horr/fra",
	"usa",
	"ger",
	"jpn",
	"fra"
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

static char *getFilename(room_t *this);

static int get_row_offset(int re1_stage, int num_stage, int num_room, int num_camera);

static void load_background(room_t *this, int num_stage, int num_room, int num_camera);
static int load_pak_bg(room_t *this, const char *filename, int row_offset);

#if 0
static void load_bgmask(room_t *this, int num_stage, int num_room, int num_camera);
static int load_pak_bgmask(room_t *this, const char *filename, int row_offset);
#endif

static render_skel_t *load_model(player_t *this, int num_model);

static void load_font(game_t *this);

/*--- Functions ---*/

game_t *game_re1pc_ctor(game_t *this)
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

	this->movies_list = (char **) re1pcgame_movies;

	this->load_font = load_font;

	this->player->load_model = load_model;

	return this;
}

void room_re1pc_init(room_t *this)
{
	this->getFilename = getFilename;

	this->load_background = load_background;
/*	this->load_bgmask = load_bgmask;*/
}

static char *getFilename(room_t *this)
{
	char *filepath;

	filepath = malloc(strlen(re1pcgame_room)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re1pcgame_room, re1_country[game_country],
		this->num_stage, this->num_stage, this->num_room);

	logMsg(1, "game_re1: Room filename %s\n", filepath);

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

	filepath = malloc(strlen(re1pcgame_bg)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_bg, re1_country[game_country], re1_stage, re1_stage,
		num_room, num_camera);

	logMsg(1, "pak: Start loading %s ...\n", filepath);

	logMsg(1, "pak: %s loading %s ...\n",
		load_pak_bg(this, filepath, row_offset) ? "Done" : "Failed",
		filepath);

	free(filepath);

/*	load_bgmask(row_offset, re1_stage);*/
}

static int load_pak_bg(room_t *this, const char *filename, int row_offset)
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
					this->background = render.createTexture(RENDER_TEXTURE_CACHEABLE);
					if (this->background) {
						this->background->load_from_surf(this->background, image);
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
static void load_bgmask(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;
	int re1_stage = (num_stage>5 ? num_stage-5 : num_stage);
	int row_offset = get_row_offset(re1_stage, num_stage, num_room, num_camera);

	filepath = malloc(strlen(re1pcgame_bgmask)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_bgmask, re1_country[game_country], re1_stage-1,
		num_room, num_camera);

	logMsg(1, "pak: Start loading %s ...\n", filepath);

	logMsg(1, "pak: %s loading %s ...\n",
		load_pak_bgmask(this, filepath, row_offset) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int load_pak_bgmask(room_t *this, const char *filename, int row_offset)
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
					this->bg_mask = render.createTexture(RENDER_TEXTURE_MUST_POT);
					if (this->bg_mask) {
						/*this->bg_mask->load_from_surf(this->bg_mask, image);*/
						this->bg_mask->load_from_tim(this->bg_mask, dstBuffer);
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

static render_skel_t *load_model(player_t *this, int num_model)
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

static void load_font(game_t *this)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;

	filepath = malloc(strlen(re1pcgame_font)+32);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_font, re1_country[game_country]);

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
