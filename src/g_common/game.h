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

/*--- Enums ---*/

typedef enum {
	GAME_UNKNOWN,
	GAME_RE1,
	GAME_RE2,
	GAME_RE3
} game_major_e;

/*--- External types ---*/

typedef struct room_s room_t;
typedef struct player_s player_t;
typedef struct menu_s menu_t;
typedef struct render_texture_s render_texture_t;

/*--- Types ---*/

typedef struct game_s game_t;

struct game_s {
	void (*dtor)(game_t *this);

	/*--- Game version ---*/
	game_major_e major;	/* re1/re2/re3 */
	int minor;	/* demo/game, pc/ps1, etc */
	const char *name;

	int num_stage;	/* Stage of game */
	int num_room;	/* Room of stage */
	int num_camera;	/* Camera in room */

	room_t *room;	/* room */
	player_t *player;	/* player */
	menu_t *menu;	/* menu */

	void (*prev_stage)(game_t *this);
	void (*next_stage)(game_t *this);
	void (*reset_stage)(game_t *this);

	void (*prev_room)(game_t *this);
	void (*next_room)(game_t *this);
	void (*reset_room)(game_t *this);

	void (*prev_camera)(game_t *this);
	void (*next_camera)(game_t *this);
	void (*reset_camera)(game_t *this);

	/*--- Room ---*/
	room_t *room_ctor(int num_stage, int num_room);

	/*--- Movies ---*/

	char **movies_list;	/* List of movies */
	int num_movie;	/* Currently playing movie */
	char *cur_movie;	/* Current filename */

	void (*prev_movie)(game_t *this);
	void (*next_movie)(game_t *this);
	void (*reset_movie)(game_t *this);

	void (*switch_movie)(game_t *this);

	/*--- Font for ASCII text ---*/
	render_texture_t *font;

	void (*load_font)(game_t *this);
	void (*get_char)(game_t *this, int ascii, int *x, int *y, int *w, int *h);
};

typedef struct {
	int version;	/* Num of version */
	const char *filename;	/* Filename to detect it */
	const char *name;	/* Name of version */
} game_detect_t;

/*--- Variables ---*/

extern game_t *game;

/*--- Functions ---*/

game_t *game_ctor(void);
int game_file_exists(const char *filename);

#endif /* GAME_H */
