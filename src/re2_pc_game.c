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
#include "model_emd2.h"
#include "log.h"

/*--- Defines ---*/

#define MAX_MODELS	58

/*--- Types ---*/

typedef struct {
	long offset;
	long length;
} re2_images_t;

/*--- Constant ---*/

static const char *re2pcgame_bg_archive = "COMMON/BIN/ROOMCUT.BIN";
static const char *re2pcgame_room = "PL%d/RDF/ROOM%d%02x0.RDT";
static const char *re2pcgame_model = "PL%d/EMD%d/EM%d%02x.%s";

static const int map_models[MAX_MODELS]={
	0x10,	0x11,	0x12,	0x13,	0x15,	0x16,	0x17,	0x18,
	0x1e,	0x1f,	0x20,	0x21,	0x22,	0x23,	0x24,	0x25,
	0x26,	0x27,	0x28,	0x29,	0x2a,	0x2b,	0x2c,	0x2d,
	0x2e,	0x2f,	0x30,	0x31,	0x33,	0x34,	0x36,	0x37,
	0x38,	0x39,	0x3a,	0x3b,	0x3e,	0x3f,	0x40,	0x41,
	0x42,	0x43,	0x44,	0x45,	0x46,	0x47,	0x48,	0x49,
	0x4a,	0x4b,	0x4f,	0x50,	0x51,	0x54,	0x55,	0x58,
	0x59,	0x5a
};

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

static int game_player = 0;

/*--- Functions prototypes ---*/

static void re2pcgame_shutdown(void);

static void re2pcgame_loadbackground(void);

static int re2pcgame_init_images(const char *filename);
static int re2pcgame_load_image(int num_image);

static void re2pcgame_loadroom(void);
static int re2pcgame_loadroom_rdt(const char *filename);

model_t *re2pcgame_load_model(int num_model);

/*--- Functions ---*/

void re2pcgame_init(state_t *game_state)
{
	if (!re2pcgame_init_images(re2pcgame_bg_archive)) {
		fprintf(stderr, "Error reading background archive infos\n");
	}

	game_state->load_background = re2pcgame_loadbackground;
	game_state->load_room = re2pcgame_loadroom;
	game_state->shutdown = re2pcgame_shutdown;

	if (game_state->version == GAME_RE2_PC_GAME_LEON) {
		game_state->movies_list = (char **) re2pcgame_leon_movies;
	} else {
		game_state->movies_list = (char **) re2pcgame_claire_movies;
		game_player = 1;
	}

	game_state->load_model = re2pcgame_load_model;
}

static void re2pcgame_shutdown(void)
{
	if (re2_images) {
		free(re2_images);
		re2_images = NULL;
	}
}

static void re2pcgame_loadbackground(void)
{
	int num_image = (game_state.stage-1)*512;
	num_image += game_state.room*16;
	num_image += game_state.camera;
	if (num_image>=num_re2_images) {
		num_image = num_re2_images-1;
	}

	logMsg(1, "adt: Loading stage %d, room %d, camera %d ... ",
		game_state.stage, game_state.room, game_state.camera);
	logMsg(1, "%s\n", re2pcgame_load_image(num_image) ? "done" : "failed");
}

static int re2pcgame_init_images(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;

	src = FS_makeRWops(filename);
	if (src) {
		Uint32 archive_length, first_offset;

		/* Read archive length */
		SDL_RWseek(src, 0, RW_SEEK_END);
		archive_length = SDL_RWtell(src);
		SDL_RWseek(src, 0, RW_SEEK_SET);

		first_offset = SDL_ReadLE32(src);
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
			SDL_Surface *image;

			/*game_state.num_cameras = 16;*/

			image = adt_surface((Uint16 *) dstBuffer);
			if (image) {
				game_state.back_surf = video.createSurfaceSu(image);
				if (game_state.back_surf) {
					video.convertSurface(game_state.back_surf);
					retval = 1;
				}
				SDL_FreeSurface(image);
			}

			free(dstBuffer);
		}

		SDL_RWclose(src);
	}

	return retval;
}

