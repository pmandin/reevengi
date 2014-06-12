/*
	Room description
	RE1 RDT manager

	Copyright (C) 2009	Patrice Mandin

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

#include "game_re1.h"
#include "rdt.h"
#include "rdt_sca.h"
#include "rdt_msg.h"
#include "rdt_rid.h"
#include "rdt_rvd.h"
#include "rdt_pri.h"
#include "rdt_scd.h"
#include "rdt_scd_dump.h"
#include "rdt_evt.h"
#include "rdt_evt_dump.h"

/*--- Functions prototypes ---*/

static void postLoad(room_t *this);
static void displayTexts(room_t *this, int num_lang);

/*--- Functions ---*/

room_t *rdt1_room_ctor(game_t *this, int num_stage, int num_room)
{
	room_t *room;

	room = room_ctor(this, num_stage, num_room);
	if (!room) {
		return NULL;
	}

	room->postLoad = postLoad;

	room->getNumCameras = rdt1_rid_getNumCameras;
	room->getCamera = rdt1_rid_getCamera;

	room->getNumCamSwitches = rdt1_rvd_getNumCamSwitches;
	room->getCamSwitch = rdt1_rvd_getCamSwitch;

	room->getNumBoundaries = rdt1_rvd_getNumBoundaries;
	room->getBoundary = rdt1_rvd_getBoundary;

	room->initMasks = rdt1_pri_initMasks;
	room->drawMasks = rdt1_pri_drawMasks;

	room->getText = rdt1_msg_getText;

	room->scriptInit = rdt1_scd_scriptInit;
	room->scriptGetInstLen = rdt1_scd_scriptGetInstLen;
	room->scriptExecInst = rdt1_scd_scriptExecInst;

	room->scriptDump = rdt1_scd_scriptDump;

	room->getNumCollisions = rdt1_sca_getNumCollisions;
	room->drawMapCollision = rdt1_sca_drawMapCollision;

	switch(this->minor) {
		case GAME_RE1_PS1_DEMO:
		case GAME_RE1_PS1_GAME:
			room_re1ps1_init(room);
			break;
		case GAME_RE1_PC_DEMO:
		case GAME_RE1_PC_GAME:
			room_re1pc_init(room);
			break;
	}

	return room;
}

static void postLoad(room_t *this)
{
	/*rdt1_header_t *rdt_header;
	int i;

	rdt_header = (rdt1_header_t *) this->file;
	logMsg(1, "RDT at 0x%08x, length %d\n", this->file, this->file_length);
	for (i=0; i<19; i++) {
		logMsg(1, "RDT header[%d]: 0x%08x\n", i, rdt_header->offsets[i]);
	}*/

	rdt1_sca_init(this);

	displayTexts(this, 0);

	/* Dump scripts if wanted */
	if (params.dump_script) {
		this->scriptDump(this, ROOM_SCRIPT_INIT);
		this->scriptDump(this, ROOM_SCRIPT_RUN);

		rdt1_evt_scriptDump(this);	/* FIXME: move to this->scriptDump */
	}
}

static void displayTexts(room_t *this, int num_lang)
{
	rdt1_header_t *rdt_header;
	int room_lang = RDT1_OFFSET_TEXT /*(num_lang==0) ? RDT2_OFFSET_TEXT_LANG1 : RDT2_OFFSET_TEXT_LANG2*/;
	Uint32 offset;
	Uint16 *txtOffsets, txtCount;
	int i;
	char tmpBuf[512];

	logMsg(1, "Language %d\n", num_lang);

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[room_lang]);
	if (offset == 0) {
		logMsg(1, " No texts to display\n");
		return;
	}

	logMsg(2, "txt offset: 0x%08x\n", offset);

	txtOffsets = (Uint16 *) &((Uint8 *) this->file)[offset];

	txtCount = SDL_SwapLE16(txtOffsets[0]) >> 1;
	for (i=0; i<txtCount; i++) {
		this->getText(this, num_lang, i, tmpBuf, sizeof(tmpBuf));
		logMsg(1, " Text[0x%02x]: %s\n", i, tmpBuf);
	}
}
