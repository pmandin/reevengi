/*
	RE1 SCD
	Game scripts

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <assert.h>

#include "../log.h"

#include "../g_common/room.h"
#include "../g_common/room_door.h"
#include "../g_common/game.h"

#include "rdt.h"
#include "rdt_scd.h"
#include "rdt_scd_dump.h"

#include "rdt_scd_defs.gen.h"

/*--- Types ---*/

#include "rdt_scd_types.gen.h"

typedef struct {
	Uint8 opcode;
	Uint8 length;
} script_inst_len_t;

/*--- Variables ---*/

#include "rdt_scd_lengths.gen.c"

/*--- Functions ---*/

Uint8 *rdt1_scd_scriptInit(room_t *this, int num_script)
{
	rdt1_header_t *rdt_header;
	Uint32 offset;
	int room_script = RDT1_OFFSET_INIT_SCRIPT;
	Uint8 *scriptPtr;

	if (!this) {
		return NULL;
	}

	if (num_script == ROOM_SCRIPT_RUN) {
		room_script = RDT1_OFFSET_ROOM_SCRIPT;
	}

	rdt_header = (rdt1_header_t *) this->file;
	if (!rdt_header) {
		return NULL;
	}

	offset = SDL_SwapLE32(rdt_header->offsets[room_script]);
	scriptPtr = & (((Uint8 *) this->file)[offset]);

	this->script_length = SDL_SwapLE16(*((Uint16 *) scriptPtr));
	this->cur_inst_offset = 0;
	this->cur_inst = &scriptPtr[2];

	logMsg(1, "rdt1: Script %d at offset 0x%08x, length 0x%04x\n", num_script, offset, this->script_length);

	return this->cur_inst;
}

int rdt1_scd_scriptGetInstLen(room_t *this, Uint8 *curInstPtr)
{
	int i;

	assert(curInstPtr);

	for (i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == curInstPtr[0]) {
			return inst_length[i].length;
		}
	}

	/* Variable length instructions */
	switch(curInstPtr[0]) {
		case INST_17:
			switch(curInstPtr[4]) {
				case 0:
					/* fields used */
					return 6+4;
				case 1:
				case 2:
				case 3:
					/* fields not used */
					return 6+4;
				default:
					return 6;
			}
			break;
		case INST_28:
			switch(curInstPtr[2]) {
				case 0:
				case 2:
				case 3:
				case 5:
				case 9:
				case 10:
					return 6;
				case 1:
					return 8;
				case 6:
				case 8:
					return 4;
				case 4:
				default:
					break;
			}
			break;
		case INST_33:
			switch(curInstPtr[1]) {
				case 0:
				case 4:
				case 6:
				case 7:
					return 2;
				case 1:
				case 3:
				case 5:
				case 8:
				case 9:
				case 10:
					return 4;
				case 2:
				default:
					break;
			}
			break;
		default:
			break;
	}

	return 0;
}

void rdt1_scd_scriptExecInst(room_t *this)
{
	script_inst_t *inst;

	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	inst = (script_inst_t *) this->cur_inst;

	switch(inst->opcode) {
		case INST_DOOR_SET:
			{
				script_inst_door_set_t *doorSet = (script_inst_door_set_t *) inst;
				room_door_t roomDoor;
				int next_stage, next_room;

				roomDoor.x = SDL_SwapLE16(doorSet->x);
				roomDoor.y = SDL_SwapLE16(doorSet->y);
				roomDoor.w = SDL_SwapLE16(doorSet->w);
				roomDoor.h = SDL_SwapLE16(doorSet->h);

				roomDoor.next_x = SDL_SwapLE16(doorSet->next_x);
				roomDoor.next_y = SDL_SwapLE16(doorSet->next_y);
				roomDoor.next_z = SDL_SwapLE16(doorSet->next_z);
				roomDoor.next_dir = SDL_SwapLE16(doorSet->next_dir);

				next_stage = doorSet->next_stage_and_room>>5;
				switch(next_stage) {
					case 0:
					default:
						next_stage = game->num_stage;
						break;
					case 1:
						next_stage = game->num_stage-1;
						break;
					case 2:
						next_stage = game->num_stage+1;
						break;
				}
				roomDoor.next_stage = next_stage;

				roomDoor.next_room = doorSet->next_stage_and_room & 31;

				roomDoor.next_camera = 0/*doorSet->next_camera & 7*/;

				this->addDoor(this, &roomDoor);
			}
			break;
#if 0
		case INST_ITEM_SET:
			{
				script_inst_item_set_t *itemSet = (script_item_set_t *) inst;
				room_item_t item;

				item.x = SDL_SwapLE16(itemSet->x);
				item.y = SDL_SwapLE16(itemSet->y);
				item.w = SDL_SwapLE16(itemSet->w);
				item.h = SDL_SwapLE16(itemSet->h);

				this->addItem(this, &item);
			}
			break;
#endif
		default:
			break;
	}
}
