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

#define INST_NOP	0x00
#define INST_IF		0x01
#define INST_ELSE	0x02
#define INST_END_IF	0x03
#define INST_EVAL_CK	0x04
#define INST_EVAL_CMP	0x06
#define INST_DOOR_SET	0x0c

#define INST_EM_SET	0x1b

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
} script_eval_ck_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 x,y,w,h;
	Uint8 unknown0[5];
	/*Uint8 unknown1;
	Uint8 next_camera;*/		/* bits 2,1,0: camera */
	Uint8 next_stage_and_room;	/* bits 7,6,5: stage, 4,3,2,1,0: room */
	Sint16 next_x,next_y,next_z;
	Sint16 next_dir;
	Uint16 unknown2;
} script_door_set_t;

typedef union {
	Uint8 opcode;
	script_if_t	i_if;
	script_else_t	i_else;
	script_endif_t	i_endif;
	script_door_set_t	door_set;
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
	{INST_EVAL_CK,	sizeof(script_eval_ck_t)},
	{0x05,	4},
	{INST_EVAL_CMP,	4},
	{0x07,	10},
	{0x09,	2},
	{INST_DOOR_SET,	26},
	{0x0d,	18},
	{0x0f,	8},

	{0x10,	10},
	{0x12,	10},
	{0x13,	4},
	{0x14,	4},
	{0x18,	26},
	{0x19,	4},
	{INST_EM_SET,	22},
	{0x1c,	10},
	{0x1d,	6},
	{0x1f,	28},

	{0x20,	14},
	{0x21,	18},
	{0x23,	4},
	{0x28,	6},
	{0x2a,	12},
	{0x2b,	4},
	{0x2f,	4},

	{0x30,	12},
	{0x31,	4},
	{0x33,	12},
	{0x35,	4},
	{0x37,	4},
	{0x3a,	4},
	{0x3b,	6},
	{0x3d,	12},

	{0x41,	4},
	{0x46,	4+(12*3)+4},
	{0x47,	14},
	{0x49,	2},
	{0x4c,	4}
};

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this);
static int scriptGetInstLen(room_t *this);
static void scriptPrintInst(room_t *this);
static void scriptExecInst(room_t *this);

static void scriptDisasmInit(void);

/*--- Functions ---*/

void room_rdt_scriptInit(room_t *this)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_INIT_SCRIPT]);
	Uint16 *init_script = (Uint16 *) (& ((Uint8 *) this->file)[offset]);
	Uint8 *inst;

	this->script_length = SDL_SwapLE16(*init_script);

	logMsg(1, "rdt1: Init script at offset 0x%08x, length 0x%04x\n", offset, this->script_length);

	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivPrintInst = scriptPrintInst;
	this->scriptPrivExecInst = scriptExecInst;
}

static Uint8 *scriptFirstInst(room_t *this)
{
	rdt1_header_t *rdt_header;
	Uint32 offset;

	if (!this) {
		return NULL;
	}
	if (this->script_length == 0) {
		return NULL;
	}

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_INIT_SCRIPT]);

	this->cur_inst_offset = 0;
	this->cur_inst = (& ((Uint8 *) this->file)[offset+2]);

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
			reindent(--indentLevel);
			strcat(strBuf, "} else {\n");
			break;
		case INST_END_IF:
			reindent(--indentLevel);
			strcat(strBuf, "}\n");
			break;
		case INST_EVAL_CK:
			{
				script_eval_ck_t *evalCk = (script_eval_ck_t *) inst;

				reindent(indentLevel);
				sprintf(tmpBuf, "EVAL_CK flag 0x%02x object 0x%02x %s\n",
					evalCk->flag, evalCk->object,
					evalCk->value ? "on" : "off");
				strcat(strBuf, tmpBuf);
			}
			break;
		case INST_EVAL_CMP:
			reindent(indentLevel);
			strcat(strBuf, "EVAL_CMP xxx\n");
			break;
		case INST_DOOR_SET:
			reindent(indentLevel);
			strcat(strBuf, "DOOR_SET xxx\n");
			break;

		/* 0x10-0x1f */

		case INST_EM_SET:
			reindent(indentLevel);
			strcat(strBuf, "EM_SET xxx\n");
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
