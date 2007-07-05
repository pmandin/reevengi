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
#include "state.h"
#include "re2_pc_demo.h"
#include "depack_adt.h"
#include "parameters.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- Constant ---*/

static const char *re2pcdemo_bg = "common/stage%d/rc%d%02x%1x.adt";

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static int re2pcdemo_load_adt_bg(const char *filename);

/*--- Functions ---*/

void re2pcdemo_init(state_t *game_state)
{
	game_state->load_background = re2pcdemo_loadbackground;
	game_state->shutdown = re2pcdemo_shutdown;
}

void re2pcdemo_shutdown(void)
{
}

void re2pcdemo_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re2pcdemo_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcdemo_bg, game_state.stage, game_state.stage, game_state.room, game_state.camera);

	printf("adt: Loading %s ... %s\n", filepath,
		re2pcdemo_load_adt_bg(filepath) ? "done" : "failed"
	);

	free(filepath);
}

int re2pcdemo_load_adt_bg(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		adt_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			game_state.num_cameras = 12;

			if (dstBufLen == 320*256*2) {
				game_state.background_surf = adt_surface((Uint16 *) dstBuffer);
				if (game_state.background_surf) {
					retval = 1;
				}
			}

			free(dstBuffer);
		}

		SDL_RWclose(src);
	}

	return retval;
}
