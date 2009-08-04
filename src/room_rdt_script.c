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
#define INST_04		0x04
#define INST_05		0x05
#define INST_06		0x06
#define INST_07		0x07
#define INST_09		0x09
#define INST_DOOR	0x0c
#define INST_0D		0x0d
#define INST_0F		0x0f
#define INST_10		0x10
#define INST_12		0x12
#define INST_13		0x13
#define INST_14		0x14
#define INST_18		0x18
#define INST_19		0x19
#define INST_1B		0x1b
#define INST_1C		0x1c
#define INST_1D		0x1d
#define INST_1F		0x1f
#define INST_20		0x20
#define INST_21		0x21
#define INST_23		0x23
#define INST_28		0x28
#define INST_2A		0x2a
#define INST_2B		0x2b
#define INST_2F		0x2f
#define INST_30		0x30
#define INST_31		0x31
#define INST_33		0x33
#define INST_35		0x35
#define INST_37		0x37
#define INST_3A		0x3a
#define INST_3B		0x3b
#define INST_3D		0x3d
#define INST_41		0x41
#define INST_46		0x46
#define INST_47		0x47
#define INST_49		0x49
#define INST_4C		0x4c
#define INST_78		0x78

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
	{INST_04,	4},
	{INST_05,	4},
	{INST_06,	2},
	{INST_07,	10},
	{INST_09,	2},
	{INST_DOOR,	26},
	{INST_0D,	18},
	{INST_0F,	8},
	{INST_10,	10},
	{INST_12,	10},
	{INST_13,	4},
	{INST_14,	4},
	{INST_18,	26},
	{INST_19,	4},
	{INST_1B,	22},
	{INST_1C,	10},
	{INST_1D,	6},
	{INST_1F,	28},
	{INST_20,	14},
	{INST_21,	18},
	{INST_23,	4},
	{INST_28,	6},
	{INST_2A,	12},
	{INST_2B,	4},
	{INST_2F,	4},
	{INST_30,	12},
	{INST_31,	4},
	{INST_33,	12},
	{INST_35,	4},
	{INST_37,	4},
	{INST_3A,	4},
	{INST_3B,	6},
	{INST_3D,	12},
	{INST_41,	4},
	{INST_46,	4+(12*3)+4},
	{INST_47,	14},
	{INST_49,	2},
	{INST_4C,	4}
};

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this);
static Uint8 *scriptNextInst(room_t *this);
static void scriptDumpInst(room_t *this);

static int scriptGetInstLen(room_t *this);

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
	if (this->script_length == 0) {
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
	int inst_len;
	Uint8 *cur_inst;

	if (!this) {
		return NULL;
	}
	if (!this->cur_inst) {
		return NULL;
	}

	inst_len = scriptGetInstLen(this);
	if (inst_len == 0) {
		return NULL;
	}

	this->cur_inst_offset += inst_len;	
	if (this->script_length>0) {
		if (this->cur_inst_offset>= this->script_length) {
			logMsg(3, "End of script reached\n");
			return NULL;
		}
	}

	cur_inst = this->cur_inst;

	this->cur_inst = &cur_inst[inst_len];
	return this->cur_inst;
}

static void scriptDumpInst(room_t *this)
{
	int i, inst_len;

	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	inst_len = scriptGetInstLen(this);
	for (i=0; i<inst_len; i++) {
		logMsg(3, "%02x ", this->cur_inst[i]);
		/*logMsg(3, "%02x%s", this->cur_inst[i],
			(((i>0) && ((i & 15)==0)) ? "\n" : " ")
		);*/
	}
	logMsg(3, "\n");

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
		default:
			/*logMsg(3, "  Unknown opcode 0x%02x, offset 0x%04x\n", this->cur_inst[0], this->cur_inst_offset);*/
			break;
	}
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

	/*logMsg(4, "opcode 0x%02x len %d\n", this->cur_inst[0], inst_len);*/
	return inst_len;
}
