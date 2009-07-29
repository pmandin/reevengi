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
#include "video.h"
#include "log.h"
#include "model_emd2.h"

/*--- Defines ---*/

#define MAX_MODELS	0x17

/*--- Types ---*/

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

typedef struct {
	Uint16 const0; /* 0xff01 */
	Uint8 cam0, cam1;
	Sint16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
} rdt_camera_switch_t;

/*--- Constant ---*/

static const char *re2pcdemo_bg = "common/stage%d/rc%d%02x%1x.adt";
static const char *re2pcdemo_room = "pl0/rd%c/room%d%02x0.rdt";
static const char *re2pcdemo_model = "pl0/emd0/em0%02x.%s";

static const int map_models[MAX_MODELS]={
	0x10,	0x11,	0x12,	0x13,	0x15,	0x16,	0x17,	0x18,
	0x1e,	0x1f,	0x20,	0x21,	0x22,	0x2d,	0x48,	0x4a,
	0x50,	0x51,	0x54,	0x55,	0x58,	0x59,	0x5a
};

/*--- Variables ---*/

static int game_lang = 'u';

/*--- Functions prototypes ---*/

static void re2pcdemo_shutdown(void);

static void re2pcdemo_loadbackground(void);
static int re2pcdemo_load_adt_bg(const char *filename);

static void re2pcdemo_loadroom(void);
static int re2pcdemo_loadroom_rdt(const char *filename);

static model_t *re2pcdemo_load_model(int num_model);

static void re2pcdemo_getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static int re2pcdemo_getNumCamswitches(room_t *this);
static void re2pcdemo_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

/*--- Functions ---*/

void re2pcdemo_init(state_t *game_state)
{
	game_state->priv_load_background = re2pcdemo_loadbackground;
	game_state->priv_load_room = re2pcdemo_loadroom;
	game_state->priv_shutdown = re2pcdemo_shutdown;

	if (game_state->version == GAME_RE2_PC_DEMO_P) {
		game_lang = 'p';
	}

	game_state->priv_load_model = re2pcdemo_load_model;
}

static void re2pcdemo_shutdown(void)
{
}

static void re2pcdemo_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re2pcdemo_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcdemo_bg, game_state.num_stage, game_state.num_stage,
		game_state.num_room, game_state.num_camera);

	logMsg(1, "adt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re2pcdemo_load_adt_bg(filepath) ? "done" : "failed");

	free(filepath);
}

static int re2pcdemo_load_adt_bg(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		Uint8 *dstBuffer;
		int dstBufLen;

		adt_depack(src, &dstBuffer, &dstBufLen);

		if (dstBuffer && dstBufLen) {
			/*game_state.num_cameras = 16;*/

			if (dstBufLen == 320*256*2) {
				SDL_Surface *image;

				image = adt_surface((Uint16 *) dstBuffer);
				if (image) {
					game_state.back_surf = video.createSurfaceSu(image);
					if (game_state.back_surf) {
						video.convertSurface(game_state.back_surf);
						retval = 1;
					}
					SDL_FreeSurface(image);
				}
			}

			free(dstBuffer);
		}

		SDL_RWclose(src);
	}

	return retval;
}

static void re2pcdemo_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re2pcdemo_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2pcdemo_room, game_lang, game_state.num_stage, game_state.num_room);

	logMsg(1, "adt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re2pcdemo_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re2pcdemo_loadroom_rdt(const char *filename)
{
	PHYSFS_sint64 length;
	Uint8 *rdt_header;
	void *file;

	file = FS_Load(filename, &length);
	if (!file) {
		return 0;
	}
	
	game_state.room = room_create(file);
	if (!game_state.room) {
		free(file);
		return 0;
	}

	rdt_header = (Uint8 *) file;
	game_state.room->num_cameras = rdt_header[1];
	game_state.room->num_camswitches = re2pcdemo_getNumCamswitches(game_state.room);

	game_state.room->getCamera = re2pcdemo_getCamera;
	game_state.room->getCamswitch = re2pcdemo_getCamswitch;

	return 1;
}

model_t *re2pcdemo_load_model(int num_model)
{
	char *filepath;
	model_t *model = NULL;
	void *emd, *tim;
	PHYSFS_sint64 emd_length, tim_length;

	if (num_model>=MAX_MODELS) {
		num_model = MAX_MODELS-1;
	}
	num_model = map_models[num_model];

	filepath = malloc(strlen(re2pcdemo_model)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re2pcdemo_model, num_model, "emd");

	logMsg(1, "Loading model %s...", filepath);
	emd = FS_Load(filepath, &emd_length);
	if (emd) {
		sprintf(filepath, re2pcdemo_model, num_model, "tim");
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

static void re2pcdemo_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
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

static int re2pcdemo_getNumCamswitches(room_t *this)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_switches = 0, prev_from = -1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary=0;

		if (prev_from != camswitch_array[i].cam0) {
			prev_from = camswitch_array[i].cam0;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].cam1==0)) {
			/* boundary, not a switch */
		} else {
			num_switches++;
		}

		++i;
	}

	return num_switches;
}

static void re2pcdemo_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	camswitch_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+8*4]);
	offset = SDL_SwapLE32(*camswitch_offset);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary = 0;

		if (prev_from != camswitch_array[i].cam0) {
			prev_from = camswitch_array[i].cam0;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].cam1==0)) {
			/* boundary, not a switch */
		} else {
			if (j==num_camswitch) {
				break;
			}

			++j;
		}

		++i;
	}

	room_camswitch->from = camswitch_array[i].cam0;
	room_camswitch->to = camswitch_array[i].cam1;
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}
