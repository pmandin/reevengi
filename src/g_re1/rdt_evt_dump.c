/*
	RE1 EVT
	Room events

	Copyright (C) 2014	Patrice Mandin

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

#include "../log.h"
#include "../parameters.h"

#include "../g_common/room.h"
#include "../g_common/game.h"

#include "rdt.h"

#ifndef ENABLE_SCRIPT_DISASM

void rdt1_evt_scriptDump(room_t *this)
{
}

#else

/*--- Types ---*/

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 dummy;
} event_inst04_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown[2];
	Uint8 dummy;
} event_inst05_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
	/*Uint8 scd_file[];*/	/* embedded scd file */
} event_inst06_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
	/*Uint8 instruction[];*/	/* bytecode instruction to execute */
} event_inst07_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 offset;
} event_instfa_t;

typedef struct {
	Uint8 opcode;
	Uint8 flag;
} event_instfe_t;

typedef union {
	Uint8 opcode;

	event_inst04_t inst04;
	event_inst05_t inst05;
	event_inst06_t inst06;
	event_inst07_t inst07;

	event_instfa_t instfa;
	event_instfe_t instfe;
} event_inst_t;

/*--- Variables ---*/

static int event_mode;

static char strBuf[512];
static char tmpBuf[512];

/*--- Functions prototypes ---*/

static int rdt1_evt_eventGetInstLen(room_t *this, Uint8 *curInstPtr);

static void eventDumpBlock(room_t *this, event_inst_t *inst, Uint32 offset, int length, int indent);

/*--- Functions ---*/

void rdt1_evt_scriptDump(room_t *this)
{
	rdt1_header_t *rdt_header;
	Uint32 offset, smaller_offset, script_length = 0;
	Uint32 *functionArrayPtr;
	Uint8 *scriptPtr;
	int i, num_funcs;

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_EVENTS]);

	if (offset==0) {
		return;
	}

	/* Search smaller offset after script to calc length */
	smaller_offset = this->file_length;
	for (i=0; i<sizeof(rdt_header->offsets)/sizeof(Uint32); i++) {
		Uint32 next_offset = SDL_SwapLE32(rdt_header->offsets[i]);
		if ((next_offset>0) && (next_offset<smaller_offset) && (next_offset>offset)) {
			smaller_offset = next_offset;
		}
	}
	if (smaller_offset>offset) {
		script_length = smaller_offset - offset;
	}

	if (script_length==0) {
		return;
	}

	/* Start of script is an array of offsets to the various script functions */
	functionArrayPtr = (Uint32 *) (& ((Uint8 *) this->file)[offset]);

	logMsg(1, "rdt: Dumping events\n");
	num_funcs = SDL_SwapLE32(functionArrayPtr[0]) >> 2;
	for (i=0; i<num_funcs; i++) {
		Uint16 func_offset;
		Uint32 func_len;
		event_inst_t *startInst;

		/* NULL terminates list */
		func_offset = SDL_SwapLE32(functionArrayPtr[i]);
		if (!func_offset) {
			break;
		}

		func_len = script_length - func_offset;
		startInst = (event_inst_t *) (& ((Uint8 *) this->file)[offset + func_offset]);

		if (i<num_funcs-2) {
			func_len = SDL_SwapLE32(functionArrayPtr[i+1]) - func_offset;
		}

		logMsg(1, "0x%08x (file at 0x%08x): BEGIN_EVENT event%02x\n", func_offset, offset+func_offset, i);
		eventDumpBlock(this, startInst, func_offset, func_len, 1);
		logMsg(1, "          : END_EVENT\n\n");
	}
}

