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

#ifndef GAME_H
#define GAME_H 1

#include "../render_texture.h"

#include "player.h"
#include "room_map.h"
#include "room.h"

/*--- Enums ---*/

typedef enum {
	GAME_UNKNOWN,
	GAME_RE1,
	GAME_RE2,
	GAME_RE3
} game_major_e;

/*--- Types ---*/

typedef struct game_s game_t;

struct game_s {
	void (*shutdown)(game_t *this);

	/*--- Game version ---*/
	game_major_e major;	/* re1/re2/re3 */
	int minor;	/* demo/game, pc/ps1, etc */
	const char *name;

	int num_stage;	/* Stage of game */
	int num_room;	/* Room of stage */
	int num_camera;	/* Camera in room */

	player_t player;	/* player data */

	room_t	room;	/* current room */

	/*--- Movies ---*/

	char **movies_list;	/* List of movies */
	int num_movie;	/* Currently playing movie */
	char *cur_movie;	/* Current filename */

	/*--- Font for ASCII text ---*/
	render_texture_t *font;

	void (*load_font)(void);
	void (*get_char)(int ascii, int *x, int *y, int *w, int *h);
};

typedef struct {
	int version;	/* Num of version */
	const char *filename;	/* Filename to detect it */
	const char *name;	/* Name of version */
} game_detect_t;

/*--- Variables ---*/

extern game_t game;

/*--- Functions ---*/

void game_init(game_t *this);
int game_file_exists(const char *filename);

#endif /* GAME_H */
