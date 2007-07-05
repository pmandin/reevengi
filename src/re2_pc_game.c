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
#include "state.h"
#include "re2_pc_game.h"
#include "depack_adt.h"
#include "parameters.h"

/*--- Defines ---*/

/*--- Types ---*/

typedef struct {
	long offset;
	long length;
} re2_images_t;

/*--- Constant ---*/

static const char *re2pcgame_bg_archive = "COMMON/BIN/ROOMCUT.BIN";

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

/*--- Functions prototypes ---*/

static int re2pcgame_init_images(const char *filename);
static int re2pcgame_load_image(int num_image);

/*--- Functions ---*/

void re2pcgame_init(state_t *game_state)
{
	if (!re2pcgame_init_images(re2pcgame_bg_archive)) {
		fprintf(stderr, "Error reading background archive infos\n");
	}

	game_state->load_background = re2pcgame_loadbackground;
	game_state->shutdown = re2pcgame_shutdown;
}

void re2pcgame_shutdown(void)
{
	if (re2_images) {
		free(re2_images);
		re2_images = NULL;
	}
}

void re2pcgame_loadbackground(void)
{
	int num_image = (game_state.stage-1)*512;
	num_image += game_state.room*16;
	num_image += game_state.camera;
	if (num_image>=num_re2_images) {
		num_image = num_re2_images-1;
	}

	printf("adt: Loading stage %d, room %d, camera %d ... %s\n",
		game_state.stage, game_state.room, game_state.camera,
		re2pcgame_load_image(num_image) ? "done" : "failed"
	);
}

static int re2pcgame_init_images(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;

	src = FS_makeRWops(filename);
	if (src) {
		/* Read archive length */
		SDL_RWseek(src, 0, RW_SEEK_END);
		Uint32 archive_length = SDL_RWtell(src);
		SDL_RWseek(src, 0, RW_SEEK_SET);

		Uint32 first_offset = SDL_ReadLE32(src);
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
			game_state.num_cameras = 12;

			game_state.background_surf = adt_surface((Uint16 *) dstBuffer);
			if (game_state.background_surf) {
				retval = 1;
			}

			free(dstBuffer);
		}

		SDL_RWclose(src);
	}

	return retval;
}
