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

#include <SDL.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "room.h"
#include "room_rdt2.h"
#include "log.h"

/*--- Defines ---*/

/* Instructions */

#define INST_NOP	0x00
#define INST_RETURN	0x01
#define INST_EVT_EXEC	0x04
#define INST_IF		0x06
#define INST_ELSE	0x07
#define INST_END_IF	0x08
#define INST_SLEEP_N	0x0a

#define INST_FUNC	0x18
#define INST_NOP1C	0x1c
#define INST_NOP1E	0x1e
#define INST_NOP1F	0x1f

#define INST_NOP20	0x20
#define INST_EVAL_CK	0x21
#define INST_EVAL_CMP	0x23
#define INST_PRINT_TEXT	0x2b
#define INST_ITEM_SET	0x2c
#define INST_SET_REG_MEM	0x2e
#define INST_SET_REG_IMM	0x2f

#define INST_SET_REG_TMP	0x30
#define INST_ADD_REG	0x31
#define INST_SET_REG2	0x32
#define INST_SET_REG3	0x33
#define INST_DOOR_SET	0x3b

#define INST_EM_SET	0x44

#define INST_NOP63	0x63

/* Item types */

#define ITEM_LIGHT	0x03
#define ITEM_OBSTACLE	0x04
#define ITEM_TYPEWRITER	0x09
#define ITEM_BOX	0x0a
#define ITEM_FIRE	0x0b

/*--- Types ---*/

typedef struct {
	Uint8 type;
	Uint8 unknown[3];
} script_condition_t;

typedef struct {
	Uint8 opcode;
	Uint8 cond;
	Uint8 ex_opcode;	/* 0x18: execute function */
	Uint8 num_func;
} script_evtexec_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 block_length;
} script_if_t;	/* always followed by script_condition_t */

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 block_length;
} script_else_t;

typedef struct {
	Uint8 opcode;
	Uint8 delay;
	Uint8 unknown;
} script_sleepn_t;

typedef struct {
	Uint8 opcode;
	Uint8 num_func;
} script_func_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 type;
	Uint8 unknown0[3];
	Sint16 x,y,w,h;
	Uint16 unknown1[3];
} script_item_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 id;
	Uint8 unknown1[3];
	Sint16 x,y,w,h;
	Sint16 next_x,next_y,next_z;
	Sint16 next_dir;
	Uint8 next_stage,next_room,next_camera;
	Uint8 unknown2;
	Uint8 door_type;
	Uint8 door_lock;
	Uint8 unknown3;
	Uint8 door_locked;
	Uint8 door_key;
	Uint8 unknown4;
} script_door_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 id;
	Uint8 unknown1[3];
} script_print_text_t;

typedef struct {
	Uint8 opcode;
	Uint8 component;
	Uint8 index;
} script_setregmem_t;

typedef struct {
	Uint8 opcode;
	Uint8 component;
	Sint16 value;
} script_setregimm_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown;
	Sint16 value[3];
} script_setreg3w_t;

typedef union {
	Uint8 opcode;
	script_evtexec_t	evtexec;
	script_if_t	i_if;
	script_else_t	i_else;
	script_sleepn_t	sleepn;
	script_func_t	func;
	script_door_set_t	door_set;
	script_item_set_t	item_set;
	script_print_text_t	print_text;
	script_setregmem_t	set_reg_mem;
	script_setregimm_t	set_reg_imm;
	script_setreg3w_t	set_reg_3w;
} script_inst_t;

typedef struct {
	Uint8 opcode;
	Uint8 length;
} script_inst_len_t;

/*--- Variables ---*/

