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
} event_inst_t;

/*--- Functions prototypes ---*/

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
		Uint16 func_offset = SDL_SwapLE32(functionArrayPtr[i]);
		Uint32 func_len = script_length - func_offset;
		event_inst_t *startInst = (event_inst_t *) (& ((Uint8 *) this->file)[offset + func_offset]);

		if (i<num_funcs-1) {
			func_len = SDL_SwapLE32(functionArrayPtr[i+1]) - func_offset;
		}

		logMsg(1, "0x%08x (file at 0x%08x): BEGIN_EVENT event%02x\n", func_offset, offset+func_offset, i);
		eventDumpBlock(this, startInst, func_offset, func_len, 1);
		logMsg(1, "          : END_EVENT\n\n");
	}
}

static void eventDumpBlock(room_t *this, event_inst_t *inst, Uint32 offset, int length, int indent)
{
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
