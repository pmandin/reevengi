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
#include "log.h"

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

typedef struct {
	Uint16 unk0;
	Uint16 const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	Sint32 camera_from_x;
	Sint32 camera_from_y;
	Sint32 camera_from_z;
	Sint32 camera_to_x;
	Sint32 camera_to_y;
	Sint32 camera_to_z;
	Uint32 offset;
} rdt_camera_pos_t;

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

static void re3ps1game_getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

/*--- Functions ---*/

void re3ps1game_init(state_t *game_state)
{
	game_state->priv_load_background = re3ps1game_loadbackground;
	game_state->priv_load_room = re3ps1game_loadroom;
	game_state->priv_shutdown = re3ps1game_shutdown;

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
	sprintf(filepath, re3ps1game_bg, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "bss: Loading %s ... ", filepath);
	logMsg(1, "%s\n", background_bss_load(filepath, CHUNK_SIZE) ? "done" : "failed");

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

	logMsg(1, "ard: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re3ps1game_loadroom_ard(filepath) ? "done" : "failed");

	free(filepath);
}

static int re3ps1game_loadroom_ard(const char *filename)
{
	PHYSFS_sint64 length;
	Uint8 *rdt_header, *ard_file;
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

	memcpy(file, &ard_file[offset], len);

	game_state.room = room_create(file);
	if (!game_state.room) {
		free(file);
		free(ard_file);
		return 0;
	}

	rdt_header = (Uint8 *) file;
	game_state.room->num_cameras = rdt_header[1];

	game_state.room->getCamera = re3ps1game_getCamera;

	free(ard_file);
	return 1;
}

static void re3ps1game_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
	Uint32 *cams_offset, offset;
	rdt_camera_pos_t *cam_array;

	cams_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+7*4]);
	offset = SDL_SwapLE32(*cams_offset);
	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[offset];

	room_camera->from_x = SDL_SwapLE32(cam_array[num_camera].camera_from_x);
	room_camera->from_y = SDL_SwapLE32(cam_array[num_camera].camera_from_y);
	room_camera->from_z = SDL_SwapLE32(cam_array[num_camera].camera_from_z);
	room_camera->to_x = SDL_SwapLE32(cam_array[num_camera].camera_to_x);
	room_camera->to_y = SDL_SwapLE32(cam_array[num_camera].camera_to_y);
	room_camera->to_z = SDL_SwapLE32(cam_array[num_camera].camera_to_z);
}
