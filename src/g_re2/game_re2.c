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

#include <SDL.h>

#include "../log.h"
#include "../parameters.h"

#include "../g_common/game.h"

#include "game_re2.h"

/*--- Constants ---*/

static const game_detect_t game_detect[]={
	{GAME_RE2_PC_GAME_LEON, "PL0/ZMOVIE/R108L.BIN", "Resident Evil 2 [LEON], PC"},
	{GAME_RE2_PC_GAME_CLAIRE, "PL1/ZMOVIE/R108C.BIN", "Resident Evil 2 [CLAIRE], PC"},
	{GAME_RE2_PC_DEMO_P, "Regist/LeonP.exe", "Resident Evil 2 Preview, PC"},
	{GAME_RE2_PC_DEMO_U, "regist/leonu.exe", "Resident Evil 2 Preview, PC"},

	{GAME_RE2_PS1_DEMO, "sced_003.60", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_DEMO, "sced_008.27", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_DEMO, "sled_009.77", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_DEMO, "slps_009.99", "BioHazard 2 Trial Edition (JP), PS1"},
	{GAME_RE2_PS1_DEMO, "slus_900.09", "Resident Evil 2 Preview (US), PS1"},
	{GAME_RE2_PS1_DEMO2, "sced_011.14", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.72", "Resident Evil 2 [LEON] (UK), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.73", "Resident Evil 2 [LEON] (FR), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.74", "Resident Evil 2 [LEON] (DE), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.75", "Resident Evil 2 [LEON] (IT), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slps_012.22", "BioHazard 2 [LEON] (JP), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slps_015.10", "BioHazard 2 Dual Shock [LEON] (JP), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slus_004.21", "Resident Evil 2 [LEON] (US), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slus_007.48", "Resident Evil 2 Dual Shock [LEON] (US), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.72", "Resident Evil 2 [CLAIRE] (UK), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.73", "Resident Evil 2 [CLAIRE] (FR), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.74", "Resident Evil 2 [CLAIRE] (DE), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.75", "Resident Evil 2 [CLAIRE] (IT), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slps_012.23", "BioHazard 2 [CLAIRE] (JP), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slps_015.11", "BioHazard 2 Dual Shock [CLAIRE] (JP), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slus_005.92", "Resident Evil 2 [CLAIRE] (US), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slus_007.56", "Resident Evil 2 Dual Shock [CLAIRE] (US), PS1"},

	{-1, "", ""}
};

/*--- Functions ---*/

void game_re2_detect(game_t *this)
{
	int i=0;

	logMsg(2, "fs: Detecting RE2 version from %s directory...\n",
		params.basedir);

	while (game_detect[i].version != -1) {
		if (game_file_exists(game_detect[i].filename)) {
			this->major = GAME_RE2;
			this->minor = game_detect[i].version;
			break;
		}
		i++;
	}
}

void game_re2_init(game_t *this)
{
	switch(this->minor) {
		case GAME_RE2_PS1_DEMO:
		case GAME_RE2_PS1_DEMO2:
		case GAME_RE2_PS1_GAME_LEON:
		case GAME_RE2_PS1_GAME_CLAIRE:
			game_re2ps1_init(this);
			break;
		case GAME_RE2_PC_DEMO_P:
		case GAME_RE2_PC_DEMO_U:
			game_re2pcdemo_init(this);
			if (params.viewmode == VIEWMODE_MOVIE) {
				logMsg(1, "No movies to play\n");
				params.viewmode = VIEWMODE_BACKGROUND;
			}
			break;
		case GAME_RE2_PC_GAME_LEON:
		case GAME_RE2_PC_GAME_CLAIRE:
			game_re2pcgame_init(this);
			break;
	}

#if 0
	/* Init default room and player pos */
	this->player.x = -1530.0f;
	this->player.y = 2020.0f;
	this->player.z = 2700.0f;
	this->player.a = 3072.0f;
#endif
}
