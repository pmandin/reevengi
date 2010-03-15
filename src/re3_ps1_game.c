/*
	RE3
	PS1
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
#include "re3_ps1_game.h"
#include "background_bss.h"
#include "parameters.h"
#include "log.h"
#include "room_rdt2.h"
#include "render.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Types ---*/

typedef struct {
	Uint32 length;
	Uint32 count;
} ard_header_t;

typedef struct {
	Uint32 length;
	Uint32 unknown;
} ard_object_t;

/*--- Constant ---*/

static const char *re3ps1game_bg = "cd_data/stage%d/r%d%02x.bss";
static const char *re3ps1game_room = "cd_data/stage%d/r%d%02x.ard";
static const char *re3ps1game_font = "cd_data/etc/sele_ob%c.tim";

static const char *re3ps1game_movies[] = {
	"cd_data/zmovie/enda.str",
	"cd_data/zmovie/endb.str",
	"cd_data/zmovie/ins01.str",
	"cd_data/zmovie/ins02.str",
	"cd_data/zmovie/ins03.str",
	"cd_data/zmovie/ins04.str",
	"cd_data/zmovie/ins05.str",
	"cd_data/zmovie/ins06.str",
	"cd_data/zmovie/ins07.str",
	"cd_data/zmovie/ins08.str",
	"cd_data/zmovie/ins09.str",
	"cd_data/zmovie/opn.str",
	"cd_data/zmovie/roopne.str",
	NULL
};

/*--- Variables ---*/

static int game_lang = 'f';

/*--- Functions prototypes ---*/

static void re3ps1game_shutdown(void);

static void re3ps1game_loadbackground(void);

static void re3ps1game_loadroom(void);
static int re3ps1game_loadroom_ard(const char *filename);

static void load_font(void);
static void get_char_pos(int ascii, int *x, int *y);

/*--- Functions ---*/

void re3ps1game_init(state_t *game_state)
{
	game_state->priv_load_background = re3ps1game_loadbackground;
	game_state->priv_load_room = re3ps1game_loadroom;
	game_state->priv_shutdown = re3ps1game_shutdown;

	game_state->movies_list = (char **) re3ps1game_movies;

	game_state->load_font = load_font;
	game_state->get_char_pos = get_char_pos;
}

static void re3ps1game_shutdown(void)
{
}

static void re3ps1game_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_bg, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "bss: Start loading %s ...\n", filepath);

	logMsg(1, "bss: %s loading %s ...\n",
		background_bss_load(filepath, CHUNK_SIZE, 0) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static void re3ps1game_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_room, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "ard: Start loading %s ...\n", filepath);

	logMsg(1, "ard: %s loading %s ...\n",
		re3ps1game_loadroom_ard(filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int re3ps1game_loadroom_ard(const char *filename)
{
	PHYSFS_sint64 length;
	Uint8 *ard_file;
	ard_object_t *ard_object;
	int i, count;
	Uint32 offset, len;
	void *file;

	ard_file = (Uint8 *) FS_Load(filename, &length);
	if (!ard_file) {
		return 0;
	}

	count = ((ard_header_t *) ard_file)->count;
	count = SDL_SwapLE32(count);
/*
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		logMsg(1, "ard: object %d at offset 0x%08x\n", i,offset);
		len = SDL_SwapLE32(ard_object->length);

		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}
*/
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		len = SDL_SwapLE32(ard_object->length);
		if (i==8) {
			/* Stop on embedded RDT file */
			break;
		}
		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}

	file = malloc(len);
	if (!file) {
		free(ard_file);
		return 0;
	}

	logMsg(3, "ard: Loading embedded RDT file from offset 0x%08x\n", offset);
	memcpy(file, &ard_file[offset], len);

	game_state.room = room_create(file, len);
	if (!game_state.room) {
		free(file);
		free(ard_file);
		return 0;
	}

	room_rdt2_init(game_state.room);

	free(ard_file);
	return 1;
}

static void load_font(void)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
	const char *filename = re3ps1game_font;

	filepath = malloc(strlen(filename)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, filename, game_lang);

	logMsg(1, "Loading font from %s...\n", filepath);

	font_file = FS_Load(filepath, &length);
	if (font_file) {
		game_state.font = render.createTexture(0);
		if (game_state.font) {
			game_state.font->load_from_tim(game_state.font, font_file);
			retval = 1;
		}

		free(font_file);
	}

	logMsg(1, "Loading font from %s... %s\n", filepath, retval ? "Done" : "Failed");

	free(filepath);
}

static void get_char_pos(int ascii, int *x, int *y)
{
}
