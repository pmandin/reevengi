/*
	RE1
	PS1
	Demo, Game

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
#include "re1_ps1.h"
#include "background_bss.h"
#include "parameters.h"
#include "model_emd.h"

/*--- Defines ---*/

#define CHUNK_SIZE 32768

/*--- Types ---*/

/*--- Constant ---*/

static const char *re1ps1_bg = "psx/stage%d/room%d%02x.bss";
static const char *re1ps1_room = "psx/stage%d/room%d%02x0.rdt";
static const char *re1ps1_model = "psx/enemy/char10.emd";

static const char *re1ps1demo_movies[] = {
	"psx/movie/capcom.str",
	"psx/movie/dm1.str",
	"psx/movie/dm2.str",
	"psx/movie/dm3.str",
	"psx/movie/oj.str",
	"psx/movie/pj.str",
	NULL
};

static const char *re1ps1game_movies[] = {
	"psx/movie/capcom.str",
	"psx/movie/dm1.str",
	"psx/movie/dm2.str",
	"psx/movie/dm3.str",
	"psx/movie/dm4.str",
	"psx/movie/dm6.str",
	"psx/movie/dm7.str",
	"psx/movie/dm8.str",
	"psx/movie/dmb.str",
	"psx/movie/dmc.str",
	"psx/movie/dmd.str",
	"psx/movie/dme.str",
	"psx/movie/dmf.str",
	"psx/movie/ed1.str",
	"psx/movie/ed2.str",
	"psx/movie/ed3.str",
	"psx/movie/ed4.str",
	"psx/movie/ed5.str",
	"psx/movie/ed6.str",
	"psx/movie/ed7.str",
	"psx/movie/ed8.str",
	"psx/movie/oj.str",
	"psx/movie/pj.str",
	"psx/movie/stfc.str",
	"psx/movie/stfj.str",
	NULL
};

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void re1ps1_shutdown(void);

static void re1ps1_loadbackground(void);

static void re1ps1_loadroom(void);
static int re1ps1_loadroom_rdt(const char *filename);

/*--- Functions ---*/

void re1ps1_init(state_t *game_state)
{
	game_state->load_background = re1ps1_loadbackground;
	game_state->load_room = re1ps1_loadroom;
	game_state->shutdown = re1ps1_shutdown;

	if (game_state->version == GAME_RE1_PS1_DEMO) {
		game_state->movies_list = (char **) re1ps1demo_movies;
	} else {
		game_state->movies_list = (char **) re1ps1game_movies;
	}

	game_state->load_model = model_emd_load;
	game_state->close_model = model_emd_close;
	game_state->draw_model = model_emd_draw;
	game_state->model = re1ps1_model;
}

static void re1ps1_shutdown(void)
{
}

static void re1ps1_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re1ps1_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_bg, game_state.stage, game_state.stage, game_state.room);

	logMsg(1, "bss: Loading %s ... ", filepath);
	logMsg(1, "%s\n", background_bss_load(filepath, CHUNK_SIZE) ? "done" : "failed");

	free(filepath);
}

static void re1ps1_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re1ps1_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_room, game_state.stage, game_state.stage, game_state.room);

	logMsg(1, "rdt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re1ps1_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re1ps1_loadroom_rdt(const char *filename)
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

void re1ps1_get_camera(long *camera_pos)
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
