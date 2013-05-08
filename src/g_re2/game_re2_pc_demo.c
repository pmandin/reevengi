/*
	RE2
	PC
	Demo

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
#include "parameters.h"
#include "log.h"

#include "../render_texture.h"

#include "../g_common/player.h"
#include "../g_common/room.h"
#include "../g_common/game.h"

#include "game_re2.h"
#include "depack_adt.h"
#include "video.h"
#include "render.h"
#include "emd.h"
#include "room_rdt2.h"

/*--- Defines ---*/

#define MAX_MODELS	0x17

/*--- Types ---*/

/*--- Constant ---*/

static const char *re2pcdemo_bg = "common/stage%d/rc%d%02x%1x.adt";
static const char *re2pcdemo_bgmask = "common/stage%d/rs%d%02x%1x.adt";
static const char *re2pcdemo_room = "pl0/rd%c/room%d%02x0.rdt";
static const char *re2pcdemo_model = "pl0/emd0/em0%02x.%s";
static const char *re2pcdemo_font = "common/dat%c/select_w.tim";

static const int map_models[MAX_MODELS]={
	0x10,	0x11,	0x12,	0x13,	0x15,	0x16,	0x17,	0x18,
	0x1e,	0x1f,	0x20,	0x21,	0x22,	0x2d,	0x48,	0x4a,
	0x50,	0x51,	0x54,	0x55,	0x58,	0x59,	0x5a
};

/*--- Variables ---*/

static int game_lang = 'u';

/*--- Functions prototypes ---*/

static void load_room(room_t *this, int num_stage, int num_room, int num_camera);
static int loadroom_rdt(room_t *this, const char *filename);

static void load_background(room_t *this, int num_stage, int num_room, int num_camera);
static int load_adt_bg(room_t *this, const char *filename);

static void load_bgmask(room_t *this, int num_stage, int num_room, int num_camera);
static int load_adt_bgmask(room_t *this, const char *filename);

static render_skel_t *load_model(player_t *this, int num_model);
static void get_model_name(player_t *this, char name[32]);

static void load_font(game_t *this);

/*--- Functions ---*/

game_t *game_re2pcdemo_ctor(game_t *this)
{
	this->room->load = load_room;
	this->room->load_background = load_background;
	this->room->load_bgmask = load_bgmask;

	if (this->minor == GAME_RE2_PC_DEMO_P) {
		game_lang = 'p';
	}

	this->player->load_model = load_model;
	this->player->get_model_name = get_model_name;

	this->load_font = load_font;

	return this;
}

static void load_room(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;

	filepath = malloc(strlen(re2pcdemo_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcdemo_room, game_lang, num_stage, num_room);

	logMsg(1, "adt: Start loading %s ...\n", filepath);

	logMsg(1, "adt: %s loading %s ...\n",
		loadroom_rdt(this, filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int loadroom_rdt(room_t *this, const char *filename)
{
	PHYSFS_sint64 length;
	void *file;

	file = FS_Load(filename, &length);
	if (!file) {
		return 0;
	}

	this->file = file;
	this->file_length = length;
	this->init(this);

	return 1;
}

static void load_background(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;

	filepath = malloc(strlen(re2pcdemo_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcdemo_bg, num_stage, num_stage, num_room, num_camera);

	logMsg(1, "adt: Start loading %s ...\n", filepath);

	logMsg(1, "adt: %s loading %s ...\n",
		load_adt_bg(this, filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int load_adt_bg(room_t *this, const char *filename)
{
	SDL_RWops *src;
	int retval = 0;

	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		adt_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			if (dstBufLen == 320*256*2) {
				SDL_Surface *image = adt_surface((Uint16 *) dstBuffer, 1);
				if (image) {
					this->background = render.createTexture(RENDER_TEXTURE_CACHEABLE);
					if (this->background) {
						this->background->load_from_surf(this->background, image);
						retval = 1;
					}
					SDL_FreeSurface(image);
				}
			}
			free(dstBuffer);
		}
		SDL_RWclose(src);
	}

	return retval;
}

static void load_bgmask(room_t *this, int num_stage, int num_room, int num_camera)
{
	char *filepath;

	filepath = malloc(strlen(re2pcdemo_bgmask)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcdemo_bgmask, num_stage, num_stage,
		num_room, num_camera);

	logMsg(1, "adt: Start loading %s ...\n", filepath);

	logMsg(1, "adt: %s loading %s ...\n",
		load_adt_bgmask(this, filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int load_adt_bgmask(room_t *this, const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		adt_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			this->bg_mask = render.createTexture(RENDER_TEXTURE_MUST_POT);
			if (this->bg_mask) {
				this->bg_mask->load_from_tim(this->bg_mask, dstBuffer);

				retval = 1;
			}
			free(dstBuffer);
		}
		SDL_RWclose(src);
	}

	return retval;
}

static render_skel_t *load_model(player_t *this, int num_model)
{
	char *filepath;
	render_skel_t *model = NULL;
	void *emd, *tim;
	PHYSFS_sint64 emd_length, tim_length;

	if (num_model>=MAX_MODELS) {
		num_model = MAX_MODELS-1;
	}
	num_model = map_models[num_model];

	filepath = malloc(strlen(re2pcdemo_model)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re2pcdemo_model, num_model, "emd");

	logMsg(1, "emd: Start loading model %s ...\n", filepath);

	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		sprintf(filepath, re2pcdemo_model, num_model, "tim");
		tim = FS_Load(filepath, &tim_length);
		if (tim) {
			model = model_emd2_load(emd, tim, emd_length, tim_length);
			free(tim);
		}
		/*free(emd);*/
	}	

	logMsg(1, "emd: %s loading model %s ...\n",
		model ? "Done" : "Failed",
		filepath);

	free(filepath);
	return model;
}

static void get_model_name(player_t *this, char name[32])
{
	int num_model = this->num_model;

	if (num_model>MAX_MODELS-1) {
		num_model = MAX_MODELS-1;
	}

	sprintf(name, "em0%02x.emd", map_models[num_model]);
}

static void load_font(game_t *this)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
	const char *filename = re2pcdemo_font;

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
