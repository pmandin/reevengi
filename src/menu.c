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

#include "render_texture.h"
#include "render_skel.h"
#include "state.h"
#include "render_text.h"

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

/*--- Types ---*/

typedef struct {
	int x,y;	/* Top left position (in chars) */
	int strtype;
	void *str;	/* Pointer to string or int value */
} menu_t;

/*--- Constants ---*/

menu_t main_menu[1+6+8]={
	{0,0, STR_TYPE_TEXT, PACKAGE_STRING},

	{0,2, STR_TYPE_TEXT, "Stage :"},
	{8,2, STR_TYPE_INT, &game_state.num_stage},
	{0,3, STR_TYPE_TEXT, "Room  :"},
	{8,3, STR_TYPE_HEXA2, &game_state.num_room},
	{0,4, STR_TYPE_TEXT, "Camera:"},
	{8,4, STR_TYPE_HEXA2, &game_state.num_camera},

	{0,6, STR_TYPE_TEXT, "Player X:"},
	{10,6, STR_TYPE_FLOAT_AS_INT, &game_state.player_x},
	{0,7, STR_TYPE_TEXT, "Player Y:"},
	{10,7, STR_TYPE_FLOAT_AS_INT, &game_state.player_y},
	{0,8, STR_TYPE_TEXT, "Player Z:"},
	{10,8, STR_TYPE_FLOAT_AS_INT, &game_state.player_z},
	{0,9, STR_TYPE_TEXT, "Player A:"},
	{10,9, STR_TYPE_FLOAT_AS_INT, &game_state.player_a}
};

/*--- Functions ---*/

void menu_render(void)
{
	int i, x,y;
	char tmpstr[32];

	x = START_X;
	y = START_Y;
	for (i=0; i<sizeof(main_menu)/sizeof(menu_t); i++) {
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
