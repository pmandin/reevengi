/*
	RE3

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

#include <SDL.h>

#include "../log.h"
#include "../parameters.h"

#include "../g_common/room.h"
#include "../g_common/game.h"

#include "game_re3.h"
#include "rdt.h"

/*--- Constants ---*/

static const game_detect_t game_detect[]={
	{GAME_RE3_PC_GAME, "rofs2.dat", "Resident Evil 3: Nemesis, PC"},
	{GAME_RE3_PC_DEMO, "rofs1.dat", "Resident Evil 3: Nemesis Preview, PC"},

	{GAME_RE3_PS1_GAME, "sles_025.28", "Resident Evil 3: Nemesis (UK), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.29", "Resident Evil 3: Nemesis (UK), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.30", "Resident Evil 3: Nemesis (FR), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.31", "Resident Evil 3: Nemesis (DE), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.32", "Resident Evil 3: Nemesis (ES), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.33", "Resident Evil 3: Nemesis (IT), PS1"},
	{GAME_RE3_PS1_GAME, "sles_026.98", "Resident Evil 3: Nemesis (UK), PS1"},
	{GAME_RE3_PS1_GAME, "slps_023.00", "BioHazard 3: Last Escape (JP), PS1"},
	{GAME_RE3_PS1_GAME, "slus_009.23", "Resident Evil 3: Nemesis (US), PS1"},
	{GAME_RE3_PS1_GAME, "slus_900.64", "Resident Evil 3: Nemesis Trial Edition (US), PS1"},

	{-1, "", ""}
};

/*--- Functions prototypes ---*/

static void get_char(game_t *this, int ascii, int *x, int *y, int *w, int *h);

/*--- Functions ---*/

void game_re3_detect(game_t *this)
{
	int i=0;

	logMsg(2, "fs: Detecting RE3 version from %s directory...\n",
		params.basedir);

	while (game_detect[i].version != -1) {
		if (game_file_exists(game_detect[i].filename)) {
			this->major = GAME_RE3;
			this->minor = game_detect[i].version;
			this->name = game_detect[i].name;
			break;
		}
		i++;
	}
}

game_t *game_re3_ctor(game_t *this)
{
	/*room_t *room;*/

	switch(this->minor) {
		case GAME_RE3_PS1_GAME:
			this = game_re3ps1game_ctor(this);
			break;
		case GAME_RE3_PC_DEMO:
		case GAME_RE3_PC_GAME:
			this = game_re3pc_ctor(this);
			break;
	}

	this->get_char = get_char;

	this->room_ctor = rdt3_room_ctor;

#if 0
	if ((params.stage==1) && (params.room==0) && (params.camera==0)) {
		/* Init default room and player pos */
		this->num_room = 13;
	}
#endif

	return this;
}

static void get_char(game_t *this, int ascii, int *x, int *y, int *w, int *h)
{
	*x = *y = 0;
	*w = 8;
	*h = 10;

	if ((ascii<=32) || (ascii>=96+27)) {
		return;
	}

	ascii -= 32;
	*x = 128+ ((ascii & 15)<<3);
	*y = 176+ ((ascii>>4)*10);
}
