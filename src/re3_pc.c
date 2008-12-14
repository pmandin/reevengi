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
#include "video.h"
#include "model_emd3.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- Constant ---*/

static const char *re3pc_bg = "data_a/bss/r%d%02x%02x.jpg";
static const char *re3pc_room = "data_%c/rdt/r%d%02x.rdt";
static const char *re3pc_model = "room/emd/em10.emd";

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
		sprintf(rofsfile, rofs_dat, params.basedir, i);
		if (FS_AddArchive(rofsfile)==0) {
			continue;
		}
		/* Try with cap letter */
		if (game_state->version==GAME_RE3_PC_GAME) {
			sprintf(rofsfile, rofs_cap_dat, params.basedir, i);
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

	game_state->load_model = model_emd3_load;
	game_state->close_model = model_emd3_close;
	game_state->draw_model = model_emd3_draw;
	/*game_state->model = re3pc_model;*/
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

	logMsg(1, "jpg: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re3pc_load_jpg_bg(filepath) ? "done" : "failed");

	free(filepath);
}

int re3pc_load_jpg_bg(const char *filename)
{
#ifdef ENABLE_SDLIMAGE
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		SDL_Surface *image;
		/*game_state.num_cameras = 0x1c;*/

		/*game_state.background_surf = IMG_Load_RW(src, 0);
		if (game_state.background_surf) {
			retval = 1;
		}*/

		image = IMG_Load_RW(src, 0);
		if (image) {
			game_state.back_surf = video.createSurfaceSu(image);
			if (game_state.back_surf) {
				video.convertSurface(game_state.back_surf);
				retval = 1;
			}
			SDL_FreeSurface(image);
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

	logMsg(1, "rdt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re3pc_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re3pc_loadroom_rdt(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	PHYSFS_sint64 length;
	Uint8 *rdt_header;
	
	game_state.num_cameras = 0x1c;

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

void re3pc_get_camera(long *camera_pos)
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
