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

/*--- Types ---*/

/*--- Constants ---*/

#define ANIM_HZ 15

/*--- Global variables ---*/

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void dtor(player_t *this);

static void unload_models(player_t *this);

static render_skel_t *load_model(player_t *this, int num_model);
static void get_model_name(player_t *this, char name[32]);

static void prev_model(player_t *this);
static void next_model(player_t *this);
static void reset_model(player_t *this);

static void prev_anim(player_t *this);
static void next_anim(player_t *this);
static void reset_anim(player_t *this);

/*--- Functions ---*/

player_t *player_ctor(void)
{
	player_t *this;

	logMsg(2, "player: ctor\n");

	this = (player_t *) calloc(1, sizeof(player_t));
	if (!this) {
		return NULL;
	}

	this->dtor = dtor;

	this->load_model = load_model;
	this->get_model_name = get_model_name;

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

	return this;
}

static void dtor(player_t *this)
{
	logMsg(2, "player: dtor\n");

	unload_models(this);

	free(this);
}

static void unload_models(player_t *this)
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

static render_skel_t *load_model(player_t *this, int num_model)
{
	return NULL;
}

static void get_model_name(player_t *this, char name[32])
{
}

static void prev_model(player_t *this)
{
	if (this->num_model>0) {
		--this->num_model;
	}
}

static void next_model(player_t *this)
{
	if (this->num_model<100) {
		++this->num_model;
	}
}

static void reset_model(player_t *this)
{
	this->num_model = 0;
}

static void prev_anim(player_t *this)
{
	if (this->num_anim>0) {
		--this->num_anim;
	}
}

static void next_anim(player_t *this)
{
	int num_anims = this->model->getNumAnims(this->model);

	if (this->num_anim<num_anims-1) {
		++this->num_anim;
	}
}

static void reset_anim(player_t *this)
{
	this->num_anim=0;
}