static const script_inst_len_t inst_length[]={
	/* 0x00-0x0f */
	{INST_NOP,	1},
	{INST_RETURN,	2},
	{0x02,		1},
	{0x03,		2},
	{INST_EVT_EXEC,	sizeof(script_evtexec_t)},
	{0x05,		2},
	{INST_IF,	sizeof(script_if_t)},
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_END_IF,	2},
	{0x09,		1},
	{INST_SLEEP_N,	sizeof(script_sleepn_t)},
	{0x0b,		1},
	{0x0c,		1},
	{0x0d,		6},
	{0x0e,		2},
	{0x0f,		2},

	/* 0x10-0x1f */
	{0x10,		2},
	{0x11,		4},
	{0x12,		2},
	{0x13,		4},
	{0x14,		6},
	{0x16,		2},
	{0x17,		6},
	{INST_FUNC,	sizeof(script_func_t)},
	/*{0x19,		2},*/
	{0x1a,		2},
	{0x1b,		6},
	{INST_NOP1C,	1},
	{0x1d,		4},
	{INST_NOP1E,	1},
	{INST_NOP1F,	1},

	/* 0x20-0x2f */
	{INST_NOP20,	1},
	{INST_EVAL_CK,	4},
	{0x22,		4},
	{INST_EVAL_CMP,	6},
	{0x24,		4},
	{0x25,		3},
	{0x26,		6},
	{0x27,		4},
	{0x28,		1},
	{0x29,		2},
	{0x2a,		1},
	{INST_PRINT_TEXT,	sizeof(script_print_text_t)},
	{INST_ITEM_SET,		sizeof(script_item_set_t)},
	{0x2d,		38},
	{INST_SET_REG_MEM,	sizeof(script_setregmem_t)},
	{INST_SET_REG_IMM,	sizeof(script_setregimm_t)},

	/* 0x30-0x3f */
	{INST_SET_REG_TMP,	1},
	{INST_ADD_REG,		1},
	{INST_SET_REG2,		sizeof(script_setreg3w_t)},
	{INST_SET_REG3,		sizeof(script_setreg3w_t)},
	{0x34,		4},
	{0x35,		3},
	{0x36,		12},
	{0x37,		4},
	{0x38,		3},
	{0x39,		8},
	{0x3a,		16},
	{INST_DOOR_SET,	sizeof(script_door_set_t)},
	{0x3c,		2},
	{0x3d,		3},
	{0x3e,		6},
	{0x3f,		4},

	/* 0x40-0x4f */
	{0x40,		8},
	{0x41,		10},
	{0x42,		1},
	{0x43,		4},
	{INST_EM_SET,	22},
	{0x45,		5},
	{0x46,		10},
	{0x47,		2},
	{0x48,		16},
	{0x49,		8},
	{0x4a,		2},
	{0x4b,		3},
	{0x4c,		5},
	{0x4d,		22},
	{0x4e,		22},
	{0x4f,		4},

	/* 0x50-0x5f */
	{0x50,		4},
	{0x51,		6},
	{0x52,		6},
	{0x53,		6},
	{0x54,		22},
	{0x55,		6},
	{0x56,		4},
	{0x57,		8},
	{0x58,		4},
	{0x59,		4},
	{0x5a,		2},
	{0x5b,		2},
	{0x5c,		3},
	{0x5d,		2},
	{0x5e,		2},
	{0x5f,		2},

	/* 0x60-0x6f */
	{0x60,		14},
	{0x61,		4},
	{0x62,		2},
	{INST_NOP63,	1},
	{0x64,		16},
	{0x65,		2},
	{0x66,		1},
	{0x67,		28},
	{0x68,		40},
	/*{0x69,		2},*/
	{0x6a,		6},
	{0x6b,		4},
	{0x6c,		1},
	{0x6d,		4},
	{0x6e,		6},
	{0x6f,		2},

	/* 0x70-0x7f */
	{0x70,		1},
	{0x71,		1},
	{0x72,		16},
	{0x73,		8},
	{0x74,		4},
	{0x75,		22},
	{0x76,		3},
	{0x77,		4},
	{0x78,		6},
	{0x79,		1},
	{0x7a,		16},
	{0x7b,		16},
	{0x7c,		6},
	{0x7d,		6},
	{0x7e,		6},
	{0x7f,		6},
	
	/* 0x80-0x8f */
	{0x80,		2},
	/*{0x81,		2},*/
	{0x82,		3},
	{0x83,		1},
	{0x84,		2},
	{0x85,		6},
	{0x86,		1},
	{0x87,		1},
	{0x88,		3},
	{0x89,		1},
	{0x8a,		6},
	{0x8b,		6},
	{0x8c,		12},
	{0x8d,		24},
	{0x8e,		24}
};

static char indentStr[256];

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this, int num_script);
static int scriptGetInstLen(room_t *this);
static void scriptExecInst(room_t *this);
static void scriptPrintInst(room_t *this);

static void scriptDisasmInit(void);

/*--- Functions ---*/

void room_rdt2_scriptInit(room_t *this)
{
	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivExecInst = scriptExecInst;
	this->scriptPrivPrintInst = scriptPrintInst;
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

	scriptDisasmInit();

	return this->cur_inst;
}

static int scriptGetInstLen(room_t *this)
{
	int i, inst_len = 0;

	if (!this) {
		return 0;
	}
	if (!this->cur_inst) {
		return 0;
	}

	for (i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == this->cur_inst[0]) {
			inst_len = inst_length[i].length;
			break;
		}
	}

	return inst_len;
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

/*
 * --- Script disassembly ---
 */

#ifndef ENABLE_SCRIPT_DISASM

static void scriptPrintInst(room_t *this)
{
}

static void scriptDisasmInit(void)
{
}

#else

/*--- Variables ---*/

static int numFunc;
static int indentLevel;

static char strBuf[256];
static char tmpBuf[256];

/*--- Functions ---*/

static void reindent(int num_indent)
{
	int i;

	memset(tmpBuf, 0, sizeof(tmpBuf));

	for (i=0; (i<num_indent) && (i<sizeof(tmpBuf)-1); i++) {
		tmpBuf[i<<1]=' ';
		tmpBuf[(i<<1)+1]=' ';
	}

	strncat(strBuf, tmpBuf, sizeof(strBuf)-1);
}

static void scriptDisasmInit(void)
{
	indentLevel = 0;
	numFunc = 0;
}

