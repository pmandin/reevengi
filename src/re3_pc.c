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

/*--- Defines ---*/

/*--- Types ---*/

/*--- Constant ---*/

static const char *re3pc_bg = "data_a/bss/r%d%02x%02x.jpg";
static const char *re3pc_room = "data_%c/rdt/r%d%02x.rdt";
static const char *rofs_dat = "%s/rofs%d.dat";
static const char *rofs_cap_dat = "%s/Rofs%d.dat";

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

/*--- Functions ---*/

void re3pc_init(state_t *game_state)
{
	int i;
	char rofsfile[1024];

	for (i=1;i<16;i++) {
		sprintf(rofsfile, rofs_dat, basedir, i);
		if (FS_AddArchive(rofsfile)==0) {
			continue;
		}
		/* Try with cap letter */
		if (game_state->version==GAME_RE3_PC_GAME) {
			sprintf(rofsfile, rofs_cap_dat, basedir, i);
			FS_AddArchive(rofsfile);
		}
	}

	game_state->load_background = re3pc_loadbackground;
	game_state->load_room = re3pc_loadroom;
	game_state->shutdown = re3pc_shutdown;

	switch(game_state->version) {
		case GAME_RE3_PC_DEMO:
			game_state->movies_list = (char **) re3pcdemo_movies;
			break;
		case GAME_RE3_PC_GAME:
			game_state->movies_list = (char **) re3pcgame_movies;
			game_lang = 'f';
			break;
	}
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
	sprintf(filepath, re3pc_bg, game_state.stage, game_state.room, game_state.camera);

	printf("jpg: Loading %s ... %s\n", filepath,
		re3pc_load_jpg_bg(filepath) ? "done" : "failed"
	);

	free(filepath);
}

int re3pc_load_jpg_bg(const char *filename)
{
#ifdef ENABLE_SDLIMAGE
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		/*game_state.num_cameras = 0x1c;*/

		game_state.background_surf = IMG_Load_RW(src, 0);
		if (game_state.background_surf) {
			retval = 1;
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
	sprintf(filepath, re3pc_room, game_lang, game_state.stage, game_state.room);

	printf("rdt: Loading %s ... %s\n", filepath,
		re3pc_loadroom_rdt(filepath) ? "done" : "failed"
	);

	free(filepath);
}

static int re3pc_loadroom_rdt(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	
	game_state.num_cameras = 0x1c;

	src = FS_makeRWops(filename);
	if (src) {
		/* Load header */
		Uint8 rdt_header[8];
		if (SDL_RWread( src, rdt_header, 8, 1 )==1) {
			game_state.num_cameras = rdt_header[1];
		}

		retval = 1;

		SDL_RWclose(src);
	}

	return retval;
}
