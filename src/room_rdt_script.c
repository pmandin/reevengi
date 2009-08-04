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

#define INST_IF		0x01
#define INST_ELSE	0x02
#define INST_ENDIF	0x03
#define INST_04		0x04
#define INST_05		0x05
#define INST_DOOR	0x0c
#define INST_0D		0x0d
#define INST_12		0x12
#define INST_18		0x18
#define INST_1B		0x1b
#define INST_1F		0x1f
#define INST_37		0x37
#define INST_3B		0x3b

/*--- Types ---*/

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
	Uint8 unknown[4];
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
	{INST_IF,	sizeof(script_if_t)},
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_ENDIF,	sizeof(script_endif_t)},
	{INST_04,	4},
	{INST_05,	5},
	{INST_DOOR,	26},
	{INST_0D,	18},
	{INST_12,	10},
	{INST_18,	26},
	{INST_1B,	22},
	{INST_1F,	28},
	{INST_37,	4},
	{INST_3B,	6}
};

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this);
static Uint8 *scriptNextInst(room_t *this);
static void scriptDumpInst(room_t *this);

/*--- Functions ---*/

void room_rdt_scriptInit(room_t *this)
{
	rdt1_header_t *rdt_header = (rdt1_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_INIT_SCRIPT]);
	Uint16 *init_script = (Uint16 *) (& ((Uint8 *) this->file)[offset]);
	Uint8 *inst;

	this->script_length = SDL_SwapLE16(*init_script);

	this->scriptFirstInst = scriptFirstInst;
	this->scriptNextInst = scriptNextInst;
	this->scriptDumpInst = scriptDumpInst;

	logMsg(3, "Init script at offset 0x%08x, length 0x%04x\n", offset, this->script_length);

	/* Dump script */
	inst = this->scriptFirstInst(this);
	while (inst) {
		this->scriptDumpInst(this);
		inst = this->scriptNextInst(this);
	}
}

static Uint8 *scriptFirstInst(room_t *this)
{
	rdt1_header_t *rdt_header;
	Uint32 offset;

	if (!this) {
		return NULL;
	}

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_INIT_SCRIPT]);

	this->cur_inst_offset = 0;
	this->cur_inst = (& ((Uint8 *) this->file)[offset+2]);
	return this->cur_inst;
}

static Uint8 *scriptNextInst(room_t *this)
{
	int i, inst_len = 0, next_offset;
	Uint8 *next_inst;

	if (!this) {
		return NULL;
	}
	if (!this->cur_inst) {
		return NULL;
	}

	next_inst = this->cur_inst;
	for (i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == next_inst[0]) {
			inst_len = inst_length[i].length;
			break;
		}
	}

	if (inst_len == 0) {
		return NULL;
	}

	next_offset = this->cur_inst_offset + inst_len;	
	if (this->script_length>0) {
		if (next_offset >= this->script_length) {
			logMsg(3, "End of script reached\n");
			return NULL;
		}
	}

	this->cur_inst = &next_inst[inst_len];
	return this->cur_inst;
}

static void scriptDumpInst(room_t *this)
{
	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	switch(this->cur_inst[0]) {
		case INST_IF:
			logMsg(3, "if (xxx) {\n");
			break;
		case INST_ELSE:
			logMsg(3, "} else {\n");
			break;
		case INST_ENDIF:
			logMsg(3, "}\n");
			break;
		default:
			logMsg(3, "Unknown opcode 0x%02x\n", this->cur_inst[0]);
			break;
	}
}
