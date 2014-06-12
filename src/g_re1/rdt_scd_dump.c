/*
	Room description
	RE1 RDT script dumper

	Copyright (C) 2009-2010	Patrice Mandin

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

#include "rdt_scd_defs.gen.h"
#include "rdt_scd_types.gen.h"

#ifndef ENABLE_SCRIPT_DISASM

void rdt1_scd_scriptDump(room_t *this, int num_script)
{
}

#else

/*--- Defines ---*/

/* Item types */

#define ITEM_MESSAGE	0x02
#define ITEM_OBSTACLE	0x02
#define ITEM_TRIGGER1	0x07	/* riddle, event, movable object */
#define ITEM_BOX	0x08	/* deposit box */
#define ITEM_OBJECT	0x09	/* pickable object */
#define ITEM_TRIGGER2	0x09
#define ITEM_TYPEWRITER	0x10

/*--- Types ---*/

/*--- Constants ---*/

#include "rdt_scd_enums.gen.c"

/*--- Variables ---*/

static char strBuf[512];
static char tmpBuf[512];
/*static char tmpFuncs[256];*/

/*--- Functions prototypes ---*/

static void reindent(int num_indent);
static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent);

/*--- Functions ---*/

void rdt1_scd_scriptDump(room_t *this, int num_script)
{
	rdt1_header_t *rdt_header;
	Uint32 offset, script_length;
	Uint8 *scriptPtr;
	int i, room_script = RDT1_OFFSET_INIT_SCRIPT;

	if (num_script == ROOM_SCRIPT_RUN) {
		room_script = RDT1_OFFSET_ROOM_SCRIPT;
	}

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[room_script]);

	if (offset==0) {
		return;
	}

	scriptPtr = & (((Uint8 *) this->file)[offset]);
	script_length = SDL_SwapLE16(*((Uint16 *) scriptPtr));

/*	memset(tmpFuncs, 0, sizeof(tmpFuncs));*/

	logMsg(1, "0x%08x: BEGIN_EVENT event00\n", offset+2);
	scriptDumpBlock(this, (script_inst_t *) &scriptPtr[2], offset+2, script_length-2, 1);
	logMsg(1, "          : END_EVENT\n\n");

#if 0
	/* Dump used functions */
	for (i=0; i<sizeof(tmpFuncs); i++) {
		if (i<0x10) {
			if ((i==0x0e))
				goto dump_print_inst;
			continue;
		}
		if (i<0x20) {
			if ((i==0x1c) || (i==0x1d))
				goto dump_print_inst;
			continue;
		}
		if (i<0x30) {
			if ((i==0x24) || (i==0x26) || (i==0x29) || (i==0x2d) || (i==0x2e))
				goto dump_print_inst;
			continue;
		}
		if (i<0x40) {
			if ((i==0x31) || (i==0x32) || (i==0x34) || (i==0x38) || (i==0x39) || (i==0x3a) || (i==0x3c) || (i==0x3d) || (i==0x3e) || (i==0x3f))
				goto dump_print_inst;
			continue;
		}
		if (i<0x50) {
			if ((i==0x42) || (i==0x45) || (i>=0x4a))
				goto dump_print_inst;
			continue;
		}


		if (i<0x51) {
				goto dump_print_inst;
		}

		continue;

dump_print_inst:
		if (tmpFuncs[i]) {
			logMsg(1, "Function 0x%02x used\n", i);
		}
	}
#endif
}

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

static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent)
{
	script_inst_t *block_ptr;
	int inst_len, block_len;

	while (length>0) {
		block_ptr = NULL;

		memset(strBuf, 0, sizeof(strBuf));

		inst_len = this->scriptGetInstLen(this, (Uint8 *) inst);

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
		reindent(indent);

#include "rdt_scd_dumps.gen.c"

		logMsg(1, "0x%08x: %s", offset, strBuf);
/*		tmpFuncs[inst->opcode] = 1;*/

		if (block_ptr) {
			int next_len = block_len - inst_len;

			scriptDumpBlock(this, (script_inst_t *) block_ptr, offset+inst_len, next_len, indent+1);

			inst_len += next_len;
		}

		if (inst_len==0) {
			break;
		}

		length -= inst_len;
		offset += inst_len;
		inst = (script_inst_t *) (&((Uint8 *) inst)[inst_len]);
	}
}

#endif /* ENABLE_SCRIPT_DISASM */
