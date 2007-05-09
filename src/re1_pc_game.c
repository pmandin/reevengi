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

#include "file.h"
#include "state.h"
#include "depack_vlc.h"
#include "depack_mdec.h"
#include "re1_ps1_demo.h"
#include "parameters.h"

/*--- Defines ---*/

#define WIDTH 320
#define HEIGHT 240

/*--- Types ---*/

/*--- Variables ---*/

static const char *re1pcgame_bg = "usa/stage%d/rc%d%02x%d.pak";

static char *finalpath = NULL;

/*--- Functions prototypes ---*/

static void re1pcgame_load_pak_bg(const char *filename);

/*--- Functions ---*/

void re1pcgame_init(state_t *game_state)
{
	game_state->load_background = re1pcgame_loadbackground;
	game_state->shutdown = re1pcgame_shutdown;
}

void re1pcgame_shutdown(void)
{
	if (finalpath) {
		free(finalpath);
		finalpath=NULL;
	}
}

void re1pcgame_loadbackground(void)
{
	char *filepath;
	int length;

	if (!finalpath) {
		finalpath = malloc(strlen(basedir)+strlen(re1pcgame_bg)+2);
		if (!finalpath) {
			fprintf(stderr, "Can not allocate mem for final path\n");
			return;
		}
		sprintf(finalpath, "%s/%s", basedir, re1pcgame_bg);	
	}

	filepath = malloc(strlen(finalpath)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, finalpath, game_state.stage, game_state.stage, game_state.room, game_state.camera);

	re1pcgame_load_pak_bg(filepath);

	free(filepath);
}

void re1pcgame_load_pak_bg(const char *filename)
{
	SDL_RWops *src;
	
	src = SDL_RWFromFile(filename, "rb");
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		pak_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			printf("Loaded %s at 0x%08x, length %d, %d angles\n", filename, dstBuffer, dstBufLen, game_state.num_cameras);

			game_state.background = dstBuffer;
			game_state.surface_bg = NULL;
			/*if (dstBufLen == 320*256*2) {
				game_state.surface_bg = adt_surface((Uint16 *) game_state.background);
			}*/
		}

		SDL_FreeRW(src);
	} else {
		printf("Can not load %s\n", filename);
	}
}