static int rdt1_evt_eventGetInstLen(room_t *this, Uint8 *curInstPtr)
{
	int i;

	assert(curInstPtr);

	switch(curInstPtr[0]) {
		/* All modes */
		case 0xf6:
			return 1;	/* continue with 0xf7 */
		case 0xf7:
			/* 1 or 0, conditionnal */
			return 1;	/* continue with 0xfe, part 2 */
		case 0xf8:
			/* write inside event script */
			return 1;	/* continue with 0xf9 */
		case 0xf9:
			/* 3 or 0, conditionnal */
			return 3;	/* continue with 0xfe, part 2 */
		case 0xfa:
			return 4;
		case 0xfb:
			/* 1 or variable */
			return 1;
		case 0xfc:
			return curInstPtr[1];	/* followed by single script bytecode instruction */
		case 0xfd:
			/* 1 or jump */
			return 1;
		case 0xfe:
			/* Disable event if following byte is 0xff */
			return 2;	/* skipped if coming from 0xf7, 0xf9 */
		case 0xff:
			/* Disable event */
			return 1;	/* continue with 0xfe */

		default:
			switch(event_mode) {
				case 0:
					switch(curInstPtr[0]) {
						case 0x00:
							return 1;	/* nop */
						case 0x01:
							event_mode = 1;
							return 1;
						case 0x02:
							event_mode = 2;
							/* and other stuff */
							return 1;
						case 0x03:
							event_mode = 2;
							return 1;
						case 0x04:
							return 3;
						case 0x05:
							return 4;
						case 0x06:
							return curInstPtr[1]+1;	/* embedded SCD */
						case 0x07:
							return curInstPtr[1];	/* followed by single script bytecode instruction */
						case 0x08:
							return 1;	/* continue with 0xfe, part 2 */
						case 0x09:
							/* write some stuff */
							return 2;
						default:
							break;
					}
					break;
				case 1:
					switch(curInstPtr[0]) {
						case 0x80:
							return 1;	/* continue with 0x8b */
						case 0x81:
							return 2+8;	/* 2 or 10, conditionnal */
						case 0x82:
							return 1;
						case 0x83:
							return 7;
						case 0x84:
							return 4;
						case 0x85:
							return 2;
						case 0x86:
							return 1;
						case 0x87:
							return 2;
						case 0x88:
							return 4;
						case 0x89:
							return 2;
						case 0x8a:
							return 2;
						case 0x8b:
							event_mode = 0;
							return 1;
						default:
							break;
					}
					break;
				case 2:
					switch(curInstPtr[0]) {
						case 0x00:
							return 1;	/* nop */
						case 1:
							event_mode = 0;
							return 1;
						case 2:
							return 1;
						case 3:
							return 1;
						case 4:
							return 1;
						case 5:
							return 4;
						case 6:
							return 4;
						case 7:
							return 8;
						case 8:
							return 3;
						case 9:
							return 2;
						case 10:
							return 4;
						case 11:
							return 8;
						default:
							break;
					}
					break;
				case 3:
					/* mode 3 does not seem to be used */
					return 2;
			}
			break;
	}

	return 1;
}

static void eventDumpBlock(room_t *this, event_inst_t *inst, Uint32 offset, int length, int indent)
{
	event_inst_t *block_ptr;
	int inst_len, block_len;
	
	event_mode = 0;

	while (length>0) {
		block_ptr = NULL;

		memset(strBuf, 0, sizeof(strBuf));

		inst_len = rdt1_evt_eventGetInstLen(this, (Uint8 *) inst);

		if (params.verbose>=2) {
			int i;
			Uint8 *curInst = (Uint8 *) inst;

			sprintf(tmpBuf, "0x%08x: #", offset);
			strcat(strBuf, tmpBuf);
			for (i=0; i<inst_len; i++) {
				sprintf(tmpBuf, " 0x%02x", curInst[i]);
				strcat(strBuf, tmpBuf);
			}
			strcat(strBuf, "\n");
			logMsg(2, strBuf);

			memset(strBuf, 0, sizeof(strBuf));
		}
		/*reindent(indent);*/

		sprintf(tmpBuf, "INST_%02x\n", inst->opcode);
		strcat(strBuf, tmpBuf);

		logMsg(1, "0x%08x: %s", offset, strBuf);

		if (inst_len==0) {
			break;
		}

		length -= inst_len;
		offset += inst_len;
		inst = (event_inst_t *) (&((Uint8 *) inst)[inst_len]);
	}
}

#endif /* ENABLE_SCRIPT_DISASM */

/*
00:1:inc value
01:1:store 1
02:1:store 2,some copies
03:1:store 2
04:4+2:call function, 04 NN (N from 0 to 3) MM
05:1+3:call function, 05 NN MM PP
06:NN:exec script at inst+2, 06 NN ....
07:NN:exec script bytecode:07 NN ....
08:2:08 NN
09:2:09 NN

80:o1 :1:nop?
81:o2 :1
82:o3 :1
83:o4 :1
84:o5 :4
85:o6 :2
86:o7 :1
87:o8 :2
88:o9 :2
89:o10:
8a:o11:
8b:o12:1

f6:1
f7:1
f8:1,??,word used to init stuff
f9:0 or 3
fa:4
fb:1 or variable
fc:variable
fd:script bytescodes, variable
fe:1:nop
ff:0:exit event: return value in eax[2] ?

0xf6	1		process stuff, continue with 0xf7 stuff
0xf7	1		on condition, continue with 0xfe stuff
0xf8	?		continue with 0xf9 stuff
0xf9	3		on condition, continue with 0xfe stuff
0xfa 	4		process stuff
0xfb 	1		can jump to other address
0xfc	ptr[1].b	process stuff, use ptr[2].w, add ptr[1].b, store curptr
0xfd	1		execute SCD stuff, jump to other address
*/
