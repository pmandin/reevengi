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
#include "re1_ps1_demo.h"

/*--- Types ---*/

/*--- Variables ---*/

static const char *basedir = "/windows_d/re1ps1";
static const char *re1ps1demo_bg = "psx/stage%d/room%d%02x.bss";

static char *finalpath = NULL;

/*--- Functions ---*/

void re1ps1demo_init(state_t *game_state)
{
	game_state->load_background = re1ps1demo_loadbackground;
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

	game_state.background = loadFile(filepath, &length);
	if (game_state.background) {
		game_state.num_cameras = length / 32768;
		printf("Loaded %s, length %d, %d angles\n", filepath, length, game_state.num_cameras);

		/* Depack vlc+mdec file */
	}

	free(filepath);
}
