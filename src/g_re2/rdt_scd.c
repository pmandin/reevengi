/*
	Room description
	RE2 RDT script

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <assert.h>

#include "../log.h"

#include "../g_common/room.h"
#include "../g_common/room_door.h"
#include "../g_common/room_item.h"

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

Uint8 *rdt2_scd_scriptInit(room_t *this, int num_script)
{
	rdt2_header_t *rdt_header;
	Uint32 offset, smaller_offset;
	Uint16 *functionArrayPtr;
	int i, room_script = RDT2_OFFSET_INIT_SCRIPT;

	if (!this) {
		return NULL;
	}
	if (num_script == ROOM_SCRIPT_RUN) {
		room_script = RDT2_OFFSET_ROOM_SCRIPT;
	}

	rdt_header = (rdt2_header_t *) this->file;
	if (!rdt_header) {
		return NULL;
	}
	offset = SDL_SwapLE32(rdt_header->offsets[room_script]);

	this->script_length = this->cur_inst_offset = 0;
	this->cur_inst = NULL;

	if (offset>0) {
		/* Search smaller offset after script to calc length */
		smaller_offset = this->file_length;
		for (i=0; i<21; i++) {
			Uint32 next_offset = SDL_SwapLE32(rdt_header->offsets[i]);
			if ((next_offset>0) && (next_offset<smaller_offset) && (next_offset>offset)) {
				smaller_offset = next_offset;
			}
		}
		if (smaller_offset>offset) {
			this->script_length = smaller_offset - offset;
		}

		/* Start of script is an array of offsets to the various script functions
		 * The first offset also gives the first instruction to execute
		 */
		functionArrayPtr = (Uint16 *) (& ((Uint8 *) this->file)[offset]);

		this->cur_inst_offset = SDL_SwapLE16(functionArrayPtr[0]);
		this->cur_inst = (& ((Uint8 *) this->file)[offset + this->cur_inst_offset]);
	}

	logMsg(1, "rdt2: Script %d at offset 0x%08x, length 0x%04x\n", num_script, offset, this->script_length);

	return this->cur_inst;
}

int rdt2_scd_scriptGetInstLen(room_t *this, Uint8 *curInstPtr)
{
	int i;

	assert(curInstPtr);

	for (i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == curInstPtr[0]) {
			return inst_length[i].length;
		}
	}

	return 0;
}

void rdt2_scd_scriptExecInst(room_t *this)
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
		case INST_DOOR_AOT_SET:
			{
				script_inst_door_aot_set_t *doorSet = (script_inst_door_aot_set_t *) inst;
				room_door_t roomDoor;

				roomDoor.x = SDL_SwapLE16(doorSet->x);
				roomDoor.y = SDL_SwapLE16(doorSet->y);
				roomDoor.w = SDL_SwapLE16(doorSet->w);
				roomDoor.h = SDL_SwapLE16(doorSet->h);

				roomDoor.next_x = SDL_SwapLE16(doorSet->next_x);
				roomDoor.next_y = SDL_SwapLE16(doorSet->next_y);
				roomDoor.next_z = SDL_SwapLE16(doorSet->next_z);
				roomDoor.next_dir = SDL_SwapLE16(doorSet->next_dir);

				roomDoor.next_stage = doorSet->next_stage+1;
				roomDoor.next_room = doorSet->next_room;
				roomDoor.next_camera = doorSet->next_camera;

				this->addDoor(this, &roomDoor);
			}
			break;

		case INST_ITEM_AOT_SET:
			{
				script_inst_item_aot_set_t *itemAotSet = (script_inst_item_aot_set_t *) inst;
				room_item_t item;

				item.x = SDL_SwapLE16(itemAotSet->x);
				item.y = SDL_SwapLE16(itemAotSet->y);
				item.w = SDL_SwapLE16(itemAotSet->w);
				item.h = SDL_SwapLE16(itemAotSet->h);

				/*if (itemAotSet->type == ITEM_OBSTACLE) {
					room_obstacle_t obstacle;

					obstacle.x = item.x;
					obstacle.y = item.y;
					obstacle.w = item.w;
					obstacle.h = item.h;

					this->addObstacle(this, &obstacle);
				} else*/ {
					this->addItem(this, &item);
				}
			}
			break;

		default:
			break;
	}
}
