/*
	RE2

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

#ifndef GAME_RE2_H
#define GAME_RE2_H 1

/*--- Enums ---*/

enum {
	GAME_RE2_PS1_DEMO,
	GAME_RE2_PS1_DEMO2,
	GAME_RE2_PS1_GAME_LEON,
	GAME_RE2_PS1_GAME_CLAIRE,
	GAME_RE2_PC_DEMO_P,
	GAME_RE2_PC_DEMO_U,
	GAME_RE2_PC_GAME_LEON,
	GAME_RE2_PC_GAME_CLAIRE
};

/*--- Functions ---*/

void game_re2_detect(game_t *this);
void game_re2_init(game_t *this);

void game_re2pcdemo_init(game_t *this);
void game_re2pcgame_init(game_t *this);
void game_re2ps1_init(game_t *this);

#endif /* GAME_RE2_H */
