/*
	Room description
	RE3 RDT manager

	Copyright (C) 2009-2013	Patrice Mandin

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
#include "rdt_sca.h"
#include "rdt_scd.h"
#include "rdt_scd_dump.h"

#include "../g_re2/rdt_rid.h"
#include "../g_re2/rdt_rvd.h"
#include "../g_re2/rdt_pri.h"
#include "../g_re2/rdt_msg.h"

/*--- Types ---*/

/*--- Functions prototypes ---*/

static void postLoad(room_t *this);
static void displayTexts(room_t *this, int num_lang);

/*--- Functions ---*/

room_t *rdt3_room_ctor(game_t *this, int num_stage, int num_room)
{
	room_t *room;

	room = room_ctor(this, num_stage, num_room);
	if (!room) {
		return NULL;
	}

	room->postLoad = postLoad;

	room->getNumCameras = rdt2_rid_getNumCameras;
	room->getCamera = rdt2_rid_getCamera;

	room->getNumCamSwitches = rdt2_rvd_getNumCamSwitches;
	room->getCamSwitch = rdt2_rvd_getCamSwitch;

	room->getNumBoundaries = rdt2_rvd_getNumBoundaries;
	room->getBoundary = rdt2_rvd_getBoundary;

	room->initMasks = rdt2_pri_initMasks;
	room->drawMasks = rdt2_pri_drawMasks;

	room->getText = rdt2_msg_getText;

	room->scriptInit = rdt3_scd_scriptInit;
	room->scriptGetInstLen = rdt3_scd_scriptGetInstLen;
	room->scriptExecInst = rdt3_scd_scriptExecInst;

	room->scriptDump = rdt3_scd_scriptDump;

	room->getNumCollisions = rdt3_sca_getNumCollisions;
	room->drawMapCollision = rdt3_sca_drawMapCollision;

	switch(this->minor) {
		case GAME_RE3_PS1_GAME:
			room_re3ps1game_init(room);
			break;
		case GAME_RE3_PC_DEMO:
		case GAME_RE3_PC_GAME:
			room_re3pc_init(room);
			break;
	}

	return room;
}

static void postLoad(room_t *this)
{
	/*rdt3_sca_init(room);*/

	displayTexts(this, 0);
	/*displayTexts(this, 1);*/

	/* Dump scripts if wanted */
	if (params.dump_script) {
		this->scriptDump(this, ROOM_SCRIPT_INIT);
		this->scriptDump(this, ROOM_SCRIPT_RUN);
	}
}

static void displayTexts(room_t *this, int num_lang)
{
	rdt3_header_t *rdt_header;
	int room_lang = (num_lang==0) ? RDT3_OFFSET_TEXT_LANG1 : RDT3_OFFSET_TEXT_LANG2;
	Uint32 offset;
	Uint16 *txtOffsets, txtCount;
	int i;
	char tmpBuf[512];

	logMsg(1, "Language %d\n", num_lang);

	rdt_header = (rdt3_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[room_lang]);
	if (offset == 0) {
		logMsg(1, " No texts to display\n");
		return;
	}

	txtOffsets = (Uint16 *) &((Uint8 *) this->file)[offset];

	txtCount = SDL_SwapLE16(txtOffsets[0]) >> 1;
	for (i=0; i<txtCount; i++) {
		this->getText(this, num_lang, i, tmpBuf, sizeof(tmpBuf));
		logMsg(1, " Text[0x%02x]: %s\n", i, tmpBuf);
	}
}
