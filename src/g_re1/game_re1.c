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

#include <SDL.h>

#include "../log.h"
#include "../parameters.h"

#include "../g_common/player.h"
#include "../g_common/room.h"
#include "../g_common/game.h"

#include "game_re1.h"
#include "rdt.h"
#include "rdt_rid.h"
#include "rdt_rvd.h"
#include "rdt_pri.h"
#include "rdt_scd.h"
#include "rdt_scd_dump.h"

/*--- Constants ---*/

static const game_detect_t game_detect[]={
	{GAME_RE1_PC_GAME, "horr/usa/data/capcom.ptc", "Resident Evil (US), PC"},
	{GAME_RE1_PC_GAME, "horr/ger/data/capcom.ptc", "Resident Evil (DE), PC"},
	{GAME_RE1_PC_GAME, "horr/jpn/data/capcom.ptc", "Resident Evil (JP), PC"},
	{GAME_RE1_PC_GAME, "usa/data/capcom.ptc", "Resident Evil (US), PC"},
	{GAME_RE1_PC_GAME, "ger/data/capcom.ptc", "Resident Evil (DE), PC"},
	{GAME_RE1_PC_GAME, "jpn/data/capcom.ptc", "Resident Evil (JP), PC"},

	{GAME_RE1_PS1_DEMO, "slpm_800.27", "BioHazard Trial Edition (JP), PS1"},
	{GAME_RE1_PS1_GAME, "ntsc.exe", "BioHazard 1.0 (JP), PS1"},
	{GAME_RE1_PS1_GAME, "sles_002.00", "Resident Evil (UK), PS1"},
	{GAME_RE1_PS1_GAME, "sles_002.27", "Resident Evil (FR), PS1"},
	{GAME_RE1_PS1_GAME, "sles_002.28", "Resident Evil (DE), PS1"},
	{GAME_RE1_PS1_GAME, "sles_009.69", "Resident Evil Director's Cut (UK), PS1"},
	{GAME_RE1_PS1_GAME, "sles_009.70", "Resident Evil Director's Cut (FR), PS1"},
	{GAME_RE1_PS1_GAME, "sles_009.71", "Resident Evil Director's Cut (DE), PS1"},
	{GAME_RE1_PS1_GAME, "slpm_867.70", "BioHazard 5th Anniversary LE (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slps_002.22", "BioHazard (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slps_009.98", "BioHazard Director's Cut (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slps_015.12", "BioHazard Director's Cut Dual Shock (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slus_001.70", "Resident Evil (US), PS1"},
	{GAME_RE1_PS1_GAME, "slus_005.51", "Resident Evil Director's Cut (US), PS1"},
	{GAME_RE1_PS1_SHOCK, "slus_007.47", "Resident Evil Director's Cut Dual Shock (US), PS1"},

	{-1, "", ""}
};

/*--- Functions prototypes ---*/

static void get_char(game_t *this, int ascii, int *x, int *y, int *w, int *h);
static void get_model_name(player_t *this, char name[32]);

/*--- Functions ---*/

void game_re1_detect(game_t *this)
{
	int i=0;

	logMsg(2, "fs: Detecting RE1 version from %s directory...\n",
		params.basedir);

	while (game_detect[i].version != -1) {
		if (game_file_exists(game_detect[i].filename)) {
			this->major = GAME_RE1;
			this->minor = game_detect[i].version;
			this->name = game_detect[i].name;
			break;
		}
		i++;
	}
}

game_t *game_re1_ctor(game_t *this)
{
	room_t *room;

	switch(this->minor) {
		case GAME_RE1_PS1_DEMO:
		case GAME_RE1_PS1_GAME:
			this = game_re1ps1_ctor(this);
			break;
		case GAME_RE1_PC_DEMO:
		case GAME_RE1_PC_GAME:
			this = game_re1pc_ctor(this);
			break;
	}

	this->get_char = get_char;
	this->player->get_model_name = get_model_name;

	room = this->room;

	room->init = rdt1_init;

	room->getNumCameras = rdt1_rid_getNumCameras;
	room->getCamera = rdt1_rid_getCamera;

	room->getNumCamSwitches = rdt1_rvd_getNumCamSwitches;
	room->getCamSwitch = rdt1_rvd_getCamSwitch;

	room->getNumBoundaries = rdt1_rvd_getNumBoundaries;
	room->getBoundary = rdt1_rvd_getBoundary;

	room->initMasks = rdt1_pri_initMasks;
	room->drawMasks = rdt1_pri_drawMasks;

	room->scriptInit = rdt1_scd_scriptInit;
	room->scriptGetInstLen = rdt1_scd_scriptGetInstLen;
	room->scriptExecInst = rdt1_scd_scriptExecInst;

	room->scriptDump = rdt1_scd_scriptDump;

#if 0
	/* Init default room and player pos */
	this->num_room = 6;
#endif

	return this;
}

static void get_char(game_t *this, int ascii, int *x, int *y, int *w, int *h)
{
	*x = *y = 0;
	*w = *h = 8;

	if ((ascii<=32) || (ascii>=96+27)) {
		return;
	}

	ascii -= 32;
	*x = (ascii & 31)<<3;
	*y = (ascii>>5)<<3;
}

static void get_model_name(player_t *this, char name[32])
{
	const char *filename = "char1%d.emd";
	int num_model = this->num_model;

	if (num_model>64) { /* 66 on pc ? */
		num_model = 64;
	}

	if (num_model>0x03) {
		filename = "em10%02x.emd";
		num_model -= 4;
		if (num_model>0x15) {
			num_model += 0x20-0x16;
		}
		if (num_model>0x2e) {
			num_model += 1;
		}
	}
	if (num_model>0x31) {
		filename = "em11%02x.emd";
		num_model -= 0x32;
	}

	sprintf(name, filename, num_model);
}
