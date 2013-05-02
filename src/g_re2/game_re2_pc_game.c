/*
	RE2
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
#include "parameters.h"

#include "../g_common/game.h"

#include "game_re2.h"
#include "depack_adt.h"
#include "g_re2/emd.h"
#include "room_rdt2.h"
#include "video.h"
#include "render.h"

/*--- Defines ---*/

#define MAX_MODELS	58

/*--- Types ---*/

typedef struct {
	long offset;
	long length;
} re2_images_t;

/*--- Constant ---*/

static const char *re2pcgame_bg_archive = "COMMON/BIN/ROOMCUT.BIN";
static const char *re2pcgame_room = "PL%d/RD%c/ROOM%d%02x%d.RDT";
static const char *re2pcgame_model = "PL%d/EMD%d/EM%d%02x.%s";
static const char *re2pcgame_font = "COMMON/DAT%c/SELECT_W.TIM";

static const int map_models[MAX_MODELS]={
	0x10,	0x11,	0x12,	0x13,	0x15,	0x16,	0x17,	0x18,
	0x1e,	0x1f,	0x20,	0x21,	0x22,	0x23,	0x24,	0x25,
	0x26,	0x27,	0x28,	0x29,	0x2a,	0x2b,	0x2c,	0x2d,
	0x2e,	0x2f,	0x30,	0x31,	0x33,	0x34,	0x36,	0x37,
	0x38,	0x39,	0x3a,	0x3b,	0x3e,	0x3f,	0x40,	0x41,
	0x42,	0x43,	0x44,	0x45,	0x46,	0x47,	0x48,	0x49,
	0x4a,	0x4b,	0x4f,	0x50,	0x51,	0x54,	0x55,	0x58,
	0x59,	0x5a
};

static const char *re2pcgame_leon_movies[] = {
	"PL0/ZMOVIE/OPN1STL.BIN",
	"PL0/ZMOVIE/OPN2NDL.BIN",
	"PL0/ZMOVIE/OPN2NDRL.BIN",
	"PL0/ZMOVIE/R108L.BIN",
	"PL0/ZMOVIE/R204L.BIN",
	"PL0/ZMOVIE/R409.BIN",
	"PL0/ZMOVIE/R700L.BIN",
	"PL0/ZMOVIE/R703L.BIN",
	"PL0/ZMOVIE/R704LE.BIN",
	"PL0/ZMOVIE/TITLELE.BIN",
	NULL
};

static const char *re2pcgame_claire_movies[] = {
	"PL1/ZMOVIE/OPN1STC.BIN",
	"PL1/ZMOVIE/OPN2NDC.BIN",
	"PL1/ZMOVIE/OPN2NDRC.BIN",
	"PL1/ZMOVIE/R108C.BIN",
	"PL1/ZMOVIE/R204C.BIN",
	"PL1/ZMOVIE/R408.BIN",
	"PL1/ZMOVIE/R700C.BIN",
	"PL1/ZMOVIE/R703C.BIN",
	"PL1/ZMOVIE/R704CE.BIN",
	"PL1/ZMOVIE/TITLECE.BIN",
	NULL
};

/*--- Variables ---*/

static re2_images_t *re2_images = NULL;
static int num_re2_images = 0;

static int game_player = 0;

static int game_lang = 'U';

/*--- Functions prototypes ---*/

static void re2pcgame_shutdown(void);

static void re2pcgame_loadbackground(void);

static int re2pcgame_init_images(const char *filename);
static int re2pcgame_load_image(int num_image);

static void re2pcgame_loadroom(void);
static int re2pcgame_loadroom_rdt(const char *filename);

render_skel_t *re2pcgame_load_model(int num_model);

static void load_font(void);
static void get_char(int ascii, int *x, int *y, int *w, int *h);

static void get_model_name(char name[32]);

/*--- Functions ---*/

void game_re2pcgame_init(game_t *this)
{
	if (!re2pcgame_init_images(re2pcgame_bg_archive)) {
		fprintf(stderr, "Error reading background archive infos\n");
	}

	this->priv_shutdown = re2pcgame_shutdown;

	this->room.priv_load_background = re2pcgame_loadbackground;
	this->room.priv_load = re2pcgame_loadroom;

	switch(this->minor) {
		case GAME_RE2_PC_GAME_LEON:
			this->movies_list = (char **) re2pcgame_leon_movies;
			if (game_file_exists("PL0/RDP/ROOM1000.RDT")) {
				game_lang = 'P';
			}
			if (game_file_exists("PL0/RDS/ROOM1000.RDT")) {
				game_lang = 'S';
			}
			if (game_file_exists("PL0/RDF/ROOM1000.RDT")) {
				game_lang = 'F';
			}
			if (game_file_exists("PL0/RDT/ROOM1000.RDT")) {
				game_lang = 'T';
			}
			break;
		case GAME_RE2_PC_GAME_CLAIRE:
			this->movies_list = (char **) re2pcgame_claire_movies;
			game_player = 1;
			if (game_file_exists("PL1/RDP/ROOM1001.RDT")) {
				game_lang = 'P';
			}
			if (game_file_exists("PL1/RDS/ROOM1001.RDT")) {
				game_lang = 'S';
			}
			if (game_file_exists("PL1/RDF/ROOM1001.RDT")) {
				game_lang = 'F';
			}
			if (game_file_exists("PL1/RDT/ROOM1001.RDT")) {
				game_lang = 'T';
			}
			break;
	}

	this->player.load_model = re2pcgame_load_model;
	this->player.get_model_name = get_model_name;

	this->load_font = load_font;
	this->get_char = get_char;
}

