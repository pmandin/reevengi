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

/*--- Types ---*/

/*--- Constant ---*/

static const char *re2pcdemo_bg = "common/stage%d/rc%d%02x%1x.adt";
static const char *re2pcdemo_room = "pl0/rd%c/room%d%02x0.rdt";
static const char *re2pcdemo_model = "pl0/emd0/em050.emd";

/*--- Variables ---*/

static int game_lang = 'u';

/*--- Functions prototypes ---*/

static void re2pcdemo_shutdown(void);

static void re2pcdemo_loadbackground(void);
static int re2pcdemo_load_adt_bg(const char *filename);

static void re2pcdemo_loadroom(void);
static int re2pcdemo_loadroom_rdt(const char *filename);

/*--- Functions ---*/

void re2pcdemo_init(state_t *game_state)
{
	game_state->load_background = re2pcdemo_loadbackground;
	game_state->load_room = re2pcdemo_loadroom;
	game_state->shutdown = re2pcdemo_shutdown;

	if (game_state->version == GAME_RE2_PC_DEMO_P) {
		game_lang = 'p';
	}

	game_state->load_model = model_emd2_load;
	game_state->close_model = model_emd2_close;
	game_state->draw_model = model_emd2_draw;
	game_state->model = re2pcdemo_model;
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
	sprintf(filepath, re2pcdemo_bg, game_state.stage, game_state.stage, game_state.room, game_state.camera);

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

				/*game_state.background_surf = adt_surface((Uint16 *) dstBuffer);
				if (game_state.background_surf) {
					retval = 1;
				}*/

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
	sprintf(filepath, re2pcdemo_room, game_lang, game_state.stage, game_state.room);

	logMsg(1, "adt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re2pcdemo_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re2pcdemo_loadroom_rdt(const char *filename)
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

void re2pcdemo_get_camera(long *camera_pos)
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

int re2pcdemo_get_num_camswitch(void)
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

int re2pcdemo_get_camswitch(int num, short *switch_pos)
{
	Uint32 *camswitch_offset, offset;
	rdt_camera_switch_t *camswitch_array;
	int i;

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
