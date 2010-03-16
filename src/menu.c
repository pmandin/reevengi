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
	STR_TYPE_HEXA8
};

/*--- Types ---*/

typedef struct {
	int x,y;	/* Top left position (in chars) */
	int strtype;
	void *str;	/* Menu item, or ptr to int value */
} menu_t;

/*--- Constants ---*/

menu_t main_menu[7]={
	{0,0, STR_TYPE_TEXT, PACKAGE_STRING},
	{0,2, STR_TYPE_TEXT, "Stage :"},
	{8,2, STR_TYPE_INT, &game_state.num_stage},
	{0,3, STR_TYPE_TEXT, "Room  :"},
	{8,3, STR_TYPE_HEXA2, &game_state.num_room},
	{0,4, STR_TYPE_TEXT, "Camera:"},
	{8,4, STR_TYPE_HEXA2, &game_state.num_camera}
};

/*--- Functions ---*/

void menu_render(void)
{
	int i, x,y;
	char tmpstr[16];

	x = START_X;
	y = START_Y;
	for (i=0; i<sizeof(main_menu)/sizeof(menu_t); i++) {
		int px = x + (main_menu[i].x*8);
		int py = y + (main_menu[i].y*11);
		char *str = (char *) main_menu[i].str;
		int *ptr_int = (int *) main_menu[i].str;

		switch(main_menu[i].strtype) {
			case STR_TYPE_TEXT:
				break;
			case STR_TYPE_INT:
				sprintf(tmpstr,"%d", *ptr_int);
				str = tmpstr;
				break;
			case STR_TYPE_HEXA2:
				sprintf(tmpstr,"%02x", *ptr_int);
				str = tmpstr;
				break;
			case STR_TYPE_HEXA4:
				sprintf(tmpstr,"%04x", *ptr_int);
				str = tmpstr;
				break;
			case STR_TYPE_HEXA8:
				sprintf(tmpstr,"%08x", *ptr_int);
				str = tmpstr;
				break;
		}

		render_text(str, px, py);
	}
}
