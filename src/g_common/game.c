/*
	Game state

	Copyright (C) 2007-2013	Patrice Mandin

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

#include <string.h>

#include "game.h"

#include "parameters.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

game_t game;

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void game_shutdown(void);

static void game_detect(void);

/*--- Functions ---*/

void game_init(void)
{
	memset(&game, 0, sizeof(game_t));

	game.init = game_init;
	game.shutdown = game_shutdown;

	game.major = GAME_UNKNOWN;

	game.num_stage = params.stage;
	game.num_room = params.room;
	game.num_camera = params.camera;

	player_init();
	room_init();

	game_detect();
}

void game_shutdown(void)
{
	game.player.shutdown();
	game.room.shutdown();
}

static void game_detect(void)
{
}
