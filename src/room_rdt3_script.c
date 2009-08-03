/*
	Room description
	RE3 RDT script

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

#include "room.h"

/*--- Defines ---*/

#define INST_RETURN	0x01
#define INST_SLEEP_1	0x02
#define INST_IF		0x06
#define INST_ELSE	0x07
#define INST_END_IF	0x08
#define INST_SLEEP_N	0x09
#define INST_FUNC	0x19

#define INST_12		0x12
#define INST_12_LEN	18

/*--- Types ---*/

typedef struct {
	Uint8 opcode;
	Uint8 num_func;
} script_func_t;

typedef union {
	Uint8 opcode;
	script_func_t func;
} script_inst_t;

/*--- Variables ---*/

static script_inst_t *cur_inst;

static char indentStr[256];

/*--- Functions prototypes ---*/

static script_inst_t *scriptResetInst(room_t *this);
static script_inst_t *scriptNextInst(room_t *this);

static void reindent(int num_indent);
static void scriptDisasm(room_t *this);

/*--- Functions ---*/

void room_rdt3_scriptInit(room_t *this)
{
	scriptDisasm(this);
}

static script_inst_t *scriptResetInst(room_t *this)
{
	Uint32 *item_offset, offset;

	if (!this) {
		return NULL;
	}
	
	item_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+16*4]);
	offset = SDL_SwapLE32(*item_offset);
	cur_inst = (script_inst_t *) &((Uint8 *) this->file)[offset];

	return cur_inst;
}

static script_inst_t *scriptNextInst(room_t *this)
{
	int item_length = 0;
	Uint8 *next_inst = (Uint8 *) cur_inst;

	if (!this) {
		return NULL;
	}

	switch(cur_inst->opcode) {
		case INST_RETURN:
			item_length = 2;
			break;
		case INST_FUNC:
			item_length = sizeof(script_func_t);
			break;
		case INST_12:
			item_length = INST_12_LEN;
			break;
	}

	if (item_length == 0) {
		/* End of list, or unknown item */
		next_inst = NULL;
	} else {
		next_inst = &next_inst[item_length];
	}
	cur_inst = (script_inst_t *) next_inst;

	return cur_inst;
}

static void reindent(int num_indent)
{
	int i;

	memset(indentStr, 0, sizeof(indentStr));
	for (i=0; (i<num_indent) && (i<255); i++) {
		indentStr[i<<1]=' ';
		indentStr[(i<<1)+1]=' ';
	}
}

static void scriptDisasm(room_t *this)
{
	int indent = 0, numFunc = 0;
	script_inst_t *inst;

	reindent(indent);

	logMsg(3, "Disasm script\n");

	logMsg(3, "function Func%d()\n{\n", numFunc++);
	reindent(++indent);

	inst = scriptResetInst(this);
	while (inst) {
		switch(inst->opcode) {
			case INST_RETURN:
				logMsg(3, "%sreturn();\n", indentStr);
				reindent(--indent);
				logMsg(3, "%s}\n", indentStr);
				if (indent==0) {
					logMsg(3, "\nfunction Func%d()\n{\n", numFunc++);
					reindent(++indent);
				}
				break;
			case INST_FUNC:
				{
					logMsg(3, "%sFunc%d()\n", indentStr, inst->func.num_func);
				}
				break;
			default:
				logMsg(3, "Unknown opcode 0x%02x\n", inst->opcode);
				break;
		}

		inst = scriptNextInst(this);
	}
}
