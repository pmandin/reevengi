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

#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "../log.h"

#include "../render_texture.h"
#include "../render_skel.h"

#include "player.h"
#include "game.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/


/*--- Functions prototypes ---*/

static void player_shutdown(player_t *this);

static void player_unloadmodels(player_t *this);

static render_skel_t *player_load_model(int num_model);
static void player_get_model_name(char name[32]);

static void prev_model(void);
static void next_model(void);
static void reset_model(void);

static void prev_anim(void);
static void next_anim(void);
static void reset_anim(void);

/*--- Functions ---*/

void player_init(player_t *this)
{
	logMsg(2, "player: init\n");

	memset(this, 0, sizeof(player_t));

	this->shutdown = player_shutdown;

	this->load_model = player_load_model;
	this->get_model_name = player_get_model_name;

	this->prev_model = prev_model;
	this->next_model = next_model;
	this->reset_model = reset_model;

	this->prev_anim = prev_anim;
	this->next_anim = next_anim;
	this->reset_anim = reset_anim;

#ifdef ENABLE_DEBUG_POS
	this->x = 13148.0f;
	this->y = -2466.0f;
	this->z = 3367.0f;
	this->a = (157.0f * 4096.0f) / 360.0f;
#endif
}

static void player_shutdown(player_t *this)
{
	logMsg(2, "player: shutdown\n");

	player_unloadmodels(this);
}

static void player_unloadmodels(player_t *this)
{
	int i;

	for (i=0; i<this->model_list_count; i++) {
		render_skel_t *model = this->model_list[i].model;
		if (model) {
			model->shutdown(model);
			this->model_list[i].num_model = -1;
			this->model_list[i].model = NULL;
		}
	}

	if (this->model_list) {
		free(this->model_list);
		this->model_list = NULL;
		this->model_list_count = 0;
	}
}

static render_skel_t *player_load_model(int num_model)
{
	return NULL;
}

static void player_get_model_name(char name[32])
{
}

static void prev_model(void)
{
	--game.player.num_model;
	if (game.player.num_model<0) {
		game.player.num_model=0;
	}
}

static void next_model(void)
{
	++game.player.num_model;
	if (game.player.num_model>100) {
		game.player.num_model=100;
	}
}

static void reset_model(void)
{
	game.player.num_model = 0;
}

static void prev_anim(void)
{
	if (game.player.num_anim>0) {
		--game.player.num_anim;
	}
}

static void next_anim(void)
{
	int num_anims = game.player.model->getNumAnims(game.player.model);

	if (game.player.num_anim<num_anims-1) {
		++game.player.num_anim;
	}
}

static void reset_anim(void)
{
	game.player.num_anim=0;
}
