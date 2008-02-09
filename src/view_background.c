/*
	Background image viewer

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

/*--- Includes ---*/

#include <stdlib.h>
#include <SDL.h>

#include "state.h"

/*--- Defines ---*/

#define KEY_STAGE_DOWN		SDLK_z
#define KEY_STAGE_UP		SDLK_s
#define KEY_STAGE_RESET		SDLK_x

#define KEY_ROOM_DOWN		SDLK_e
#define KEY_ROOM_UP		SDLK_d
#define KEY_ROOM_RESET		SDLK_c

#define KEY_CAMERA_DOWN		SDLK_r
#define KEY_CAMERA_UP		SDLK_f
#define KEY_CAMERA_RESET	SDLK_v

/*--- Variables ---*/

static int reload_bg = 1;
static int reload_room = 1;

/*--- Functions ---*/

int view_background_input(SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN) {
		switch (event->key.keysym.sym) {
			case KEY_STAGE_DOWN:
				game_state.stage -= 1;
				if (game_state.stage < 1) {
					game_state.stage = 7;
				}
				reload_bg = reload_room = 1;
				break;						
			case KEY_STAGE_UP:
				game_state.stage += 1;
				if (game_state.stage > 7) {
					game_state.stage = 1;
				}
				reload_bg = reload_room = 1;
				break;						
			case KEY_STAGE_RESET:
				game_state.stage = 1;
				reload_bg = reload_room = 1;
				break;						
			case KEY_ROOM_DOWN:
				game_state.room -= 1;
				if (game_state.room < 0) {
					game_state.room = 0x1c;
				}
				reload_bg = reload_room = 1;
				break;						
			case KEY_ROOM_UP:
				game_state.room += 1;
				if (game_state.room > 0x1c) {
					game_state.room = 0;
				}
				reload_bg = reload_room = 1;
				break;						
			case KEY_ROOM_RESET:
				game_state.room = 0;
				reload_bg = reload_room = 1;
				break;						
			case KEY_CAMERA_DOWN:
				game_state.camera -= 1;
				if ((game_state.camera<0) && (game_state.num_cameras>0)) {
					game_state.camera = game_state.num_cameras-1;
				}
				reload_bg = 1;
				break;						
			case KEY_CAMERA_UP:
				game_state.camera += 1;
				if (game_state.camera>=game_state.num_cameras) {
					game_state.camera = 0;
				}
				reload_bg = 1;
				break;						
			case KEY_CAMERA_RESET:
				game_state.camera = 0;
				reload_bg = 1;
				break;						
		}
	}

	return(reload_bg);
}

video_surface_t *view_background_update(void)
{
	if (reload_room) {
		state_loadroom();
		reload_room = 0;
	}
	if (reload_bg) {
		state_loadbackground();
		reload_bg = 0;
	}

	return game_state.back_surf;
}