static void scriptPrintInst(room_t *this)
{
	script_inst_t *inst;

	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	inst = (script_inst_t *) this->cur_inst;

	memset(strBuf, 0, sizeof(strBuf));

	if ((indentLevel==0) && (inst->opcode!=0xff)) {
		logMsg(1, "func%02x() {\n", numFunc++);
		++indentLevel;
	}

	switch(inst->opcode) {

		/* 0x00-0x0f */

		case INST_NOP:
			reindent(indentLevel);
			strcat(strBuf, "nop\n");
			break;
		case INST_RETURN:
			if (indentLevel>1) {
				reindent(indentLevel);
				strcat(strBuf, "return\n");
			} else {
				reindent(--indentLevel);
				strcat(strBuf, "}\n\n");
			}
			break;
		case INST_EVT_EXEC:
			reindent(indentLevel);
			if (inst->evtexec.ex_opcode == INST_FUNC) {
				sprintf(tmpBuf, "EVT_EXEC #0x%02x, func%02x()\n",
					inst->evtexec.cond, inst->evtexec.num_func);
			} else {
				sprintf(tmpBuf, "EVT_EXEC #0x%02x, xxx\n",
					inst->evtexec.cond);
			}
			strcat(strBuf, tmpBuf);
			break;
		case INST_IF:
			reindent(indentLevel++);
			strcat(strBuf, "if (xxx) {\n");
			break;
		case INST_ELSE:
			reindent(--indentLevel);
			strcat(strBuf, "} else {\n");
			break;
		case INST_END_IF:
			reindent(--indentLevel);
			strcat(strBuf, "}\n");
			break;
		case INST_SLEEP_N:
			reindent(indentLevel);
			sprintf(tmpBuf, "sleep %d\n", SDL_SwapLE16(inst->sleepn.delay));
			strcat(strBuf, tmpBuf);
			break;

		/* 0x10-0x1f */

		case INST_FUNC:
			reindent(indentLevel);
			sprintf(tmpBuf, "func%02x()\n", inst->func.num_func);
			strcat(strBuf, tmpBuf);
			break;
		case INST_NOP1C:
		case INST_NOP1E:
		case INST_NOP1F:
			reindent(indentLevel);
			strcat(strBuf, "nop\n");
			break;

		/* 0x20-0x2f */

		case INST_NOP20:
			reindent(indentLevel);
			strcat(strBuf, "nop\n");
			break;
		case INST_EVAL_CK:
			reindent(indentLevel);
			strcat(strBuf, "EVAL_CK xxx\n");
			break;
		case INST_EVAL_CMP:
			reindent(indentLevel);
			strcat(strBuf, "EVAL_CMP xxx\n");
			break;
		case INST_PRINT_TEXT:
			reindent(indentLevel);
			sprintf(tmpBuf, "PRINT_TEXT #0x%02x\n", inst->print_text.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_ITEM_SET:
			reindent(indentLevel);
			strcat(strBuf, "ITEM_SET xxx\n");
			break;
		case INST_SET_REG_MEM:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET_REG_MEM %d,%d\n",
				inst->set_reg_mem.component, inst->set_reg_mem.index);
			strcat(strBuf, tmpBuf);
			break;
		case INST_SET_REG_IMM:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET_REG_IMM %d,%d\n",
				inst->set_reg_imm.component, SDL_SwapLE16(inst->set_reg_imm.value));
			strcat(strBuf, tmpBuf);
			break;

		/* 0x30-0x3f */

		case INST_SET_REG_TMP:
			reindent(indentLevel);
			strcat(strBuf, "SET_REG_TMP\n");
			break;
		case INST_ADD_REG:
			reindent(indentLevel);
			strcat(strBuf, "ADD_REG\n");
			break;
		case INST_SET_REG2:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET_REG2 %d,%d,%d\n",
				SDL_SwapLE16(inst->set_reg_3w.value[0]),
				SDL_SwapLE16(inst->set_reg_3w.value[1]),
				SDL_SwapLE16(inst->set_reg_3w.value[2]));
			strcat(strBuf, tmpBuf);
			break;
		case INST_SET_REG3:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET_REG3 %d,%d,%d\n",
				SDL_SwapLE16(inst->set_reg_3w.value[0]),
				SDL_SwapLE16(inst->set_reg_3w.value[1]),
				SDL_SwapLE16(inst->set_reg_3w.value[2]));
			strcat(strBuf, tmpBuf);
			break;
		case INST_DOOR_SET:
			reindent(indentLevel);
			strcat(strBuf,"DOOR_SET xxx\n");
			break;

		/* 0x40-0x4f */

		case INST_EM_SET:
			reindent(indentLevel);
			strcat(strBuf,"EM_SET xxx\n");
			break;

		/* 0x50-0x5f */

		/* 0x60-0x6f */

		case INST_NOP63:
			reindent(indentLevel);
			strcat(strBuf, "nop\n");
			break;

		default:
			reindent(indentLevel);
			sprintf(tmpBuf, "Unknown opcode 0x%02x\n", inst->opcode);
			strcat(strBuf, tmpBuf);
			break;

	}

	logMsg(1, "%s", strBuf);
}

#endif /* ENABLE_SCRIPT_DISASM */
