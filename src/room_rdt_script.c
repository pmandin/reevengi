/*
	Room description
	RE1 RDT script

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
#include "room_rdt.h"
#include "state.h"
#include "log.h"

/*--- Defines ---*/

/* Instructions */

#define INST_NOP	0x00
#define INST_IF		0x01
#define INST_ELSE	0x02
#define INST_END_IF	0x03
#define INST_BIT_TEST	0x04
#define INST_BIT_OP	0x05
#define INST_CMP06	0x06
#define INST_CMP07	0x07
#define INST_SET06	0x08
#define INST_DOOR_SET	0x0c
#define INST_ITEM_SET	0x0d
#define INST_NOP0E	0x0e

#define INST_CMP10	0x10
#define INST_CMP11	0x11
#define INST_ITEM_MODEL_SET	0x18
#define INST_EM_SET	0x1b
#define INST_OM_SET	0x1f

/* Item types */

#define ITEM_MESSAGE	0x02
#define ITEM_OBSTACLE	0x02
#define ITEM_TRIGGER1	0x07	/* riddle, event, movable object */
#define ITEM_BOX	0x08	/* deposit box */
#define ITEM_OBJECT	0x09	/* pickable object */
#define ITEM_TRIGGER2	0x09
#define ITEM_TYPEWRITER	0x10

/*--- Types ---*/

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
} script_if_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
} script_else_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
} script_endif_t;

typedef struct {
	Uint8 opcode;
	Uint8 flag;
	Uint8 object;
	Uint8 value;
} script_bit_test_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 x,y,w,h;
	Uint8 unknown0[5];
	Uint8 next_stage_and_room;	/* bits 7-5: stage, 4-0: room */
	Sint16 next_x,next_y,next_z;
	Sint16 next_dir;
	Uint16 unknown2;
} script_door_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 x,y,w,h;
	Uint8 type;	/* 0x10: typewriter */
	Uint8 unknown[7];
} script_item_set_t;

typedef union {
	Uint8 opcode;
	script_if_t	i_if;
	script_else_t	i_else;
	script_endif_t	i_endif;
	script_door_set_t	door_set;
	script_item_set_t	item_set;
} script_inst_t;

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
	{INST_SET06,	4},
	{0x09,	2},
	{0x0a,	2},
	{0x0b,	4},
	{INST_DOOR_SET,	sizeof(script_door_set_t)},
	{INST_ITEM_SET,	sizeof(script_item_set_t)},
	{INST_NOP0E,	2},
	{0x0f,	8},

	{INST_CMP10,	2},
	{INST_CMP11,	2},
	{0x12,	10},
	{0x13,	4},
	{0x14,	4},
	{0x15,	2},
	{0x16,	2},
	{0x17,	10},
	{0x18,	26},
	{0x19,	4},
	{0x1a,	2},
	{INST_EM_SET,	22},
	{0x1c,	6},
	{0x1d,	2},
	{0x1e,	4},
	{0x1f,	28},

	{0x20,	14},
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

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this, int num_script);
static int scriptGetInstLen(room_t *this);
static void scriptPrintInst(room_t *this);
static void scriptExecInst(room_t *this);

static void scriptDisasmInit(void);

/*--- Functions ---*/

void room_rdt_scriptInit(room_t *this)
{
	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivPrintInst = scriptPrintInst;
	this->scriptPrivExecInst = scriptExecInst;
}

static Uint8 *scriptFirstInst(room_t *this, int num_script)
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
	offset = SDL_SwapLE32(rdt_header->offsets[room_script]);
	scriptPtr = & (((Uint8 *) this->file)[offset]);

	this->script_length = SDL_SwapLE16(*((Uint16 *) scriptPtr));
	this->cur_inst_offset = 0;
	this->cur_inst = &scriptPtr[2];

	logMsg(1, "rdt1: Script %d at offset 0x%08x, length 0x%04x\n", num_script, offset, this->script_length);

	scriptDisasmInit();

	return this->cur_inst;
}

static int scriptGetInstLen(room_t *this)
{
	int i;

	if (!this) {
		return 0;
	}
	if (!this->cur_inst) {
		return 0;
	}

	for (i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == this->cur_inst[0]) {
			return inst_length[i].length;
		}
	}

	/* Variable length instructions */
	switch(this->cur_inst[0]) {
		case 0x28:
			switch(this->cur_inst[2]) {
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
			switch(this->cur_inst[1]) {
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

static void scriptExecInst(room_t *this)
{
#if 0
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
	}
#endif
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

/*static int numFunc;*/
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
	/*numFunc = 0;*/
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

	/*if ((indentLevel==0) && (inst->opcode!=0xff)) {
		sprintf(strBuf, "func%02x() {\n", numFunc++);
		++indentLevel;
	}*/

	switch(inst->opcode) {

		/* 0x00-0x0f */

		case INST_NOP:
			reindent(indentLevel);
			strcat(strBuf, "nop\n");
			break;
		case INST_IF:
			reindent(indentLevel++);
			strcat(strBuf, "if (xxx) {\n");
			break;
		case INST_ELSE:
			reindent(indentLevel-1);
			strcat(strBuf, "} else {\n");
			break;
		case INST_END_IF:
			reindent(--indentLevel);
			strcat(strBuf, "}\n");
			break;
		case INST_BIT_TEST:
			{
				script_bit_test_t *evalCk = (script_bit_test_t *) inst;

				reindent(indentLevel);
				sprintf(tmpBuf, "BIT_TEST flag 0x%02x object 0x%02x %s\n",
					evalCk->flag, evalCk->object,
					evalCk->value ? "on" : "off");
				strcat(strBuf, tmpBuf);
			}
			break;
		case INST_BIT_OP:
			reindent(indentLevel);
			strcat(strBuf, "BIT_OP xxx\n");
			break;
		case INST_CMP06:
			reindent(indentLevel);
			strcat(strBuf, "OBJ06_TEST xxx\n");
			break;
		case INST_CMP07:
			reindent(indentLevel);
			strcat(strBuf, "OBJ07_TEST xxx\n");
			break;
		case INST_SET06:
			reindent(indentLevel);
			strcat(strBuf, "OBJ06_SET xxx\n");
			break;
		case INST_DOOR_SET:
			reindent(indentLevel);
			strcat(strBuf, "DOOR_SET xxx\n");
			break;
		case INST_ITEM_SET:
			reindent(indentLevel);
			strcat(strBuf, "ITEM_SET xxx\n");
			break;
		case INST_NOP0E:
			reindent(indentLevel);
			strcat(strBuf, "Nop\n");
			break;

		/* 0x10-0x1f */

		case INST_CMP10:
			reindent(indentLevel);
			strcat(strBuf, "OBJ10_TEST xxx\n");
			break;
		case INST_CMP11:
			reindent(indentLevel);
			strcat(strBuf, "OBJ11_TEST xxx\n");
			break;
		case INST_ITEM_MODEL_SET:
			reindent(indentLevel);
			strcat(strBuf, "ITEM_MODEL_SET xxx\n");
			break;
		case INST_EM_SET:
			reindent(indentLevel);
			strcat(strBuf, "EM_SET xxx\n");
			break;
		case INST_OM_SET:
			reindent(indentLevel);
			strcat(strBuf, "OM_SET xxx\n");
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
