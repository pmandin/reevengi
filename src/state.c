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

/*--- Variables ---*/

state_t game_state;

/*--- Functions ---*/

void state_init(void)
{
	memset(&game_state, 0, sizeof(state_t));

	game_state.stage = 1;
	game_state.room = 0;
	game_state.camera = 0;
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
	if (game_state.surface_bg) {
		SDL_FreeSurface(game_state.surface_bg);
		game_state.surface_bg = NULL;
	}

	if (game_state.background) {
		free(game_state.background);
		game_state.background=NULL;
	}
}
