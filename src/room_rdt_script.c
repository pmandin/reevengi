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
#define INST_ENDIF	0x03
#define INST_DOOR_SET	0x0c

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

/*
1000
0c 00  8c 0a f4 01 a4 06 08 07  03 00 00 04 00 01  fc 21 00 00 dc 1e 00 04 00 81
-> s1,r1,c4

1010
0c 00  b0 1d 14 1e 60 09 dc 05  00 00 00 04 00 00  ac 0d 00 00 28 0a 00 0c 00 81
0c 01  b0 36 30 43 d0 07 dc 05  02 00 1d 05 ca 02  48 0d 00 00 f0 23 00 04 34 81
0c 02  c8 4b 38 4a d0 07 8c 0a  01 00 00 01 00 03  60 09 00 00 e0 60 00 00 00 81
0c 03  5c 12 c4 09 2d 0a 14 05  00 05 18 60 00 41  14 37 00 00 ec 2c 00 00 00 81
-> s1,r0,c0
-> s2,r1,c0

1030
0c 00  00 00 94 5c c4 09 34 08  02 00 00 01 00 01  38 4a 00 00 14 50 00 08 00 81
0c 01  20 03 f4 01 80 0c c4 09  01 00 01 01 92 04  b8 3d 00 00 74 0e 00 04 fe 81
0c 02  38 4a 24 45 c4 09 60 09  02 00 02 01 40 0c  20 35 00 00 e8 1c 00 08 00 81
0c 03  4c 1d fc 3a fc 08 d0 07  03 00 00 04 40 0d  e0 15 00 00 f0 0a 00 0c 00 81
0c 04  00 00 74 27 60 09 40 06  02 00 1c 03 cb 0e  e8 1c 00 00 cc 29 00 08 33 81
-> s1,r1,c0   0000 0001 0000 0000
-> s1,r4,c2   0000 0100 1001 0010
-> s1,r12,c0  0000 1100 0100 0000
-> s1,r13,c0

1040
0c 00  d0 39 04 10 b8 0b 1c 0c  02 00 01 01 92 03  b8 0b 00 00 b8 0b 00 0c ff 81
0c 01  68 74 a0 0f a4 06 ac 0d  02 00 1f 05 86 0f  b0 36 00 00 b8 0b 00 0c 33 81
0c 02  0c 7b 6c 07 fc 08 08 07  06 00 00 01 d3 10  00 00 00 00 00 00 00 00 ff 81
0c 03  68 29 00 00 f0 0a fc 08  01 00 03 04 00 05  9c 18 00 00 14 37 00 04 00 81
-> s1,r5,c2
-> s1,r3,c0

1050
0c 00  b4 14 b0 36 f0 0a f0 0a  02 00 03 04 00 04  ec 2c 00 00 f0 0a 00 0c 00 81
0c 01  70 7b 9c 18 d0 07 a0 0f  09 00 03 07 00 06  48 0d 00 00 68 42 00 00 00 81

1060
0c 00  7c 79 10 27 d0 07 b8 0b  06 00 04 07 00 07  8c 0a 00 00 c8 19 00 00 00 81
-> s1,r7,c0
0c 01  7c 79 a8 48 d0 07 b0 04  02 00 03 01 c5 11  f0 0a 00 00 f8 11 00 00 34 81
-> s1,r0x11,c0
0c 02  e8 03 98 3a 9e 07 d8 0e  06 00 03 07 00 05  b4 78 00 00 6c 20 00 08 00 81
-> s1,r5,c0
0c 03  08 39 20 03 50 14 98 08  02 00 03 01 c5 11  f0 0a 00 00 f8 11 00 00 34 81

0c 04  f0 3c 1c 3e 54 0b 28 0a  00 04 16 60 00 43  cc 42 00 00 d4 62 00 0c 00 81
-> s2,r3,c0 : 0x43 = 0x0100 0011

room 3000
0c 00  3c 5a 98 6c a4 06 d0 07  00 08 0f 40 00 01  80 57 00 00 10 0e 00 0c 00 81
0c 01  00 00 00 00 01 00 01 00  01 06 15 40 00 02  4c 4f 00 00 5c 12 00 04 00 00
0c 02  1c 57 80 0c 5c 12 04 10  0c 01 08 00 00 3b  08 20 00 00 0c 17 00 08 00 81
0x3b = 0011 1011
*/

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
	/*{INST_IF,	sizeof(script_if_t)},*/
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_ENDIF,	sizeof(script_endif_t)},
	{0x04,	4},
	{0x05,	4},
	{0x06,	2},
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
static void scriptExecInst(room_t *this);

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

#else

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
			logMsg(1, "  nop\n");
			break;
		case INST_IF:
			logMsg(1, "  if (xxx) {\n");
			break;
		case INST_ELSE:
			logMsg(1, "  } else {\n");
			break;
		case INST_ENDIF:
			logMsg(1, "  }\n");
			break;
		case INST_DOOR_SET:
			logMsg(1, "  DOOR_SET\n");
			break;
	}
}

#endif /* ENABLE_SCRIPT_DISASM */
