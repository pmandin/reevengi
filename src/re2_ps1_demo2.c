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
#include "log.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Types ---*/

typedef struct {
	Uint32	offset;
	Uint32	flag;	/* 0: ignore, 1: tim or emd */
} re2ps1_ems_t;

/*--- Constant ---*/

static const char *re2ps1_bg1 = "res2/zcommon/bss/room1%02x.bss";
static const char *re2ps1_bg2 = "res2/zcommon/bss2/room2%02x.bss";
static const char *re2ps1_room1 = "res2/zpl%d/rdt/room1%02x0.rdt";
static const char *re2ps1_room2 = "res2/zpl%d/rdt2/room2%02x0.rdt";
static const char *re2ps1_model = "pl%d/pld/cdemd%d.ems";

static const char *re2ps1demo_movies[] = {
	"res2/zz/virgin.str",
	NULL
};

static const re2ps1_ems_t re2ps1demo21_ems[]={
	{0x00000000, 0},
	{0x0000d000, 0},
	{0x0001a000, 1},	/* tim */
	{0x0002a800, 1},	/* emd */
	{0x0004e800, 1},	/* tim */
	{0x0005f000, 1},	/* emd */
	{0x00083800, 1},	/* tim */
	{0x000a4800, 1},	/* emd */
	{0x000c8800, 1},	/* tim */
	{0x000d9000, 1},	/* emd */
	{0x000fd000, 1},	/* tim */
	{0x0010d800, 1},	/* emd */
	{0x00131800, 1},	/* tim */
	{0x00142000, 1},	/* emd */
	{0x00166000, 1},	/* tim */
	{0x00176800, 1},	/* emd */
	{0x0019b800, 1},	/* tim */
	{0x001ac000, 1},	/* emd */
	{0x001ce800, 1},	/* tim */
	{0x001ef800, 1},	/* emd */
	{0x00212000, 1},	/* tim */
	{0x00233000, 1},	/* emd */
	{0x00255800, 0},
	{0x0025b000, 0},
	{0x00260800, 1},	/* tim */
	{0x00271000, 1},	/* emd */
	{0x0028f000, 0},
	{0x00294000, 0},
	{0x00299000, 1},	/* tim */
	{0x002a1800, 1},	/* emd */
	{0x002a9800, 0},
	{0x002b1000, 0},
	{0x002b8800, 1},	/* tim */
	{0x002c9000, 1},	/* emd */
	{0x002f4000, 0},
	{0x002fd000, 1},	/* tim */
	{0x0031e000, 1},	/* emd */
	{0x0034c800, 0},
	{0x00352800, 0},
	{0x00358800, 1},	/* tim */
	{0x00369000, 1},	/* emd */
	{0x00389000, 0},
	{0x0038f800, 0},
	{0x00396000, 1},	/* tim */
	{0x003a6800, 1},	/* emd */
	{0x003b4000, 0},
	{0x003b5800, 0},
	{0x003b7000, 1},	/* tim */
	{0x003bf800, 1},	/* emd */
	{0x003c1800, 0},
	{0x003c5000, 0},
	{0x003c8800, 1},	/* tim */
	{0x003d1000, 1},	/* emd */
	{0x003db000, 0},
	{0x003e1800, 0},
	{0x003e8000, 1},	/* tim */
	{0x00409000, 1},	/* emd */
	{0x00437000, 0},
	{0x00439000, 0},
	{0x0043b000, 1},	/* tim */
	{0x00443800, 1},	/* emd */
	{0x00447000, 0},
	{0x0044c000, 0},
	{0x00451000, 1},	/* tim */
	{0x00461800, 1},	/* emd */
	{0x00483800, 0},
	{0x00488800, 0},
	{0x0048d800, 1},	/* tim */
	{0x004a6000, 1},	/* emd */
	{0x004c8800, 0},
	{0x004c9800, 1},	/* tim */
	{0x004d2000, 1},	/* emd */
	{0x004d6000, 0},
	{0x004d7800, 0},
	{0x004d9000, 1},	/* tim */
	{0x004e1800, 1},	/* emd */
	{0x004e8000, 0},
	{0x004f3000, 1},	/* tim */
	{0x00503800, 1},	/* emd */
	{0x00538800, 0},
	{0x0053b000, 0},
	{0x0053d800, 1},	/* tim */
	{0x00546000, 1},	/* emd */
	{0x00547800, 0},
	{0x0054f000, 0},
	{0x00556800, 1},	/* tim */
	{0x0056f000, 1},	/* emd */
	{0x00594800, 0},
	{0x0059c800, 0},
	{0x005a4800, 1},	/* tim */
	{0x005bd000, 1},	/* emd */
	{0x005ec800, 0},
	{0x005f3000, 0},
	{0x005f9800, 1},	/* tim */
	{0x00612000, 1},	/* emd */
	{0x00644800, 0},
	{0x0064f800, 0},
	{0x0065a800, 1},	/* tim */
	{0x00673000, 1},	/* emd */
	{0x006b6000, 0},
	{0x006bc000, 0},
	{0x006c2000, 1},	/* tim */
	{0x006e2800, 1},	/* emd */
	{0x006fe000, 0},
	{0x00704000, 0},
	{0x0070a000, 1},	/* tim */
	{0x00712800, 1},	/* emd */
	{0x00721000, 0},
	{0x00726800, 0},
	{0x0072c000, 1},	/* tim */
	{0x0073c800, 1},	/* emd */
	{0x00771800, 0},
	{0x00776800, 0},
	{0x0077b800, 1},	/* tim */
	{0x0078c000, 1},	/* emd */
	{0x00797000, 0},
	{0x00798800, 0},
	{0x0079a000, 1},	/* tim */
	{0x007a2800, 1},	/* emd */
	{0x007a3800, 0},
	{0x007a4800, 1},	/* tim */
	{0x007ad000, 1},	/* emd */
	{0x007bb000, 0},
	{0x007bb800, 0},
	{0x007bc000, 1},	/* tim */
	{0x007c4800, 1},	/* emd */
	{0x007cb000, 1},	/* tim */
	{0x007e3800, 1},	/* emd */
	{0x007f1000, 1},	/* tim */
	{0x00801800, 1},	/* emd */
	{0x00814800, 1},	/* tim */
	{0x0082d000, 1},	/* emd */
	{0x00840000, 1},	/* tim */
	{0x00858800, 1},	/* emd */
	{0x0086b000, 1},	/* tim */
	{0x0087b800, 1},	/* emd */
	{0x0088e000, 1},	/* tim */
	{0x008a6800, 1},	/* emd */
	{0x008b9000, 1},	/* tim */
	{0x008d1800, 1},	/* emd */
	{0x008de800, 1},	/* tim */
	{0x008f7000, 1},	/* emd */
	{0x008ff000, 1},	/* tim */
	{0x00917800, 1},	/* emd */
	{0x00924800, 1},	/* tim */
	{0x0093d000, 1},	/* emd */
	{0x00964800, 1},	/* tim */
	{0x00975000, 1},	/* emd */
	{0x00987800, 1},	/* tim */
	{0x009a0000, 1},	/* emd */
	{0x009b2800, 1},	/* tim */
	{0x009c3000, 1},	/* emd */
	{0x009d6000, 1},	/* tim */
	{0x009e6800, 1},	/* emd */
	{0x009f9000, 1},	/* tim */
	{0x00a09800, 1},	/* emd */
	{0x00a1c800, 1},	/* tim */
	{0x00a2d000, 1},	/* emd */
	{0x00a3f800, 1},	/* tim */
	{0x00a58000, 1},	/* emd */
	{0x00a6b000, 1},	/* tim */
	{0x00a83800, 1},	/* emd */
	{0x00a97000, 0}		/* file length */
};

