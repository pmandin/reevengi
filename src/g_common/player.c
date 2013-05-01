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

/*--- Global variables ---*/

/*--- Variables ---*/


/*--- Functions prototypes ---*/

static void player_shutdown(player_t *this);

static void player_unloadmodels(player_t *this);

static render_skel_t *player_priv_load_model(int num_model);
static void player_priv_get_model_name(char name[32]);

/*--- Functions ---*/

void player_init(player_t *this)
{
	logMsg(2, "player: init\n");

	memset(this, 0, sizeof(player_t));

	this->shutdown = player_shutdown;

	this->priv_load_model = player_priv_load_model;
	this->priv_get_model_name = player_priv_get_model_name;

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

static render_skel_t *player_priv_load_model(int num_model)
{
	return NULL;
}

static void player_priv_get_model_name(char name[32])
{
}
