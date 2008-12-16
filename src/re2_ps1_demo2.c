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

#define MAX_RE2PS1DEMO2_EMS 55

/*--- Types ---*/

typedef struct {
	Uint32	offset;
	Uint32	length;
} re2ps1demo2_ems_t;

/*--- Constant ---*/

static const char *re2ps1_bg1 = "res2/zcommon/bss/room1%02x.bss";
static const char *re2ps1_bg2 = "res2/zcommon/bss2/room2%02x.bss";
static const char *re2ps1_room1 = "res2/zpl%d/rdt/room1%02x0.rdt";
static const char *re2ps1_room2 = "res2/zpl%d/rdt2/room2%02x0.rdt";

static const char *re2ps1demo_movies[] = {
	"res2/zz/virgin.str",
	NULL
};

static const re2ps1demo2_ems_t re2ps1demo2_ems[MAX_RE2PS1DEMO2_EMS*2]={
	/*{0x00000000, 0x0000d000},
	{0x0000d000, 0x0000d000},*/
	{0x0001a000, 0x00010800},	/* tim */
	{0x0002a800, 0x00024000},	/* emd */
	{0x0004e800, 0x00010800},	/* tim */
	{0x0005f000, 0x00024800},	/* emd */
	{0x00083800, 0x00021000},	/* tim */
	{0x000a4800, 0x00024000},	/* emd */
	{0x000c8800, 0x00010800},	/* tim */
	{0x000d9000, 0x00024000},	/* emd */
	{0x000fd000, 0x00010800},	/* tim */
	{0x0010d800, 0x00024000},	/* emd */
	{0x00131800, 0x00010800},	/* tim */
	{0x00142000, 0x00024000},	/* emd */
	{0x00166000, 0x00010800},	/* tim */
	{0x00176800, 0x00025000},	/* emd */
	{0x0019b800, 0x00010800},	/* tim */
	{0x001ac000, 0x00022800},	/* emd */
	{0x001ce800, 0x00021000},	/* tim */
	{0x001ef800, 0x00022800},	/* emd */
	{0x00212000, 0x00021000},	/* tim */
	{0x00233000, 0x00022800},	/* emd */
	/*{0x00255800, 0x00005800},
	{0x0025b000, 0x00005800},*/
	{0x00260800, 0x00010800},	/* tim */
	{0x00271000, 0x0001e000},	/* emd */
	/*{0x0028f000, 0x00005000},
	{0x00294000, 0x00005000},*/
	{0x00299000, 0x00008800},	/* tim */
	{0x002a1800, 0x00008000},	/* emd */
	/*{0x002a9800, 0x00007800},
	{0x002b1000, 0x00007800},*/
	{0x002b8800, 0x00010800},	/* tim */
	{0x002c9000, 0x0002b000},	/* emd */
	/*{0x002f4000, 0x00009000},*/
	{0x002fd000, 0x00021000},	/* tim */
	{0x0031e000, 0x0002e800},	/* emd */
	/*{0x0034c800, 0x00006000},
	{0x00352800, 0x00006000},*/
	{0x00358800, 0x00010800},	/* tim */
	{0x00369000, 0x00020000},	/* emd */
	/*{0x00389000, 0x00006800},
	{0x0038f800, 0x00006800},*/
	{0x00396000, 0x00010800},	/* tim */
	{0x003a6800, 0x0000d800},	/* emd */
	/*{0x003b4000, 0x00001800},
	{0x003b5800, 0x00001800},*/
	{0x003b7000, 0x00008800},	/* tim */
	{0x003bf800, 0x00002000},	/* emd */
	/*{0x003c1800, 0x00003800},
	{0x003c5000, 0x00003800},*/
	{0x003c8800, 0x00008800},	/* tim */
	{0x003d1000, 0x0000a000},	/* emd */
	/*{0x003db000, 0x00006800},
	{0x003e1800, 0x00006800},*/
	{0x003e8000, 0x00021000},	/* tim */
	{0x00409000, 0x0002e000},	/* emd */
	/*{0x00437000, 0x00002000},
	{0x00439000, 0x00002000},*/
	{0x0043b000, 0x00008800},	/* tim */
	{0x00443800, 0x00003800},	/* emd */
	/*{0x00447000, 0x00005000},
	{0x0044c000, 0x00005000},*/
	{0x00451000, 0x00010800},	/* tim */
	{0x00461800, 0x00022000},	/* emd */
	/*{0x00483800, 0x00005000},
	{0x00488800, 0x00005000},*/
	{0x0048d800, 0x00018800},	/* tim */
	{0x004a6000, 0x00022800},	/* emd */
	/*{0x004c8800, 0x00001000},*/
	{0x004c9800, 0x00008800},	/* tim */
	{0x004d2000, 0x00004000},	/* emd */
	/*{0x004d6000, 0x00001800},
	{0x004d7800, 0x00001800},*/
	{0x004d9000, 0x00008800},	/* tim */
	{0x004e1800, 0x00006800},	/* emd */
	/*{0x004e8000, 0x0000b000},*/
	{0x004f3000, 0x00010800},	/* tim */
	{0x00503800, 0x00035000},	/* emd */
	/*{0x00538800, 0x00002800},
	{0x0053b000, 0x00002800},*/
	{0x0053d800, 0x00008800},	/* tim */
	{0x00546000, 0x00001800},	/* emd */
	/*{0x00547800, 0x00007800},
	{0x0054f000, 0x00007800},*/
	{0x00556800, 0x00018800},	/* tim */
	{0x0056f000, 0x00025800},	/* emd */
	/*{0x00594800, 0x00008000},
	{0x0059c800, 0x00008000},*/
	{0x005a4800, 0x00018800},	/* tim */
	{0x005bd000, 0x0002f800},	/* emd */
	/*{0x005ec800, 0x00006800},
	{0x005f3000, 0x00006800},*/
	{0x005f9800, 0x00018800},	/* tim */
	{0x00612000, 0x00032800},	/* emd */
	/*{0x00644800, 0x0000b000},
	{0x0064f800, 0x0000b000},*/
	{0x0065a800, 0x00018800},	/* tim */
	{0x00673000, 0x00043000},	/* emd */
	/*{0x006b6000, 0x00006000},
	{0x006bc000, 0x00006000},*/
	{0x006c2000, 0x00000800},	/* tim */
	{0x006e2800, 0x0001b800},	/* emd */
	/*{0x006fe000, 0x00006000},
	{0x00704000, 0x00006000},*/
	{0x0070a000, 0x00008800},	/* tim */
	{0x00712800, 0x0000e800},	/* emd */
	/*{0x00721000, 0x00005800},
	{0x00726800, 0x00005800},*/
	{0x0072c000, 0x00010800},	/* tim */
	{0x0073c800, 0x00035000},	/* emd */
	/*{0x00771800, 0x00005000},
	{0x00776800, 0x00005000},*/
	{0x0077b800, 0x00010800},	/* tim */
	{0x0078c000, 0x0000b000},	/* emd */
	/*{0x00797000, 0x00001800},
	{0x00798800, 0x00001800},*/
	{0x0079a000, 0x00008800},	/* tim */
	{0x007a2800, 0x00001000},	/* emd */
	/*{0x007a3800, 0x00001000},*/
	{0x007a4800, 0x00008800},	/* tim */
	{0x007ad000, 0x0000e000},	/* emd */
	/*{0x007bb000, 0x00000800},
	{0x007bb800, 0x00000800},*/
	{0x007bc000, 0x00008800},	/* tim */
	{0x007c4800, 0x00006800},	/* emd */
	{0x007cb000, 0x00018800},	/* tim */
	{0x007e3800, 0x0000d800},	/* emd */
	{0x007f1000, 0x00010800},	/* tim */
	{0x00801800, 0x00013000},	/* emd */
	{0x00814800, 0x00018800},	/* tim */
	{0x0082d000, 0x00013000},	/* emd */
	{0x00840000, 0x00018800},	/* tim */
	{0x00858800, 0x00012800},	/* emd */
	{0x0086b000, 0x00010800},	/* tim */
	{0x0087b800, 0x00012800},	/* emd */
	{0x0088e000, 0x00018800},	/* tim */
	{0x008a6800, 0x00012800},	/* emd */
	{0x008b9000, 0x00018800},	/* tim */
	{0x008d1800, 0x0000d000},	/* emd */
	{0x008de800, 0x00018800},	/* tim */
	{0x008f7000, 0x00008000},	/* emd */
	{0x008ff000, 0x00018800},	/* tim */
	{0x00917800, 0x0000d000},	/* emd */
	{0x00924800, 0x00018800},	/* tim */
	{0x0093d000, 0x00027800},	/* emd */
	{0x00964800, 0x00010800},	/* tim */
	{0x00975000, 0x00012800},	/* emd */
	{0x00987800, 0x00018800},	/* tim */
	{0x009a0000, 0x00012800},	/* emd */
	{0x009b2800, 0x00010800},	/* tim */
	{0x009c3000, 0x00013000},	/* emd */
	{0x009d6000, 0x00010800},	/* tim */
	{0x009e6800, 0x00012800},	/* emd */
	{0x009f9000, 0x00010800},	/* tim */
	{0x00a09800, 0x00013000},	/* emd */
	{0x00a1c800, 0x00010800},	/* tim */
	{0x00a2d000, 0x00012800},	/* emd */
	{0x00a3f800, 0x00018800},	/* tim */
	{0x00a58000, 0x00013000},	/* emd */
	{0x00a6b000, 0x00018800},	/* tim */
	{0x00a83800, 0x00013800},	/* emd */
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
