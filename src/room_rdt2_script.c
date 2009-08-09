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

#define INST_NOP	0x00
#define INST_RETURN	0x01
#define INST_IF		0x06
#define INST_ELSE	0x07
#define INST_END_IF	0x08

#define INST_DOOR_SET	0x3b

#define INST_EM_SET	0x44

/*--- Types ---*/

typedef struct {
	Uint8 type;
	Uint8 unknown[3];
} script_condition_t;

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

typedef union {
	Uint8 opcode;
	script_if_t	i_if;
	script_else_t	i_else;
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
	{0x02,		2},
	{0x03,		2},
	{0x04,		4},
	{0x05,		2},
	/*{INST_IF,	sizeof(script_if_t)},*/
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_END_IF,	2},
	{0x09,		4},
	{0x0a,		2},
	{0x0b,		2},
	{0x0d,		2},
	{0x0e,		2},
	{0x0f,		2},

	/* 0x10-0x1f */
	{0x10,		2},
	{0x11,		2},
	/*{0x12,		2},*/
	{0x13,		4},
	{0x14,		2+4},
	{0x16,		2},
	{0x17,		8},
	{0x18,		2},
	{0x1a,		2},
	{0x1b,		2},
	{0x1c,		2},
	{0x1d,		4},

	/* 0x20-0x2f */
	{0x20,		2},
	{0x21,		4},
	{0x22,		4},
	{0x25,		2},
	/*{0x26,		2},*/
	{0x28,		4},
	{0x29,		4},
	{0x2b,		6},
	{0x2c,		20},
	{0x2d,		38},
	{0x2e,		4},

	/* 0x30-0x3f */
	{0x32,		12},
	{0x33,		8},
	{0x34,		4},
	{0x35,		2},
	{0x36,		12},
	{0x37,		4},
	{0x38,		4},
	{0x39,		6},
	{0x3a,		16},
	{INST_DOOR_SET,	32},
	{0x3c,		16},
	{0x3d,		4+8},
	{0x3f,		4},

	/* 0x40-0x4f */
	{0x40,		8},
	{0x41,		10},
	{INST_EM_SET,	22},
	{0x46,		10},
	{0x47,		16},
	{0x4b,		3},
	{0x4c,		5},
	{0x4e,		22},

	/* 0x50-0x5f */
	{0x51,		6},
	{0x54,		22},
	{0x57,		8},
	{0x58,		4},
	{0x59,		4},
	{0x5b,		2},
	{0x5d,		2},
	{0x5e,		2},
	{0x5f,		2},

	/* 0x60-0x6f */
	{0x60,		14},
	{0x66,		2},
	{0x67,		28},
	{0x68,		40},
	{0x6b,		6},
	{0x6c,		2},
	{0x6e,		6},

	/* 0x70-0x7f */
	{0x7a,		16},
	{0x7b,		16},
	
	/* dummy, follow nop sometimes */
	{0xfc,		1}
};

static char indentStr[256];

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this);
static int scriptGetInstLen(room_t *this);
static void scriptPrintInst(room_t *this);

static int scriptGetConditionLen(script_condition_t *conditionPtr);

/*--- Functions ---*/

void room_rdt2_scriptInit(room_t *this)
{
	int i;
	rdt2_header_t *rdt_header = (rdt2_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_INIT_SCRIPT]);
	Uint32 smaller_offset = this->file_length;

	/* Search smaller offset after script to calc length */
	for (i=0; i<21; i++) {
		Uint32 next_offset = SDL_SwapLE32(rdt_header->offsets[i]);
		if ((next_offset>0) && (next_offset<smaller_offset) && (next_offset>offset)) {
			smaller_offset = next_offset;
		}
	}

	if (smaller_offset>offset) {
		this->script_length = smaller_offset - offset;
	}

	logMsg(1, "rdt2: Init script at offset 0x%08x, length 0x%04x\n", offset, this->script_length);

	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivPrintInst = scriptPrintInst;
}

static Uint8 *scriptFirstInst(room_t *this)
{
	rdt2_header_t *rdt_header;
	Uint32 offset;
	Uint16 *functionArrayPtr;

	if (!this) {
		return NULL;
	}
	if (this->script_length == 0) {
		return NULL;
	}

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_INIT_SCRIPT]);

	/* Start of script is an array of offsets to the various script functions
	 * The first offset also gives the first instruction to execute
	 */
	functionArrayPtr = (Uint16 *) (& ((Uint8 *) this->file)[offset]);

	this->cur_inst_offset = SDL_SwapLE16(functionArrayPtr[0]);
	this->cur_inst = (& ((Uint8 *) this->file)[offset + this->cur_inst_offset]);
	return this->cur_inst;
}

static int scriptGetConditionLen(script_condition_t *conditionPtr)
{
	int inst_len = 0;

	switch(conditionPtr->type) {
		case 0x21:
			inst_len = 4;
			break;
		case 0x23:
			inst_len = 6;
			break;
		case 0x3e:
			inst_len = 2;
			break;
		default:
			logMsg(1, "Unknown condition type 0x%02x\n", conditionPtr->type);
			break;
	}

	return inst_len;
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

	/* Exceptions, variable lengths */
	if (inst_len == 0) {
		script_inst_t *cur_inst = (script_inst_t *) this->cur_inst;
		switch(cur_inst->opcode) {
			case INST_IF:
				inst_len = sizeof(script_if_t) +
					scriptGetConditionLen(
						(script_condition_t *) (&this->cur_inst[sizeof(script_if_t)])
					);
				break;
			case 0x12:
				inst_len = 2 +
					scriptGetConditionLen(
						(script_condition_t *) (&this->cur_inst[2])
					);
				break;
			default:
				break;
		}
	}

	return inst_len;
}

/*
 * --- Script disassembly ---
 */

#ifndef ENABLE_SCRIPT_DISASM

static void scriptPrintInst(room_t *this)
{
}

#else

static void reindent(int num_indent)
{
	int i;

	memset(indentStr, 0, sizeof(indentStr));
	for (i=0; (i<num_indent) && (i<255); i++) {
		indentStr[i<<1]=' ';
		indentStr[(i<<1)+1]=' ';
	}
}

static void scriptPrintInst(room_t *this)
{
	int indent = 0, numFunc = 0;
	script_inst_t *inst;

	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	reindent(indent);

	inst = (script_inst_t *) this->cur_inst;

	switch(inst->opcode) {
		case INST_NOP:
			logMsg(1, "%snop\n", indentStr);
			break;
		case INST_RETURN:
			logMsg(1, "%sreturn\n", indentStr);
			reindent(--indent);
			logMsg(1, "%s}\n", indentStr);
			break;
		case INST_IF:
			{
				logMsg(1, "%sif (xxx) {\n", indentStr);
				reindent(++indent);
			}
			break;
		case INST_ELSE:
			{
				reindent(--indent);
				logMsg(1,"%s} else {\n", indentStr);
				reindent(++indent);
			}
			break;
		case INST_END_IF:
			{
				reindent(--indent);
				logMsg(1,"%s}\n", indentStr);
			}
			break;
		case INST_DOOR_SET:
			logMsg(1,"%sDOOR_SET xxx\n", indentStr);
			break;
		case INST_EM_SET:
			logMsg(1,"%sEM_SET xxx\n", indentStr);
			break;
		/*default:
			logMsg(3, "Unknown opcode 0x%02x offset 0x%08x\n", inst->opcode, this->cur_inst_offset);
			break;*/

	}
}

#endif /* ENABLE_SCRIPT_DISASM */
