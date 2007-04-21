/*
	RE1
	PS1
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

#include "file.h"
#include "state.h"
#include "depack_vlc.h"
#include "depack_mdec.h"
#include "re1_ps1_demo.h"
#include "parameters.h"

/*--- Defines ---*/

#define WIDTH 320
#define HEIGHT 240

#define CHUNK_SIZE 32768

/*--- Types ---*/

/*--- Variables ---*/

static const char *re1ps1demo_bg = "psx/stage%d/room%d%02x.bss";

static char *finalpath = NULL;

/*--- Functions prototypes ---*/

static void re1ps1demo_load_bss_bg(const char *filename);
static void re1ps1demo_load_mdec_bg(void *buffer, int length);

/*--- Functions ---*/

void re1ps1demo_init(state_t *game_state)
{
	game_state->load_background = re1ps1demo_loadbackground;
	game_state->shutdown = re1ps1demo_shutdown;
}

void re1ps1demo_shutdown(void)
{
	if (finalpath) {
		free(finalpath);
		finalpath=NULL;
	}
}

void re1ps1demo_loadbackground(void)
{
	char *filepath;
	int length;

	if (!finalpath) {
		finalpath = malloc(strlen(basedir)+strlen(re1ps1demo_bg)+2);
		if (!finalpath) {
			fprintf(stderr, "Can not allocate mem for final path\n");
			return;
		}
		sprintf(finalpath, "%s/%s", basedir, re1ps1demo_bg);	
	}

	filepath = malloc(strlen(finalpath)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, finalpath, game_state.stage, game_state.stage, game_state.room);

	re1ps1demo_load_bss_bg(filepath);

	free(filepath);
}

void re1ps1demo_load_bss_bg(const char *filename)
{
	SDL_RWops *src;
	Uint8 *dstBuffer;
	int dstBufLen;
	
	src = SDL_RWFromFile(filename, "rb");
	if (src==NULL) {
		fprintf(stderr,"Can not load from file %s\n", filename);
		return;
	}

	game_state.num_cameras = SDL_RWseek(src, 0, RW_SEEK_END) / CHUNK_SIZE;
	if (game_state.camera<0) {
		game_state.camera=0;
	} else if (game_state.camera>=game_state.num_cameras) {
		game_state.camera = game_state.num_cameras-1;
	}
	SDL_RWseek(src, game_state.camera * CHUNK_SIZE, RW_SEEK_SET);
	printf("Selected angle %d/%d\n", game_state.camera, game_state.num_cameras);

	vlc_depack(src, &dstBuffer, &dstBufLen);

	if (dstBuffer && dstBufLen) {
		printf("vlc: loaded %s at 0x%08x, length %d\n", filename, dstBuffer, dstBufLen);

		saveFile("/tmp/room.vlc", dstBuffer, dstBufLen);

		re1ps1demo_load_mdec_bg(dstBuffer, dstBufLen);

		free(dstBuffer);

/*		if (dstBufLen == 320*256*2) {
			game_state.background = dstBuffer;
			game_state.surface_bg = adt_surface((Uint16 *) game_state.background);
		}
 */	}

	SDL_FreeRW(src);
}

static void re1ps1demo_load_mdec_bg(void *buffer, int length)
{
	SDL_RWops *src;
	Uint8 *dstBuffer;
	int dstBufLen;

	printf("load mdec from 0x%08x, length %d\n", buffer, length);

	src = SDL_RWFromMem(buffer, length);
	if (src==NULL) {
		fprintf(stderr,"Can not read from 0x%08x, length %d\n", buffer, length);
		return;
	}

	mdec_depack(src, &dstBuffer, &dstBufLen, WIDTH, HEIGHT);

	if (dstBuffer && dstBufLen) {
		printf("mdec: loaded at 0x%08x, length %d\n", dstBuffer, dstBufLen);

		/*saveFile("/tmp/room.rgb", dstBuffer, dstBufLen);*/

		game_state.background = dstBuffer;
		game_state.surface_bg = mdec_surface((Uint8 *) game_state.background, WIDTH, HEIGHT);
	} else {
		fprintf(stderr, "mdec: failed depacking frame\n");
	}
		
	SDL_FreeRW(src);
}
