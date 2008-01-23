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
#include <physfs.h>

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

	game_state.movies_list = NULL;
	game_state.num_movie = 0;
	/*memset(game_state.cur_movie, 0, sizeof(game_state.cur_movie));*/
	game_state.cur_movie = NULL;

	state_detect();
}

void state_shutdown(void)
{
	state_unloadbackground();
	state_unloadroom();

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

void state_loadroom(void)
{
	state_unloadroom();

	if (game_state.load_room) {
		(*game_state.load_room)();
	}
}

void state_unloadroom(void)
{
	if (game_state.room_file) {
		free(game_state.room_file);
		game_state.room_file = NULL;
	}
}

void state_newmovie(void)
{
	char **movie = game_state.movies_list;
	int i;

	for (i=0; movie[i]; i++) {
		if (i==game_state.num_movie) {
			/*sprintf(game_state.cur_movie, "%s/%s", basedir, movie[i]);*/
			game_state.cur_movie = movie[i];
			break;
		}
	}
}

int state_getnummovies(void)
{
	char **movie = game_state.movies_list;
	int i;

	for (i=0; movie[i]; i++) {
	}
	return i;
}

/* Detect some game version */

static int game_file_exists(char *filename)
{
	char *filenamedir;
	int detected = 0;
	
	filenamedir = malloc(strlen(basedir)+strlen(filename)+4);
	if (filenamedir) {
		PHYSFS_file	*curfile;

		sprintf(filenamedir, "%s/%s", basedir, filename);

		curfile = PHYSFS_openRead(filename);
		if (curfile) {
			char dummy;

			if (PHYSFS_read(curfile, &dummy, 1, 1)>0) {
				detected = 1;
			}

			PHYSFS_close(curfile);
		}

		free(filenamedir);
	}

	return detected;
}

static void state_detect(void)
{
	game_state.version = GAME_UNKNOWN;

	if (game_file_exists("rofs2.dat")) {
		game_state.version = GAME_RE3_PC_GAME;
	} else if (game_file_exists("rofs1.dat")) {
		game_state.version = GAME_RE3_PC_DEMO;
	} else if (game_file_exists("REGIST/LEONF.EXE")) {
		game_state.version = GAME_RE2_PC_GAME_LEON;
	} else if (game_file_exists("REGIST/CLAIREF.EXE")) {
		game_state.version = GAME_RE2_PC_GAME_CLAIRE;
	} else if (game_file_exists("Regist/LeonP.exe")) {
		game_state.version = GAME_RE2_PC_DEMO;
	} else if (game_file_exists("regist/leonu.exe")) {
		game_state.version = GAME_RE2_PC_DEMO;
	} else if (game_file_exists("horr/usa/data/capcom.ptc")) {
		game_state.version = GAME_RE1_PC_GAME;
	} else if (game_file_exists("sles_025.30")) {
		game_state.version = GAME_RE3_PS1_GAME;
	} else if (game_file_exists("sles_009.73")) {
		game_state.version = GAME_RE2_PS1_GAME_LEON;
	} else if (game_file_exists("sles_109.73")) {
		game_state.version = GAME_RE2_PS1_GAME_CLAIRE;
	} else if (game_file_exists("slps_009.99") || game_file_exists("sled_009.77")) {
		game_state.version = GAME_RE2_PS1_DEMO;
	} else if (game_file_exists("sles_002.27")) {
		game_state.version = GAME_RE1_PS1_GAME;
	} else if (game_file_exists("slpm_800.27")) {
		game_state.version = GAME_RE1_PS1_DEMO;
	}
}
