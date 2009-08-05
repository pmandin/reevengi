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

#include "room.h"
#include "room_rdt.h"

/*--- Defines ---*/

#define INST_NOP	0x00
#define INST_IF		0x01
#define INST_ELSE	0x02
#define INST_ENDIF	0x03
#define INST_DOOR	0x0c

/*--- Types ---*/

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
} script_if_base_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
	Uint8 unknown[2];	/* [0] = 0x50,0x1a,0x07,0x21 */
} script_if02_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
	Uint8 unknown[4];	/* [0] = 0x04,0x06 */
} script_if04_t;

typedef union {
	script_if_base_t	base;
	script_if02_t		if02;
	script_if04_t		if04;
} script_if_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
} script_else_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
} script_endif_t;

typedef union {
	Uint8 opcode;
	script_if_t	i_if;
	script_else_t	i_else;
	script_endif_t	i_endif;
} script_inst_t;

typedef struct {
	Uint8 opcode;
	Uint8 length;
} script_inst_len_t;

/*--- Variables ---*/

static const script_inst_len_t inst_length[]={
	{INST_NOP,	2},
	/*{INST_IF,	sizeof(script_if_t)},*/
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_ENDIF,	sizeof(script_endif_t)},
	{0x04,	4},
	{0x05,	4},
	{0x06,	2},
	{0x07,	10},
	{0x09,	2},
	{INST_DOOR,	26},
	{0x0d,	18},
	{0x0f,	8},
	{0x10,	10},
	{0x12,	10},
	{0x13,	4},
	{0x14,	4},
	{0x18,	26},
	{0x19,	4},
	{0x1b,	22},
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

/*--- Functions ---*/

void room_rdt_scriptInit(room_t *this)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_INIT_SCRIPT]);
	Uint16 *init_script = (Uint16 *) (& ((Uint8 *) this->file)[offset]);
	Uint8 *inst;

	this->script_length = SDL_SwapLE16(*init_script);

	logMsg(3, "rdt1: Init script at offset 0x%08x, length 0x%04x\n", offset, this->script_length);

	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivPrintInst = scriptPrintInst;
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

	/* Exceptions, variable lengths */
	if (inst_len == 0) {
		switch (this->cur_inst[0]) {
			case INST_IF:
				{
					script_if02_t *i_if = (script_if02_t *) this->cur_inst;
					if (i_if->unknown[0] == 0x50) {
						inst_len = sizeof(script_if02_t);
					} else if (i_if->unknown[0] == 0x1a) {
						inst_len = sizeof(script_if02_t);
					} else if (i_if->unknown[0] == 0x21) {
						inst_len = sizeof(script_if02_t);
					} else if (i_if->unknown[0] == 0x07) {
						inst_len = sizeof(script_if02_t);
					} else if (i_if->unknown[0] == 0x04) {
						inst_len = sizeof(script_if04_t);
					} else if (i_if->unknown[0] == 0x06) {
						inst_len = sizeof(script_if04_t);
					}
				}
				break;
			default:
				break;
		}
	}

	return inst_len;
}

static void scriptPrintInst(room_t *this)
{
	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	switch(this->cur_inst[0]) {
		case INST_NOP:
			logMsg(3, "  nop\n");
			break;
		case INST_IF:
			logMsg(3, "  if (xxx) {\n");
			break;
		case INST_ELSE:
			logMsg(3, "  } else {\n");
			break;
		case INST_ENDIF:
			logMsg(3, "  }\n");
			break;
		case INST_DOOR:
			logMsg(3, "  create door\n");
			break;
	}
}
