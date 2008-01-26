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

#include "filesystem.h"
#include "state.h"
#include "re3_ps1_game.h"
#include "background_bss.h"
#include "parameters.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Types ---*/

/*--- Constant ---*/

static const char *re3ps1game_bg = "cd_data/stage%d/r%d%02x.bss";
static const char *re3ps1game_room = "cd_data/stage%d/r%d%02x.ard";

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

/*--- Functions prototypes ---*/

static void re3ps1game_shutdown(void);

static void re3ps1game_loadbackground(void);

static void re3ps1game_loadroom(void);
static int re3ps1game_loadroom_ard(const char *filename);

/*--- Functions ---*/

void re3ps1game_init(state_t *game_state)
{
	game_state->load_background = re3ps1game_loadbackground;
	game_state->load_room = re3ps1game_loadroom;
	game_state->shutdown = re3ps1game_shutdown;

	game_state->movies_list = (char **) re3ps1game_movies;
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
	sprintf(filepath, re3ps1game_bg, game_state.stage, game_state.stage, game_state.room);

	printf("bss: Loading %s ... %s\n", filepath,
		background_bss_load(filepath, CHUNK_SIZE) ? "done" : "failed"
	);

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
	sprintf(filepath, re3ps1game_room, game_state.stage, game_state.stage, game_state.room);

	printf("ard: Loading %s ... %s\n", filepath,
		re3ps1game_loadroom_ard(filepath) ? "done" : "failed"
	);

	free(filepath);
}

typedef struct {
	unsigned long length;
	unsigned long count;
} ard_header_t;

typedef struct {
	unsigned long length;
	unsigned long unknown;
} ard_object_t;

static int re3ps1game_loadroom_ard(const char *filename)
{
	SDL_RWops *src;
	PHYSFS_sint64 length;
	Uint8 *rdt_header, *ard_file;
	ard_object_t *ard_object;
	int i, count;
	Uint32 offset, len;
	
	game_state.num_cameras = 0x1c;

	ard_file = (Uint8 *) FS_Load(filename, &length);
	if (!ard_file) {
		return 0;
	}

	count = ((ard_header_t *) ard_file)->count;
	count = SDL_SwapLE32(count);
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		if (i==8) {
			break;
		}
		len = ard_object->length;
		len = SDL_SwapLE32(len);
		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}

	game_state.room_file = malloc(len);
	if (!game_state.room_file) {
		free(ard_file);
		return 0;
	}

	memcpy(game_state.room_file, &ard_file[offset], len);

	rdt_header = (Uint8 *) game_state.room_file;
	game_state.num_cameras = rdt_header[1];

	free(ard_file);
	return 1;
}
