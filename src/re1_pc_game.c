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

#include <SDL.h>

#include "filesystem.h"
#include "state.h"
#include "depack_pak.h"
#include "re1_pc_game.h"
#include "parameters.h"
#include "video.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- Constant ---*/

static const char *re1pcgame_bg = "horr/usa/stage%d/rc%d%02x%d.pak";
static const char *re1pcgame_room = "horr/usa/stage%d/room%d%02x0.rdt";

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

/*--- Functions prototypes ---*/

static void re1pcgame_shutdown(void);

static void re1pcgame_loadbackground(void);
static int re1pcgame_load_pak_bg(const char *filename);

static void re1pcgame_loadroom(void);
static int re1pcgame_loadroom_rdt(const char *filename);

/*--- Functions ---*/

void re1pcgame_init(state_t *game_state)
{
	game_state->load_background = re1pcgame_loadbackground;
	game_state->load_room = re1pcgame_loadroom;
	game_state->shutdown = re1pcgame_shutdown;

	game_state->movies_list = (char **) re1pcgame_movies;
}

void re1pcgame_shutdown(void)
{
}

void re1pcgame_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re1pcgame_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_bg, game_state.stage, game_state.stage, game_state.room, game_state.camera);

	printf("pak: Loading %s ... %s\n", filepath,
		re1pcgame_load_pak_bg(filepath) ? "done" : "failed"
	);

	free(filepath);
}

int re1pcgame_load_pak_bg(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		pak_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			SDL_RWops *tim_src;
			/*game_state.num_cameras = 8;*/
			
			tim_src = SDL_RWFromMem(dstBuffer, dstBufLen);
			if (tim_src) {
				SDL_Surface *image;

				/*game_state.background_surf = background_tim_load(tim_src);
				if (game_state.background_surf) {
					retval = 1;
				}*/

				image = background_tim_load(tim_src);
				if (image) {
					game_state.back_surf = video.createSurfaceSu(image);
					if (game_state.back_surf) {
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

static void re1pcgame_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re1pcgame_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1pcgame_room, game_state.stage, game_state.stage, game_state.room);

	printf("rdt: Loading %s ... %s\n", filepath,
		re1pcgame_loadroom_rdt(filepath) ? "done" : "failed"
	);

	free(filepath);
}

static int re1pcgame_loadroom_rdt(const char *filename)
{
	PHYSFS_sint64 length;
	Uint8 *rdt_header;

	game_state.num_cameras = 8;

	game_state.room_file = FS_Load(filename, &length);
	if (!game_state.room_file) {
		return 0;
	}

	rdt_header = (Uint8 *) game_state.room_file;
	game_state.num_cameras = rdt_header[1];

	return 1;
}

typedef struct {
	long camera_from_x;
	long camera_from_y;
	long camera_from_z;
	long camera_to_x;
	long camera_to_y;
	long camera_to_z;
	long unknown[5];
} rdt_camera_pos_t;

void re1pcgame_get_camera(long *camera_pos)
{
	rdt_camera_pos_t *cam_array;
	
	cam_array = (rdt_camera_pos_t *) &((Uint8 *)game_state.room_file)[0x9c];

	camera_pos[0] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_x);
	camera_pos[1] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_y);
	camera_pos[2] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_z);
	camera_pos[3] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_x);
	camera_pos[4] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_y);
	camera_pos[5] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_z);
}