static const re2ps1_ems_t re2ps1demo22_ems[]={
	{0x00000000, 1},	/* tim */
	{0x00010800, 1},	/* emd */
	{0x00034800, 1},	/* tim */
	{0x00055800, 1},	/* emd */
	{0x00079800, 1},	/* tim */
	{0x0008a000, 1},	/* emd */
	{0x000ae000, 1},	/* tim */
	{0x000cf000, 1},	/* emd */
	{0x000f1800, 1},	/* tim */
	{0x00102000, 1},	/* emd */
	{0x00120000, 1},	/* tim */
	{0x00130800, 1},	/* emd */
	{0x0013e000, 1},	/* tim */
	{0x00146800, 1},	/* emd */
	{0x00148800, 1},	/* tim */
	{0x00151000, 1},	/* emd */
	{0x00154000, 1},	/* tim */
	{0x00164800, 1},	/* emd */
	{0x00177000, 1},	/* tim */
	{0x0018f800, 1},	/* emd */
	{0x0019c800, 1},	/* tim */
	{0x001b5000, 1},	/* emd */
	{0x001c2000, 1},	/* tim */
	{0x001da800, 1},	/* emd */
	{0x001ed000, 1},	/* tim */
	{0x001fd800, 1},	/* emd */
	{0x00210000, 1},	/* tim */
	{0x00228800, 1},	/* emd */
	{0x0023b800, 1},	/* tim */
	{0x00254000, 1},	/* emd */
	{0x00267800, 0}		/* file length */
};

