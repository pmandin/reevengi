/*
	RE3
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#ifdef ENABLE_SDLIMAGE
#include <SDL_image.h>
#endif

#include "file.h"
#include "state.h"
#include "re3_pc_demo.h"
#include "parameters.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- Variables ---*/

static const char *re3pcdemo_bg = "data_a/bss/r%d%02x%02x.jpg";

static char *finalpath = NULL;

/*--- Functions prototypes ---*/

static int re3pcdemo_load_jpg_bg(const char *filename);

/*--- Functions ---*/

void re3pcdemo_init(state_t *game_state)
{
	game_state->load_background = re3pcdemo_loadbackground;
	game_state->shutdown = re3pcdemo_shutdown;
}

void re3pcdemo_shutdown(void)
{
	if (finalpath) {
		free(finalpath);
		finalpath=NULL;
	}
}

void re3pcdemo_loadbackground(void)
{
	char *filepath;
	int length;

	if (!finalpath) {
		finalpath = malloc(strlen(basedir)+strlen(re3pcdemo_bg)+2);
		if (!finalpath) {
			fprintf(stderr, "Can not allocate mem for final path\n");
			return;
		}
		sprintf(finalpath, "%s/%s", basedir, re3pcdemo_bg);	
	}

	filepath = malloc(strlen(finalpath)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, finalpath, game_state.stage, game_state.stage, game_state.room, game_state.camera);

	if (re3pcdemo_load_jpg_bg(filepath)) {
		printf("jpg: Loaded %s\n", filepath);
	} else {
		fprintf(stderr, "jpg: Can not load %s\n", filepath);
	}

	free(filepath);
}

int re3pcdemo_load_jpg_bg(const char *filename)
{
#ifdef ENABLE_SDLIMAGE
	SDL_RWops *src;
	int retval = 0;
	
	src = SDL_RWFromFile(filename, "rb");
	if (src) {
		game_state.num_cameras = 0x1c;

		game_state.background_surf = IMG_Load_RW(src, 0);
		if (game_state.background_surf) {
			retval = 1;
		}

		SDL_FreeRW(src);
	}

	return retval;
#else
	return 0;
#endif
}
