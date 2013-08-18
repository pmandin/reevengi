/*
	RE1

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

#ifndef GAME_RE1_H
#define GAME_RE1_H 1

/*--- Enums ---*/

enum {
	GAME_RE1_PS1_DEMO,
	GAME_RE1_PS1_GAME,
	GAME_RE1_PS1_SHOCK,
	GAME_RE1_PC_DEMO,
	GAME_RE1_PC_GAME,
};

/*--- Functions ---*/

void game_re1_detect(game_t *this);

game_t *game_re1_ctor(game_t *this);

game_t *game_re1pc_init(game_t *this);
void room_re1pc_init(room_t *this);

game_t *game_re1ps1_init(game_t *this);
void room_re1ps1_init(room_t *this);

#endif /* GAME_RE1_H */
