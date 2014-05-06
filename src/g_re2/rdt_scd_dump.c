/*
	Room description
	RE2 RDT script dumper

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

#include "rdt.h"

#include "rdt_scd_defs.gen.h"
#include "rdt_scd_types.gen.h"

#ifndef ENABLE_SCRIPT_DISASM

void rdt2_scd_scriptDump(room_t *this, int num_script)
{
}

#else

/*--- Defines ---*/

/* Effect sprites */

#define ESPR_LIGHT	0x03
#define ESPR_OBSTACLE	0x04
#define ESPR_TYPEWRITER	0x09
#define ESPR_BOX	0x0a
#define ESPR_FIRE	0x0b

/* possible room objects:
	inst2c: espr3d , fire can do damage
	inst3b: door
	inst4e: item
	inst67: wall
	inst68: ?, ptr+2 stored in object list, like others
	inst69: ?, ptr+2 stored in object list, like others
	inst8d: ?, ptr+2 stored in object list, like others
*/

/*--- Types ---*/

typedef struct {
	Uint8 id;
	Uint8 hexa;	/* display as : 0: decimal, 1:hexa */
	const char *name;
} em_var_name_t;

typedef struct {
	Uint8 array;
	Uint8 bit;
	const char *name;
} bitarray_name_t;

typedef struct {
	Uint8 model;
	const char *name;
} em_model_name_t;

/*--- Constants ---*/

static const em_var_name_t em_var_name[]={
	/*
	{0x00, 1, ""},	em[0x00].w
	{0x01, 1, ""},	em[0x02].w
	{0x02, 1, ""},	em[0x04].b
	{0x03, 1, ""},	em[0x05].b
	{0x04, 1, ""},	em[0x06].b
	{0x05, 1, ""},	em[0x07].b
	{0x06, 1, ""},	em[0x08].b
	*/
	{0x07, 1, "#EM_POSE"},	/* em[0x10e].w */
	/*
	{0x08, 1, ""},	em[0x0a].b
	{0x09, 1, ""},	em[0x0b].b
	{0x0a, 1, ""},	em[0x10].l
	*/
	{0x0b, 0, "#EM_X_POS"},	/* em[0x38].l */
	{0x0c, 0, "#EM_Y_POS"},	/* em[0x3c].l */
	{0x0d, 0, "#EM_Z_POS"},	/* em[0x40].l */
	{0x0e, 0, "#EM_X_ANGLE"},	/* em[0x74].w */
	{0x0f, 0, "#EM_Y_ANGLE"},	/* em[0x76].w */
	{0x10, 0, "#EM_Z_ANGLE"},	/* em[0x78].w */
	/*
	{0x11, 1, ""},	em[0x106].b
	{0x12, 1, ""},	em[0x154].w
	{0x13, 1, ""},	em[0x1c2].w*/
	{0x14, 0, "#EM_X_POS_NEW"},	/*em[0x1c4].w*/
	{0x15, 0, "#EM_Z_POS_NEW"}	/*em[0x1c6].w*/
	/*{0x16, 1, ""},	em[0x1cc].w
	{0x17, 1, ""},	em[0x1d4].w
	{0x18, 1, ""},	em[0x1d6].w
	{0x19, 1, ""},	em[0x1d8].w
	{0x1a, 1, ""},	em[0x1da].w
	{0x1b, 1, ""},	em[0x144].b
	{0x1c, 1, ""},	em[0x144].w
	{0x1d, 1, ""},	em[0x146].w
	{0x1e, 1, ""},	em[0x148].w
	{0x1f, 1, ""},	em[0x14e].b
	{0x20, 1, ""},	em[0x94].w
	{0x21, 1, ""},	em[0x98].w
	{0x22, 1, ""},	em[0x96].w
	{0x23, 1, ""},	em[0x9a].w
	{0x24, 1, ""},	em[0x9e].w
	{0x25, 1, ""},	em[0x9c].w
	{0x26, 1, ""},	em[0x1de].w
	{0x27, 1, ""},	em[0x118].w
	{0x28, 1, ""},	em[0x11a].w
	{0x29, 1, ""},	em[0x218].w
	{0x2a, 1, ""},	em[0x21a].w
	{0x2b, 1, ""},	em[0x1d3].b
	*/
};

