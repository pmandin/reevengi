/*
	RE1
	PS1
	Demo, Game

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
#include "re1_ps1.h"
#include "background_bss.h"
#include "parameters.h"

/*--- Defines ---*/

#define CHUNK_SIZE 32768

/*--- Types ---*/

/*--- Constant ---*/

static const char *re1ps1_bg = "psx/stage%d/room%d%02x.bss";

static const char *re1ps1demo_movies[] = {
	"psx/movie/capcom.str",
	"psx/movie/dm1.str",
	"psx/movie/dm2.str",
	"psx/movie/dm3.str",
	"psx/movie/oj.str",
	"psx/movie/pj.str",
	NULL
};

static const char *re1ps1game_movies[] = {
	"psx/movie/capcom.str",
	"psx/movie/dm1.str",
	"psx/movie/dm2.str",
	"psx/movie/dm3.str",
	"psx/movie/dm4.str",
	"psx/movie/dm6.str",
	"psx/movie/dm7.str",
	"psx/movie/dm8.str",
	"psx/movie/dmb.str",
	"psx/movie/dmc.str",
	"psx/movie/dmd.str",
	"psx/movie/dme.str",
	"psx/movie/dmf.str",
	"psx/movie/ed1.str",
	"psx/movie/ed2.str",
	"psx/movie/ed3.str",
	"psx/movie/ed4.str",
	"psx/movie/ed5.str",
	"psx/movie/ed6.str",
	"psx/movie/ed7.str",
	"psx/movie/ed8.str",
	"psx/movie/oj.str",
	"psx/movie/pj.str",
	"psx/movie/stfc.str",
	"psx/movie/stfj.str",
	NULL
};

/*--- Variables ---*/

/*--- Functions prototypes ---*/

/*--- Functions ---*/

void re1ps1_init(state_t *game_state)
{
	game_state->load_background = re1ps1_loadbackground;
	game_state->shutdown = re1ps1_shutdown;
}

void re1ps1_shutdown(void)
{
}

void re1ps1_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re1ps1_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re1ps1_bg, game_state.stage, game_state.stage, game_state.room);

	printf("bss: Loading %s ... %s\n", filepath,
		background_bss_load(filepath, CHUNK_SIZE) ? "done" : "failed"
	);

	free(filepath);
}
