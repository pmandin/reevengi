/*
	Game state

	Copyright (C) 2007-2010	Patrice Mandin

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

#ifndef STATE_H
#define STATE_H 1

#include "room.h"
#include "render_texture.h"
#include "render_skel.h"
#include "render_mask.h"

/*--- Defines ---*/

/*#define ENABLE_DEBUG_POS	1*/

/*--- Enums ---*/

/*enum {
	GAME_UNKNOWN,
	GAME_RE1_PS1_DEMO,
	GAME_RE1_PS1_GAME,
	GAME_RE1_PS1_SHOCK,
	GAME_RE2_PS1_DEMO,
	GAME_RE2_PS1_DEMO2,
	GAME_RE2_PS1_GAME_LEON,
	GAME_RE2_PS1_GAME_CLAIRE,
	GAME_RE3_PS1_DEMO,
	GAME_RE3_PS1_GAME,
	GAME_RE1_PC_DEMO,
	GAME_RE1_PC_GAME,
	GAME_RE2_PC_DEMO_P,
	GAME_RE2_PC_DEMO_U,
	GAME_RE2_PC_GAME_LEON,
	GAME_RE2_PC_GAME_CLAIRE,
	GAME_RE3_PC_DEMO,
	GAME_RE3_PC_GAME
};*/

/*--- Types ---*/

typedef struct {
	int num_model;
	render_skel_t	*model;
} model_item_t;

typedef struct {
	/*--- Game version ---*/
	int version;

	/*--- Data ---*/

	/* Stage of game */
	int num_stage;
	/* Room of stage */
	int num_room;
	/* Camera in room */
	int num_camera;

	/* Background image */
	render_texture_t *background;

	/* and its masks */
	render_texture_t *bg_mask;
	render_mask_t *rdr_mask;

	/* List of movies */
	char **movies_list;
	/* Currently playing movie */
	int num_movie;
	/* Movie filename */
	char *cur_movie/*[1024]*/;

	/* Font for ASCII text */
	render_texture_t *font;

	/*--- RDT room manager */
	room_t *room;

	/*--- EMD model manager ---*/
	int num_model;

	int model_list_count;
	model_item_t *model_list;

	int num_anim;
	int num_frame;

	/*--- Player position and angle */
	float player_x, player_y, player_z, player_a;

	/*--- Private functions, for backend ---*/
	void (*priv_load_background)(void);
	void (*priv_load_bgmask)(void);
	void (*priv_load_room)(void);
	render_skel_t *(*priv_load_model)(int num_model);
	void (*priv_shutdown)(void);

	/*--- Functions ---*/
	void (*load_background)(void);
	void (*load_room)(void);
	render_skel_t *(*load_model)(int num_model);
	void (*load_font)(void);
	void (*get_char)(int ascii, int *x, int *y, int *w, int *h);
	void (*get_model_name)(char name[32]);
	void (*shutdown)(void);
} state_t;

/*--- Variables ---*/

extern state_t game_state;

/*--- Functions ---*/

void state_init(void);

const char *state_getGameName(void);
int state_game_file_exists(char *filename);

void state_setstage(int new_stage);
void state_setroom(int new_room);
void state_setcamera(int new_camera);

void state_newmovie(void);
int state_getnummovies(void);

#endif /* STATE_H */