/*--- Variables ---*/

static int game_player = 0;

/*--- Functions prototypes ---*/

static void re2ps1_shutdown(void);

static void re2ps1_loadbackground(void);

static void re2ps1_loadroom(void);
static int re2ps1_loadroom_rdt(const char *filename);

static int re2ps1_parse_ems(int num_model,
	const re2ps1_ems_t *ems, int ems_size,
	int *num_tim, int *num_emd);

static model_t *re2ps1_load_model(int num_model);

/*--- Functions ---*/

void re2ps1demo2_init(state_t *game_state)
{
	game_state->load_background = re2ps1_loadbackground;
	game_state->load_room = re2ps1_loadroom;
	game_state->shutdown = re2ps1_shutdown;

	game_state->movies_list = (char **) re2ps1demo_movies;

	game_state->load_model = re2ps1_load_model;
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

static int re2ps1_parse_ems(int num_model,
	const re2ps1_ems_t *ems, int ems_size,
	int *num_tim, int *num_emd)
{
	int i, num_parsed = 0;
	int is_tim = 1;
	int new_tim, new_emd;

	for (i=0; i<ems_size; i++) {
		if (!ems[i].flag) {
			continue;
		}
		if (is_tim) {
			new_tim = i;
		} else {
			new_emd = i;
			if (num_parsed == num_model) {
				*num_tim = new_tim;
				*num_emd = new_emd;
				break;
			}
			num_parsed++;
		}
		is_tim ^= 1;
	}

	return num_parsed;
}

model_t *re2ps1_load_model(int num_model)
{
	char *filepath;
	model_t *model = NULL;
	SDL_RWops *src, *src_emd, *src_tim;
	int i = 0, num_file = game_player;
	int num_tim = -1, num_emd = -1;
	int parsed = 0;
	const re2ps1_ems_t *ems_array;
	Uint32 emd_offset, tim_offset;
	Uint32 emd_length, tim_length;
	void *emdBuf, *timBuf;

	ems_array = re2ps1demo21_ems;
	parsed = re2ps1_parse_ems(num_model,
		re2ps1demo21_ems, sizeof(re2ps1demo21_ems)/sizeof(re2ps1_ems_t),
		&num_tim, &num_emd);
	if ((num_tim==-1) || (num_emd==-1)) {
		int num_model2 = num_model-parsed;
		num_file += 2;
		ems_array = re2ps1demo22_ems;
		parsed = re2ps1_parse_ems(num_model2,
			re2ps1demo22_ems, sizeof(re2ps1demo22_ems)/sizeof(re2ps1_ems_t),
			&num_tim, &num_emd);
	}

	if ((num_emd==-1) || (num_tim==-1)) {
		return NULL;
	}
	emd_offset = ems_array[num_emd].offset;
	emd_length = ems_array[num_emd+1].offset - emd_offset;
	tim_offset = ems_array[num_tim].offset;
	tim_length = ems_array[num_tim+1].offset - tim_offset;

	filepath = malloc(strlen(re2ps1_model)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return NULL;
	}
	sprintf(filepath, re2ps1_model,
		game_player, num_file);

	logMsg(1, "Loading model 0x%02x...", num_model);
	src = FS_makeRWops(filepath);	
	if (src) {
		src_tim = src_emd = NULL;

		SDL_RWseek(src, tim_offset, RW_SEEK_SET);
		timBuf = malloc(tim_length);
		if (timBuf) {
			SDL_RWread(src, timBuf, tim_length, 1);
			src_tim = SDL_RWFromMem(timBuf, tim_length);
		}

		SDL_RWseek(src, emd_offset, RW_SEEK_SET);
		emdBuf = malloc(emd_length);
		if (emdBuf) {
			SDL_RWread(src, emdBuf, emd_length, 1);
			src_emd = SDL_RWFromMem(emdBuf, emd_length);
		}

		if (src_tim && src_emd) {
			model = model_emd2_load(src_emd, src_tim);
		}

		if (src_emd) {
			SDL_RWclose(src_emd);
		}
		if (emdBuf) {
			free(emdBuf);
		}
		if (src_tim) {
			SDL_RWclose(src_tim);
		}
		if (timBuf) {
			free(timBuf);
		}
		SDL_RWclose(src);
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
