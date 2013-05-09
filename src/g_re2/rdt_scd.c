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

#include "room.h"
#include "room_rdt2.h"
#include "log.h"
#include "room_rdt2_script_common.h"
#include "room_rdt2_script_dump.h"

typedef struct {
	Uint8 opcode;
	Uint8 length;
} script_inst_len_t;

/*--- Variables ---*/

static const script_inst_len_t inst_length[]={
	/* 0x00-0x0f */
	{INST_NOP,	1},
	{INST_RETURN,	2},
	{INST_DO_EVENTS,	1},
	{INST_RESET,	sizeof(script_reset_t)},
	{INST_EVT_EXEC,	sizeof(script_evtexec_t)},
	{0x05,		2},
	{INST_IF,	sizeof(script_if_t)},
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_END_IF,	2},
	{INST_SLEEP_INIT,	1},
	{INST_SLEEP_LOOP,	sizeof(script_sleep_loop_t)},
	{0x0b,		1},
	{0x0c,		1},
	{INST_BEGIN_LOOP,	sizeof(script_loop_t)},
	{INST_END_LOOP,	2},
	{INST_BEGIN_WHILE,	sizeof(script_begin_while_t)},

	/* 0x10-0x1f */
	{INST_END_WHILE,	2},
	{INST_DO,	sizeof(script_do_t)},
	{INST_WHILE,	sizeof(script_while_t)},
	{INST_BEGIN_SWITCH,	sizeof(script_switch_t)},
	{INST_CASE,	sizeof(script_case_t)},
	{0x15,		2},
	{INST_END_SWITCH,	2},
	{INST_GOTO,	sizeof(script_goto_t)},
	{INST_FUNC,	sizeof(script_func_t)},
	{0x19,		2},
	{INST_BREAK,	2},
	{0x1b,		6},
	{INST_NOP1C,	1},
	{INST_CHG_SCRIPT,	sizeof(script_chg_script_t)},
	{INST_NOP1E,	1},
	{INST_NOP1F,	1},

	/* 0x20-0x2f */
	{INST_NOP20,	1},
	{INST_BIT_TEST,	sizeof(script_bittest_t)},
	{INST_BIT_CHG,	sizeof(script_bitchg_t)},
	{INST_CMP_VARW,	sizeof(script_cmp_varw_t)},
	{INST_SET_VARW,		sizeof(script_set_varw_t)},
	{INST_COPY_VARW,	sizeof(script_copy_varw_t)},
	{INST_OP_VARW_IMM,	sizeof(script_op_varw_imm_t)},
	{INST_OP_VARW,		sizeof(script_op_varw_t)},
	{0x28,		1},
	{INST_CAM_SET,	sizeof(script_cam_set_t)},
	{0x2a,		1},
	{INST_PRINT_TEXT,	sizeof(script_print_text_t)},
	{INST_ESPR3D_SET,	sizeof(script_espr3d_set_t)},
	{INST_TRIGGER_SET,	sizeof(script_trigger_set_t)},
	{INST_SET_REG_MEM,	sizeof(script_setregmem_t)},
	{INST_SET_REG_IMM,	sizeof(script_setregimm_t)},

	/* 0x30-0x3f */
	{INST_SET_REG_TMP,	1},
	{INST_ADD_REG,		1},
	{INST_EM_SET_POS,	sizeof(script_setreg3w_t)},
	{INST_SET_REG3,		sizeof(script_setreg3w_t)},
	{INST_EM_SET_VAR,	sizeof(script_set_var_t)},
	{INST_EM_SET_VAR_VARW,	sizeof(script_em_set_var_varw_t)},
	{0x36,		12},
	{INST_CAM_CHG,	sizeof(script_cam_chg_t)},
	{INST_FLOOR_SET,	sizeof(script_floor_set_t)},
	{0x39,		8},
	{INST_ESPR_SET,	sizeof(script_espr_set_t)},
	{INST_DOOR_SET,	sizeof(script_door_set_t)},
	{INST_STATUS_SET,	sizeof(script_status_set_t)},
	{INST_EM_GET_VAR_VARW,	sizeof(script_em_get_var_varw_t)},
	{INST_CMP_IMM,	sizeof(script_cmp_imm_t)},
	{0x3f,		4},

	/* 0x40-0x4f */
	{0x40,		8},
	{0x41,		10},
	{INST_STATUS_SHOW,	1},
	{0x43,		4},
	{INST_EM_SET,	sizeof(script_em_set_t)},
	{0x45,		5},
	{0x46,		sizeof(script_inst46_t)},
	{INST_ACTIVATE_OBJECT,	sizeof(script_set_cur_obj_t)},
	{0x48,		16},
	{0x49,		8},
	{0x4a,		2},
	{INST_CAMSWITCH_SWAP,	sizeof(script_camswitch_swap_t)},
	{0x4c,		5},
	{0x4d,		22},
	{INST_ITEM_SET,	sizeof(script_item_set_t)},
	{0x4f,		4},

	/* 0x50-0x5f */
	{0x50,		4},
	{INST_SND_SET,	sizeof(script_snd_set_t)},
	{0x52,		6},
	{0x53,		6},
	{0x54,		22},
	{0x55,		6},
	{0x56,		4},
	{0x57,		8},
	{0x58,		4},
	{INST_SND_PLAY,		sizeof(script_snd_play_t)},
	{0x5a,		2},
	{0x5b,		2},
	{0x5c,		3},
	{0x5d,		2},
	{INST_ITEM_HAVE,	sizeof(script_item_have_t)},
	{0x5f,		2},

	/* 0x60-0x6f */
	{0x60,		14},
	{0x61,		4},
	{INST_ITEM_REMOVE,	sizeof(script_item_remove_t)},
	{INST_NOP63,	1},
	{0x64,		16},
	{0x65,		2},
	{0x66,		1},
	{INST_WALL_SET,		sizeof(script_wall_set_t)},
	{0x68,		40},
	{0x69,		30},
	{INST_LIGHT_POS_SET,	sizeof(script_light_pos_set_t)},
	{INST_LIGHT_RANGE_SET,	sizeof(script_light_range_set_t)},
	{0x6c,		1},
	{0x6d,		4},
	{0x6e,		6},
	{INST_MOVIE_PLAY,	sizeof(script_movie_play_t)},

	/* 0x70-0x7f */
	{0x70,		1},
	{0x71,		1},
	{0x72,		16},
	{0x73,		8},
	{0x74,		4},
	{0x75,		22},
	{INST_ITEM_ADD,		sizeof(script_item_add_t)},
	{0x77,		4},
	{0x78,		6},
	{0x79,		1},
	{0x7a,		16},
	{0x7b,		16},
	{INST_LIGHT_COLOR_SET,	sizeof(script_light_color_set_t)},
	{INST_LIGHT_POS_CAM_SET,	sizeof(script_light_pos_cam_set_t)},
	{INST_LIGHT_RANGE_CAM_SET,	sizeof(script_light_range_cam_set_t)},
	{INST_LIGHT_COLOR_CAM_SET,	sizeof(script_light_color_cam_set_t)},
	
	/* 0x80-0x8f */
	{0x80,		2},
	/*{0x81,		3},*/
	{0x82,		3},
	{0x83,		1},
	{0x84,		2},
	{0x85,		6},
	{0x86,		1},
	{0x87,		1},
	{INST_ITEM_HAVE_AND_REMOVE,	sizeof(script_item_have_and_remove_t)},
	{0x89,		1},
	{INST_NOP8A,	6},
	{INST_NOP8B,	6},
	{INST_NOP8C,	8},
	{0x8d,		24},
	{0x8e,		24}
};

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this, int num_script);
static int scriptGetInstLen(Uint8 *curInstPtr);
static void scriptExecInst(room_t *this);

/*--- Functions ---*/

void room_rdt2_scriptInit(room_t *this)
{
	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivExecInst = scriptExecInst;

	this->scriptDump = room_rdt2_scriptDump;
}

static Uint8 *scriptFirstInst(room_t *this, int num_script)
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

static int scriptGetInstLen(Uint8 *curInstPtr)
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

static void scriptExecInst(room_t *this)
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
#if 0
		case INST_ITEM_SET:
			{
				script_item_set_t *itemSet = (script_item_set_t *) inst;
				room_item_t item;

				item.x = SDL_SwapLE16(itemSet->x);
				item.y = SDL_SwapLE16(itemSet->y);
				item.w = SDL_SwapLE16(itemSet->w);
				item.h = SDL_SwapLE16(itemSet->h);

				if (itemSet->type == ITEM_OBSTACLE) {
					room_obstacle_t obstacle;

					obstacle.x = item.x;
					obstacle.y = item.y;
					obstacle.w = item.w;
					obstacle.h = item.h;

					this->addObstacle(this, &obstacle);
				} else {
					this->addItem(this, &item);
				}
			}
			break;
#endif
	}
}
