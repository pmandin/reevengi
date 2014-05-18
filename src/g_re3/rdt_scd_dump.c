/*
	Room description
	RE3 RDT script dumper

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
#include "../g_re2/rdt.h"

#include "rdt_scd_defs.gen.h"
#include "rdt_scd_types.gen.h"

#ifndef ENABLE_SCRIPT_DISASM

void rdt3_scd_scriptDump(room_t *this, int num_script)
{
}

#else

/*--- Defines ---*/

#define INST_END_SCRIPT	0xff

/*--- Constants ---*/

#include "rdt_scd_enums.gen.c"

/*--- Variables ---*/

static char strBuf[512];
static char tmpBuf[512];

/*--- Functions prototypes ---*/

static void reindent(int num_indent);
static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent);

/*--- Functions ---*/

void rdt3_scd_scriptDump(room_t *this, int num_script)
{
	rdt2_header_t *rdt_header;
	Uint32 offset, smaller_offset, script_length = 0;
	Uint16 *functionArrayPtr;
	int i, num_funcs, room_script = RDT2_OFFSET_INIT_SCRIPT;

	assert(this);

	if (num_script == ROOM_SCRIPT_RUN) {
		return;
		/*room_script = RDT2_OFFSET_ROOM_SCRIPT;*/
	}

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[room_script]);

	if (offset==0) {
		return;
	}

	/* Search smaller offset after script to calc length */
	smaller_offset = this->file_length;
	for (i=0; i<21; i++) {
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

	/* Start of script is an array of offsets to the various script functions
	 * The first offset also gives the first instruction to execute
	 */
	functionArrayPtr = (Uint16 *) (& ((Uint8 *) this->file)[offset]);

	logMsg(1, "rdt3: Dumping script %d\n", num_script);
	num_funcs = SDL_SwapLE16(functionArrayPtr[0]) >> 1;
	for (i=0; i<num_funcs; i++) {
		Uint16 func_offset = SDL_SwapLE16(functionArrayPtr[i]);
		Uint32 func_len = script_length - func_offset;
		script_inst_t *startInst = (script_inst_t *) (& ((Uint8 *) this->file)[offset + func_offset]);

		if (i<num_funcs-1) {
			func_len = SDL_SwapLE16(functionArrayPtr[i+1]) - func_offset;
		}

		logMsg(1, "0x%08x: BEGIN_FUNC event%02x\n", func_offset, i);
		scriptDumpBlock(this, startInst, func_offset, func_len, 1);
		logMsg(1, "          : END_FUNC\n\n");
	}
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

		switch(inst->opcode) {
			/*case INST_AOT_SET:
				{
					this->getText(this, 0, SDL_SwapLE16(inst->i_aot_set.message), tmpBuf, sizeof(tmpBuf));
					logMsg(1, "0x%08x: #\t%s\n", offset, tmpBuf);
				}
				break;*/
			case INST_MESSAGE_ON:
				{
					this->getText(this, 0, inst->i_message_on.id, tmpBuf, sizeof(tmpBuf));
					logMsg(1, "0x%08x: #\tL0\t%s\n", offset, tmpBuf);

					this->getText(this, 1, inst->i_message_on.id, tmpBuf, sizeof(tmpBuf));
					sprintf(strBuf, "#\tL1\t%s\n", tmpBuf);
				}
				break;
		}

		if (block_ptr) {
			int next_len;

			if (inst->opcode == INST_IF) {
				script_inst_t *end_block_ptr;

				end_block_ptr = (script_inst_t *) (&((Uint8 *) block_ptr)[block_len-2]);
				if (end_block_ptr->opcode == INST_ENDIF) {
					block_len += 2;
				}
			}

			next_len = block_len - inst_len;
			if (inst->opcode == INST_CASE) next_len = block_len;
			if (inst->opcode == INST_WHILE) next_len = block_len - 2;
			if (inst->opcode == INST_FOR) next_len = block_len - 2;
			if (inst->opcode == INST_SWITCH) next_len = block_len - 2;
			if (inst->opcode == INST_DO) next_len = block_len - 2;

			scriptDumpBlock(this, (script_inst_t *) block_ptr, offset+inst_len, next_len, indent+1);

			if (inst->opcode == INST_DO) next_len += sizeof(script_inst_do_t);
			inst_len += next_len;
		}

		if (inst_len==0) {
			break;
		}

		length -= inst_len;
		offset += inst_len;
		inst = (script_inst_t *) (&((Uint8 *) inst)[inst_len]);
		/*printf("instlen %d, len %d\n", inst_len, length);*/
	}
}

#endif /* ENABLE_SCRIPT_DISASM */
