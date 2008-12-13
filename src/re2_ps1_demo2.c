/*
	RE2
	PS1
	Demo2

	Copyright (C) 2008	Patrice Mandin

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
#include "re2_ps1_demo2.h"
#include "background_bss.h"
#include "parameters.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Types ---*/

/*--- Constant ---*/

static const char *re2ps1_bg1 = "res2/zcommon/bss/room1%02x.bss";
static const char *re2ps1_bg2 = "res2/zcommon/bss2/room2%02x.bss";
static const char *re2ps1_room1 = "res2/zpl%d/rdt/room1%02x0.rdt";
static const char *re2ps1_room2 = "res2/zpl%d/rdt2/room2%02x0.rdt";

static const char *re2ps1demo_movies[] = {
	"res2/zz/virgin.str",
	NULL
};

/*--- Variables ---*/

static int game_player = 0;

/*--- Functions prototypes ---*/

static void re2ps1_shutdown(void);

static void re2ps1_loadbackground(void);

static void re2ps1_loadroom(void);
static int re2ps1_loadroom_rdt(const char *filename);

/*--- Functions ---*/

void re2ps1demo2_init(state_t *game_state)
{
	game_state->load_background = re2ps1_loadbackground;
	game_state->load_room = re2ps1_loadroom;
	game_state->shutdown = re2ps1_shutdown;

	game_state->movies_list = (char **) re2ps1demo_movies;
}

static void re2ps1_shutdown(void)
{
}

static void re2ps1_loadbackground(void)
{
	char *filepath;
	const char *re2ps1_bg = ((game_state.stage == 1) ? re2ps1_bg1 : re2ps1_bg2);

	filepath = malloc(strlen(re2ps1_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2ps1_bg, game_state.room);

	logMsg(1, "bss: Loading %s ... ", filepath);
	logMsg(1, "%s\n", background_bss_load(filepath, CHUNK_SIZE) ? "done" : "failed");

	free(filepath);
}

static void re2ps1_loadroom(void)
{
	char *filepath;
	const char *re2ps1_room = ((game_state.stage == 1) ? re2ps1_room1 : re2ps1_room2);

	filepath = malloc(strlen(re2ps1_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2ps1_room, game_player, game_state.room);

	logMsg(1, "rdt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re2ps1_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re2ps1_loadroom_rdt(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	PHYSFS_sint64 length;
	Uint8 *rdt_header;
	
	game_state.num_cameras = 16;

	game_state.room_file = FS_Load(filename, &length);
	if (!game_state.room_file) {
		return retval;
	}

	rdt_header = (Uint8 *) game_state.room_file;
	game_state.num_cameras = rdt_header[1];

	retval = 1;
	return retval;
}

typedef struct {
	unsigned short unk0;
	unsigned short const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	long camera_from_x;
	long camera_from_y;
	long camera_from_z;
	long camera_to_x;
	long camera_to_y;
	long camera_to_z;
	unsigned long offset;
} rdt_camera_pos_t;

void re2ps1demo2_get_camera(long *camera_pos)
{
	Uint32 *cams_offset, offset;
	rdt_camera_pos_t *cam_array;
	
	cams_offset = (Uint32 *) ( &((Uint8 *)game_state.room_file)[8+7*4]);
	offset = SDL_SwapLE32(*cams_offset);
	cam_array = (rdt_camera_pos_t *) &((Uint8 *)game_state.room_file)[offset];

	camera_pos[0] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_x);
	camera_pos[1] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_y);
	camera_pos[2] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_z);
	camera_pos[3] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_x);
	camera_pos[4] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_y);
	camera_pos[5] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_z);
}
