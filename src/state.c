/*
	Game state

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

#include <stdlib.h>
#include <SDL.h>

#include "state.h"
#include "parameters.h"

/*--- Variables ---*/

state_t game_state;

/*--- Functions prototypes ---*/

static void state_detect(void);
static int game_file_exists(char *filename);

/*--- Functions ---*/

void state_init(void)
{
	memset(&game_state, 0, sizeof(state_t));

	game_state.stage = 1;
	game_state.room = 0;
	game_state.camera = 0;

	state_detect();
}

void state_shutdown(void)
{
	if (game_state.shutdown) {
		(*game_state.shutdown)();
	}
}

void state_setstage(int new_stage)
{
	game_state.stage = new_stage;
}

void state_setroom(int new_room)
{
	game_state.room = new_room;
}

void state_setcamera(int new_camera)
{
	game_state.camera = new_camera;
}

void state_loadbackground(void)
{
	state_unloadbackground();

	if (game_state.load_background) {
		(*game_state.load_background)();
	}
}

void state_unloadbackground(void)
{
	if (game_state.background_surf) {
		SDL_FreeSurface(game_state.background_surf);
		game_state.background_surf = NULL;
	}
}

/* Detect some game version */

static int game_file_exists(char *filename)
{
	char *filenamedir;
	int detected = 0;
	
	filenamedir = malloc(strlen(basedir)+strlen(filename)+4);
	if (filenamedir) {
		int handle;

		sprintf(filenamedir, "%s/%s", basedir, filename);

		handle = open(filenamedir, 0644);
		if (handle>=0) {
			detected = 1;
			close(handle);
		}

		free(filenamedir);
	}

	return detected;
}

static void state_detect(void)
{
	game_state.version = GAME_UNKNOWN;

	if (game_file_exists("rofs1.dat")) {
		game_state.version = GAME_RE3_PC_DEMO;
	} else if (game_file_exists("common/datu/warning.adt")) {
		game_state.version = GAME_RE2_PC_DEMO;
	} else if (game_file_exists("usa/data/chris02.pix")) {
		game_state.version = GAME_RE1_PC_GAME;
	} else if (game_file_exists("sles_025.30")) {
		game_state.version = GAME_RE3_PS1_GAME;
	} else if (game_file_exists("slps_009.99")) {
		game_state.version = GAME_RE2_PS1_DEMO;
	} else if (game_file_exists("slpm_800.27")) {
		game_state.version = GAME_RE1_PS1_DEMO;
	}
}
