/*
	Debug menu

	Copyright (C) 2010	Patrice Mandin

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>

#include "../render_texture.h"
#include "../render_skel.h"
#include "../r_common/render_text.h"

#include "player.h"
#include "menu.h"
#include "game.h"

/*--- Defines ---*/

#define START_X	16
#define START_Y 16

#define MENU_MAIN 0

enum {
	STR_TYPE_TEXT=0,
	STR_TYPE_INT,
	STR_TYPE_HEXA2,
	STR_TYPE_HEXA4,
	STR_TYPE_HEXA8,
	STR_TYPE_FLOAT_AS_INT
};

enum {
	PTR_NULL=0,
	PTR_GAM_STAGE,
	PTR_GAM_ROOM,
	PTR_GAM_CAMERA,
	PTR_PLR_ANIM,
	PTR_PLR_X,
	PTR_PLR_Y,
	PTR_PLR_Z,
	PTR_PLR_A
};

/*--- Types ---*/

typedef struct {
	int x,y;	/* Top left position (in chars) */
	int strtype;
	int vname;	/* Value to initialize pointer */
	void *str;	/* Pointer to string or int value */
} menu_item_t;

/*--- Variables ---*/

static char model_name[32];

/*--- Constants ---*/

menu_item_t main_menu[1+10+8]={
	{0,0, STR_TYPE_TEXT, PTR_NULL, PACKAGE_STRING},

	{0,2, STR_TYPE_TEXT, PTR_NULL, "Stage :"},
	{8,2, STR_TYPE_INT, PTR_GAM_STAGE, NULL},
	{0,3, STR_TYPE_TEXT, PTR_NULL, "Room  :"},
	{8,3, STR_TYPE_HEXA2, PTR_GAM_ROOM, NULL},
	{0,4, STR_TYPE_TEXT, PTR_NULL, "Camera:"},
	{8,4, STR_TYPE_HEXA2, PTR_GAM_CAMERA, NULL},
	{0,5, STR_TYPE_TEXT, PTR_NULL, "Model :"},
	{8,5, STR_TYPE_TEXT, PTR_NULL, model_name},
	{0,6, STR_TYPE_TEXT, PTR_NULL, "Anim  :"},
	{8,6, STR_TYPE_INT, PTR_PLR_ANIM, NULL},

	{0,8, STR_TYPE_TEXT, PTR_NULL, "Player X:"},
	{10,8, STR_TYPE_FLOAT_AS_INT, PTR_PLR_X, NULL},
	{0,9, STR_TYPE_TEXT, PTR_NULL, "Player Y:"},
	{10,9, STR_TYPE_FLOAT_AS_INT, PTR_PLR_Y, NULL},
	{0,10, STR_TYPE_TEXT, PTR_NULL, "Player Z:"},
	{10,10, STR_TYPE_FLOAT_AS_INT, PTR_PLR_Z, NULL},
	{0,11, STR_TYPE_TEXT, PTR_NULL, "Player A:"},
	{10,11, STR_TYPE_FLOAT_AS_INT, PTR_PLR_A, NULL}
};

/*--- Functions prototypes ---*/

static void dtor(menu_t *this);

static void init(menu_t *this, game_t *game, player_t *player);

static void draw(menu_t *this);

/*--- Functions ---*/

menu_t *menu_ctor(void)
{
	menu_t *this;

	this = (menu_t *) calloc(1, sizeof(menu_t));
	this->dtor = dtor;
	this->init = init;
	this->draw = draw;

	return this;
}

static void dtor(menu_t *this)
{
	free(this);
}

static void init(menu_t *this, game_t *game, player_t *player)
{
	int i;

	for (i=0; i<sizeof(main_menu)/sizeof(menu_item_t); i++) {
		if (!main_menu[i].vname) {
			continue;
		}

		switch(main_menu[i].vname) {
			case PTR_GAM_STAGE:
				main_menu[i].str = &game->num_stage;
				break;
			case PTR_GAM_ROOM:
				main_menu[i].str = &game->num_room;
				break;
			case PTR_GAM_CAMERA:
				main_menu[i].str = &game->num_camera;
				break;
			case PTR_PLR_ANIM:
				main_menu[i].str = &player->num_anim;
				break;
			case PTR_PLR_X:
				main_menu[i].str = &player->x;
				break;
			case PTR_PLR_Y:
				main_menu[i].str = &player->y;
				break;
			case PTR_PLR_Z:
				main_menu[i].str = &player->z;
				break;
			case PTR_PLR_A:
				main_menu[i].str = &player->a;
				break;
		}
	}
}

static void draw(menu_t *this)
{
	player_t *player = game->player;

	int i, x,y;
	char tmpstr[32];

	player->get_model_name(player, model_name);

	x = START_X;
	y = START_Y;
	for (i=0; i<sizeof(main_menu)/sizeof(menu_item_t); i++) {
		int px = x + (main_menu[i].x*8);
		int py = y + (main_menu[i].y*11);
		char *str = (char *) main_menu[i].str;

		switch(main_menu[i].strtype) {
			case STR_TYPE_INT:
				{
					int *ptr_int = (int *) main_menu[i].str;
					sprintf(tmpstr,"%d", *ptr_int);
					str = tmpstr;
				}
				break;
			case STR_TYPE_HEXA2:
				{
					int *ptr_int = (int *) main_menu[i].str;
					sprintf(tmpstr,"%02x", *ptr_int);
					str = tmpstr;
				}
				break;
			case STR_TYPE_HEXA4:
				{
					int *ptr_int = (int *) main_menu[i].str;
					sprintf(tmpstr,"%04x", *ptr_int);
					str = tmpstr;
				}
				break;
			case STR_TYPE_HEXA8:
				{
					int *ptr_int = (int *) main_menu[i].str;
					sprintf(tmpstr,"%08x", *ptr_int);
					str = tmpstr;
				}
				break;
			case STR_TYPE_FLOAT_AS_INT:
				{
					float *ptr_flt = (float *) main_menu[i].str;
					sprintf(tmpstr,"%.0f", *ptr_flt);
					str = tmpstr;
				}
				break;
			case STR_TYPE_TEXT:
			default:
				break;
		}

		render_text(str, px, py);
	}
}