static void re2pcgame_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re2pcgame_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcgame_room, game_player, game_state.stage, game_state.room);

	logMsg(1, "rdt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re2pcgame_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re2pcgame_loadroom_rdt(const char *filename)
{
	PHYSFS_sint64 length;
	Uint8 *rdt_header;
	
	game_state.num_cameras = 16;

	game_state.room_file = FS_Load(filename, &length);
	if (!game_state.room_file) {
		return 0;
	}

	rdt_header = (Uint8 *) game_state.room_file;
	game_state.num_cameras = rdt_header[1];

	return 1;
}

model_t *re2pcgame_load_model(int num_model)
{
	char *filepath;
	model_t *model = NULL;
	void *emd, *tim;
	PHYSFS_sint64 emd_length, tim_length;
	int i;

	if (num_model>=MAX_MODELS) {
		num_model = MAX_MODELS-1;
	}
	num_model = map_models[num_model];

	filepath = malloc(strlen(re2pcgame_model)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re2pcgame_model,
		game_player, game_player, game_player,
		num_model, "EMD");
	for (i=0; i<strlen(filepath); i++) {
		filepath[i] = toupper(filepath[i]);
	}

	logMsg(1, "Loading model %s...", filepath);
	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		sprintf(filepath, re2pcgame_model,
			game_player, game_player, game_player,
			num_model, "TIM");
		for (i=0; i<strlen(filepath); i++) {
			filepath[i] = toupper(filepath[i]);
		}
		tim = FS_Load(filepath, &tim_length);
		if (tim) {
			model = model_emd2_load(emd, tim, emd_length, tim_length);
		} else {
			free(emd);
		}
	}	
	logMsg(1, "%s\n", model ? "done" : "failed");

	free(filepath);
	return model;
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

void re2pcgame_get_camera(long *camera_pos)
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

typedef struct {
	unsigned short const0; /* 0xff01 */
	unsigned char cam0;
	unsigned char cam1;
	short x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	short x2,y2;
	short x3,y3;
	short x4,y4;
} rdt_camera_switch_t;

int re2pcgame_get_num_camswitch(void)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int num_switches = 0;

	camswitch_offset = (Uint32 *) ( &((Uint8 *)game_state.room_file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *)game_state.room_file)[offset];

	while ((*(Uint32 *) camswitch_array) != 0xffffffff) {
		/*printf("cam switch %d at offset 0x%08x\n", num_switches, offset);*/
		offset += sizeof(rdt_camera_switch_t);
		camswitch_array = (rdt_camera_switch_t *) &((Uint8 *)game_state.room_file)[offset];
		num_switches++;
	}

	return num_switches;
}

int re2pcgame_get_camswitch(int num, short *switch_pos)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;

	camswitch_offset = (Uint32 *) ( &((Uint8 *)game_state.room_file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *)game_state.room_file)[offset];

	if ((camswitch_array[num].cam1==0) || (camswitch_array[num].cam0!=game_state.camera)) {
		return 0;
	}

	switch_pos[0] = SDL_SwapLE16(camswitch_array[num].x1);
	switch_pos[1] = SDL_SwapLE16(camswitch_array[num].y1);
	switch_pos[2] = SDL_SwapLE16(camswitch_array[num].x2);
	switch_pos[3] = SDL_SwapLE16(camswitch_array[num].y2);
	switch_pos[4] = SDL_SwapLE16(camswitch_array[num].x3);
	switch_pos[5] = SDL_SwapLE16(camswitch_array[num].y3);
	switch_pos[6] = SDL_SwapLE16(camswitch_array[num].x4);
	switch_pos[7] = SDL_SwapLE16(camswitch_array[num].y4);
	return 1;
}
