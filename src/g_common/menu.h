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

#ifndef MENU_H
#define MENU_H 1

/*--- External types ---*/

typedef struct player_s player_t;
typedef struct game_s game_t;

/*--- Types ---*/

typedef struct menu_s menu_t;

struct menu_s {
	void (*dtor)(menu_t *this);

	void (*init)(menu_t *this, game_t *game, player_t *player);

	void (*draw)(menu_t *this);
};

/*--- Functions protocotypes ---*/

menu_t *menu_ctor(void);

#endif /* MENU_H */
