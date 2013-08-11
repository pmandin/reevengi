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

#include "rdt.h"
#include "rdt_scd.h"
#include "rdt_scd_common.h"
#include "rdt_scd_dump.h"

/*--- Types ---*/

typedef struct {
	Uint8 opcode;
	Uint8 length;
} script_inst_len_t;

/*--- Variables ---*/

static const script_inst_len_t inst_length[]={
	{INST_NOP,	2},
	{INST_IF,	sizeof(script_if_t)},
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_END_IF,	sizeof(script_endif_t)},
	{INST_BIT_TEST,	sizeof(script_bit_test_t)},
	{INST_BIT_OP,	4},
	{INST_CMP06,	4},
	{INST_CMP07,	6},
	{INST_STAGEROOMCAM_SET,	sizeof(script_stageroomcam_set_t)},
	{0x09,	2},
	{0x0a,	2},
	{INST_PRINT_MSG,	sizeof(script_printmsg_t)},
	{INST_DOOR_SET,	sizeof(script_door_set_t)},
	{INST_ITEM_SET,	sizeof(script_item_set_t)},
	{INST_NOP0E,	sizeof(script_nop0e_t)},
	{0x0f,	8},

	{INST_CMP10,	2},
	{INST_CMP11,	2},
	{0x12,	10},
	{0x13,	4},
	{0x14,	4},
	{0x15,	2},
	{0x16,	2},
	{0x17,	10},
	{INST_ITEM_MODEL_SET,	sizeof(script_item_model_set_t)},
	{0x19,	4},
	{0x1a,	2},
	{INST_EM_SET,	22},
	{0x1c,	6},
	{0x1d,	2},
	{0x1e,	4},
	{INST_OM_SET,	sizeof(script_om_set_t)},

	{INST_PLAYER_POS_SET,	sizeof(script_player_pos_set_t)},
	{0x21,	14},
	{0x22,	4},
	{0x23,	2},
	{0x24,	4},
	{0x25,	4},
	{0x27,	2},
	/*{0x28,	6},*/
	{0x29,	2},
	{0x2a,	12},
	{0x2b,	4},
	{0x2c,	2},
	{0x2d,	4},
	{0x2f,	4},

	{0x30,	12},
	{0x31,	4},
	{0x32,	4},
	/*{0x33,	12},*/
	{0x34,	8},
	{0x35,	4},
	{0x36,	4},
	{0x37,	4},
	{0x38,	4},
	{0x39,	2},
	{0x3a,	4},
	{0x3b,	6},
	{0x3c,	6},
	{0x3d,	12},
	{0x3e,	2},
	{0x3f,	6},

	{0x40,	16},
	{0x41,	4},
	{0x42,	4},
	{0x43,	4},
	{0x44,	2},
	{0x45,	2},
	{0x46,	2+(12*3)+6},
	{0x47,	14},
	{0x48,	2},
	{0x49,	2},
	{0x4a,	2},
	{0x4b,	2},
	{0x4c,	4},
	{0x4d,	2},
	{0x4e,	4},
	{0x4f,	2},

	{0x50,	2}
};

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
		case 0x28:
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
		case 0x33:
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
				script_door_set_t *doorSet = (script_door_set_t *) inst;
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
				if (next_stage==0) {
					next_stage = 1;
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
				script_item_set_t *itemSet = (script_item_set_t *) inst;
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