static void re2pcgame_shutdown(void)
{
	if (re2_images) {
		free(re2_images);
		re2_images = NULL;
	}
}

static void re2pcgame_loadbackground(void)
{
	int num_image;
	
	num_image = (game.num_stage-1)*512;
	num_image += game.num_room*16;
	num_image += game.num_camera;
	if (num_image>=num_re2_images) {
		num_image = num_re2_images-1;
	}

	logMsg(1, "adt: Start loading stage %d, room %d, camera %d ...\n",
		game.num_stage, game.num_room, game.num_camera);

	logMsg(1, "adt: %s loading stage %d, room %d, camera %d ...\n",
		re2pcgame_load_image(num_image) ? "Done" : "Failed",
		game.num_stage, game.num_room, game.num_camera);
}

static int re2pcgame_init_images(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;

	src = FS_makeRWops(filename);
	if (src) {
		Uint32 archive_length, first_offset;

		/* Read archive length */
		SDL_RWseek(src, 0, RW_SEEK_END);
		archive_length = SDL_RWtell(src);
		SDL_RWseek(src, 0, RW_SEEK_SET);

		first_offset = SDL_ReadLE32(src);
		num_re2_images = first_offset >> 2;

		re2_images = (re2_images_t *) malloc(num_re2_images * sizeof(re2_images_t));
		if (re2_images) {
			int i;

			/* Fill offsets */
			re2_images[0].offset = first_offset;
			for (i=1; i<num_re2_images; i++) {
				re2_images[i].offset = SDL_ReadLE32(src);
			}

			/* Calc length */
			for (i=0;i<num_re2_images-1;i++) {
				re2_images[i].length = re2_images[i+1].offset -
					re2_images[i].offset;
			}
			re2_images[num_re2_images-1].length = archive_length -
				re2_images[num_re2_images-1].offset;

			retval = 1;
		}

		SDL_RWclose(src);
	}

	return retval;
}

static int re2pcgame_load_image(int num_image)
{
	SDL_RWops *src;
	int retval = 0;

	if (!re2_images[num_image].length) {
		return 0;
	}
	
	src = FS_makeRWops(re2pcgame_bg_archive);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		SDL_RWseek(src, re2_images[num_image].offset, RW_SEEK_SET);

		adt_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			SDL_Surface *image = adt_surface((Uint16 *) dstBuffer, 1);
			if (image) {
				game.room.background = render.createTexture(RENDER_TEXTURE_CACHEABLE);
				if (game.room.background) {
					game.room.background->load_from_surf(game.room.background, image);
					retval = 1;
				}
				SDL_FreeSurface(image);
			}
			free(dstBuffer);
		}
		SDL_RWclose(src);
	}

	return retval;
}

static void re2pcgame_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re2pcgame_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcgame_room, game_player, game_lang, game.num_stage,
		game.num_room, game_player);

	logMsg(1, "rdt: Start loading %s ...\n", filepath);

	logMsg(1, "rdt: %s loading %s ...\n",
		re2pcgame_loadroom_rdt(filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int re2pcgame_loadroom_rdt(const char *filename)
{
	PHYSFS_sint64 length;
	void *file;

	file = FS_Load(filename, &length);
	if (!file) {
		return 0;
	}

	game.room.file = file;
	game.room.file_length = length;

	room_rdt2_init(&game.room);

	return 1;
}

render_skel_t *re2pcgame_load_model(int num_model)
{
	char *filepath;
	render_skel_t *model = NULL;
	void *emd, *tim;
	PHYSFS_sint64 emd_length, tim_length;
	int i;

	if (num_model>=MAX_MODELS) {
		num_model = MAX_MODELS-1;
	}
	num_model = map_models[num_model];

	filepath = malloc(strlen(re2pcgame_model)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re2pcgame_model,
		game_player, game_player, game_player,
		num_model, "EMD");
	for (i=0; i<strlen(filepath); i++) {
		filepath[i] = toupper(filepath[i]);
	}

	logMsg(1, "emd: Start loading model %s ...\n", filepath);

	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		sprintf(filepath, re2pcgame_model,
			game_player, game_player, game_player,
			num_model, "TIM");
		for (i=0; i<strlen(filepath); i++) {
			filepath[i] = toupper(filepath[i]);
		}
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

static void load_font(void)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
	const char *filename = re2pcgame_font;

	filepath = malloc(strlen(filename)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, filename, game_lang);

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

static void get_char(int ascii, int *x, int *y, int *w, int *h)
{
	*x = *y = 0;
	*w = 8;
	*h = 10;

	if ((ascii<=32) || (ascii>=96+27)) {
		return;
	}

	ascii -= 32;
	*x = (ascii & 31)<<3;
	*y = (ascii>>5)*10;
}

static void get_model_name(char name[32])
{
	int num_model = game.player.num_model;

	if (num_model>MAX_MODELS-1) {
		num_model = MAX_MODELS-1;
	}

	sprintf(name, "em%d%02x.emd", game_player, map_models[num_model]);
}
