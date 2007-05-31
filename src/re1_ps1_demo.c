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

#include "state.h"
#include "re1_ps1_demo.h"
#include "background_bss.h"
#include "parameters.h"

/*--- Defines ---*/

#define CHUNK_SIZE 32768

/*--- Types ---*/

/*--- Variables ---*/

static const char *re1ps1demo_bg = "psx/stage%d/room%d%02x.bss";

/*--- Functions prototypes ---*/

/*--- Functions ---*/

void re1ps1demo_init(state_t *game_state)
{
	game_state->load_background = re1ps1demo_loadbackground;
	game_state->shutdown = re1ps1demo_shutdown;
}

void re1ps1demo_shutdown(void)
{
}

void re1ps1demo_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re1ps1demo_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1demo_bg, game_state.stage, game_state.stage, game_state.room);

	printf("bss: Loading %s ... %s\n", filepath,
		background_bss_load(filepath, CHUNK_SIZE) ? "done" : "failed"
	);

	free(filepath);
}
