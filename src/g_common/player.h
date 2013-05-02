/*
	Player data

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

#ifndef PLAYER_H
#define PLAYER_H 1

#include "../render_skel.h"

/*--- Defines ---*/

/*#define ENABLE_DEBUG_POS	1*/

/*--- Types ---*/

typedef struct {
	int num_model;
	render_skel_t	*model;
} model_item_t;

typedef struct player_s player_t;

struct player_s {
	void (*shutdown)(player_t *this);

	/* Game specific functions */
	render_skel_t *(*load_model)(int num_model);
	void (*get_model_name)(char name[32]);

	float x,y,z,a;
	render_skel_t *model;

	/*--- 3D model ---*/
	int num_model;

	int model_list_count;
	model_item_t *model_list;

	int num_anim;
	int num_frame;

	void (*prev_model)(void);
	void (*next_model)(void);
	void (*reset_model)(void);

	void (*prev_anim)(void);
	void (*next_anim)(void);
	void (*reset_anim)(void);
};

/*--- Variables ---*/

/*--- Functions ---*/

void player_init(player_t *this);

#endif /* PLAYER_H */