static const bitarray_name_t bitarray_names[]={
	{0, 0x19, "game.difficulty"},

	{1, 0x00, "game.character"},
	{1, 0x01, "game.scenario"},
	{1, 0x06, "game.type"},
	{1, 0x1b, "game.letterbox"},
	
	{2, 0x07, "room.mutex"},
	
	{4, 0x05, "room2010.seen_licker"},
	{4, 0x06, "room2000.first_visit"},
	{4, 0x12, "room10b0.put_jewel1"},
	{4, 0x13, "room10b0.put_jewel2"},
	{4, 0x1a, "room1010.already_visited"},
	{4, 0x1b, "room1010.kendo_attacked"},
	{4, 0x3a, "room3090.ladder_down"},
	{4, 0x48, "room2000.put_medal"},
	{4, 0x55, "room2040.first_visit"},
	{4, 0x56, "room2040.met_licker"},
	{4, 0x5b, "room60c0.registered_fingerprint"},
	{4, 0x88, "room7000.electricity_enabled"},
	{4, 0x8c, "room7000.gates_opened"},
	{4, 0x8e, "room60c0.fingerprint_ok_scenario_a"},
	{4, 0x8f, "room60c0.fingerprint_ok_scenario_b"},
	{4, 0x90, "room7010.opened_sockets_stock"},
	{4, 0x94, "room1100.ladder_down"},
	{4, 0x99, "room2040.cord_broken"},
	{4, 0xaf, "room2070.used_specialkey"},
	
	{5, 0x02, "room2040.corpse_examined"},
	{5, 0x03, "room1010.kendo_examined"},
	{5, 0x12, "room1030.enable_brad"},
	
	{0x0b, 0x1f, "player_answer"},
	
	{0x1d, 0x02, "room60c0.door_unlocked_scenario_a"},
	{0x1d, 0x09, "room2040.cord_on_shutter"},
	{0x1d, 0x0a, "room20f0.cord_on_shutter"},
	{0x1d, 0x11, "room1030.met_brad"}
};

/*--- Variables ---*/

static char strBuf[512];
static char tmpBuf[512];

/*--- Functions prototypes ---*/

static void reindent(int num_indent);
static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent);
static void getBitArrayName(char *dest, int num_array, int num_bit);

/*--- Functions ---*/

void rdt2_scd_scriptDump(room_t *this, int num_script)
{
	rdt2_header_t *rdt_header;
	Uint32 offset, smaller_offset, script_length = 0;
	Uint16 *functionArrayPtr;
	int i, num_funcs, room_script = RDT2_OFFSET_INIT_SCRIPT;

	assert(this);

	if (num_script == ROOM_SCRIPT_RUN) {
		room_script = RDT2_OFFSET_ROOM_SCRIPT;
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

	logMsg(1, "rdt: Dumping script %d\n", num_script);
	num_funcs = SDL_SwapLE16(functionArrayPtr[0]) >> 1;
	for (i=0; i<num_funcs; i++) {
		Uint16 func_offset = SDL_SwapLE16(functionArrayPtr[i]);
		Uint32 func_len = script_length - func_offset;
		script_inst_t *startInst = (script_inst_t *) (& ((Uint8 *) this->file)[offset + func_offset]);

		if (i<num_funcs-1) {
			func_len = SDL_SwapLE16(functionArrayPtr[i+1]) - func_offset;
		}

		logMsg(1, "0x%08x: BEGIN_EVENT event%02x\n", func_offset, i);
		scriptDumpBlock(this, startInst, func_offset, func_len, 1);
		logMsg(1, "          : END_EVENT\n\n");
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

		if (block_ptr) {
			int next_len;
			
			if (inst->opcode == INST_IFEL_CK) {
				script_inst_t *end_block_ptr;

				end_block_ptr = (script_inst_t *) (&((Uint8 *) block_ptr)[block_len-2]);
				if (end_block_ptr->opcode == INST_ENDIF) {
					block_len += 2;
				}
			}

			next_len = block_len - inst_len;
			if (inst->opcode == INST_CASE) next_len = block_len;
			if (inst->opcode == INST_WHILE) next_len = block_len - 2;
			if (inst->opcode == INST_DO) next_len = block_len - 2;

			logMsg(2, " block 0x%04x inst 0x%04x next 0x%04x\n", block_len, inst_len, next_len);
			scriptDumpBlock(this, (script_inst_t *) block_ptr, offset+inst_len, next_len, indent+1);
			logMsg(2, " block end\n");

			if (inst->opcode == INST_DO) next_len += sizeof(script_inst_do_t);
			inst_len += next_len;
		}

		if (inst_len==0) {
			break;
		}

		length -= inst_len;
		offset += inst_len;
		inst = (script_inst_t *) (&((Uint8 *) inst)[inst_len]);
		logMsg(2, "instlen %d, len %d\n", inst_len, length);
	}
}

static void getBitArrayName(char dest[40], int num_array, int num_bit)
{
	int i;

	for (i=0; i<sizeof(bitarray_names)/sizeof(bitarray_name_t); i++) {
		if (bitarray_names[i].array != num_array) continue;
		if (bitarray_names[i].bit != num_bit) continue;
		strcpy(dest, bitarray_names[i].name);
		return;
	}

	sprintf(dest, "array 0x%02x, bit 0x%02x", num_array, num_bit);
}

#endif /* ENABLE_SCRIPT_DISASM */
