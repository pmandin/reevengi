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

#include "rdt_scd_common.h"

#ifndef ENABLE_SCRIPT_DISASM

void rdt3_scd_scriptDump(room_t *this, int num_script)
{
}

#else

/*--- Defines ---*/

#define INST_END_SCRIPT	0xff

/*--- Types ---*/

typedef struct {
	Uint8 value;
	char *name;
} script_dump_t;

/*--- Constants ---*/

static const script_dump_t work_set_0[]={
	{0, "NO_WK"},
	{1, "PL_WK"},
	{2, "SPL_WK"},
	{3, "EM_WK"},
	{4, "OM_WK"},
	{6, "ALL_WK"},
	
	{0x80, "PL_PARTS_WK"},
	{0xa0, "SPL_PARTS_WK"},
	{0xc0, "EM_PARTS_WK"},
	{0xe0, "OM_PARTS_WK"}
};

/*--- Variables ---*/

static char strBuf[256];
static char tmpBuf[256];

/*--- Functions prototypes ---*/

static void reindent(int num_indent);
static const char *getNameFromValue(int value, const script_dump_t *array, int num_array_items);
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

		logMsg(1, "0x%08x: BEGIN_FUNC func%02x\n", func_offset, i);
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

static const char *getNameFromValue(int value, const script_dump_t *array, int num_array_items)
{
	int i;

	for (i=0; i<num_array_items; i++) {
		if (array[i].value == value) {
			return array[i].name;
		}
	}

	return NULL;
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

		switch(inst->opcode) {

			/* 0x00-0x0f */

			case INST_NOP:
				strcat(strBuf, "nop\n");
				break;
			case INST_EVT_END:
				strcat(strBuf, "EVT_END\n");
				break;
			case INST_EVT_NEXT:
				strcat(strBuf, "EVT_NEXT\n");
				break;
			case INST_EVT_CHAIN:
				strcat(strBuf, "EVT_CHAIN xx\n");
				break;
			case INST_EVT_EXEC:
				sprintf(tmpBuf, "EVT_EXEC 0x%02x func%02x()\n",
					inst->exec.mask, inst->exec.func[1]);
				strcat(strBuf, tmpBuf);
				break;
			case INST_EVT_KILL:
				strcat(strBuf, "EVT_KILL xxx\n");
				break;
			case INST_IF:
				strcat(strBuf, "BEGIN_IF\n");
				block_len = SDL_SwapLE16(inst->i_if.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_if_t)]);
				{
					script_inst_t *end_block_ptr = (script_inst_t *) (&((Uint8 *) block_ptr)[block_len-2]);
					if (end_block_ptr->opcode == INST_END_IF) {
						block_len += 2;
					}
				}				
				break;
			case INST_ELSE:
				strcat(strBuf, "ELSE\n");
				block_len = SDL_SwapLE16(inst->i_else.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_else_t)]);
				break;
			case INST_END_IF:
				strcat(strBuf, "END_IF\n");
				break;
			case INST_SLEEP:
				strcat(strBuf, "SLEEP\n");
				break;
			case INST_SLEEPING:
				sprintf(tmpBuf, "SLEEPING %d\n", SDL_SwapLE16(inst->sleepn.delay));
				strcat(strBuf, tmpBuf);
				break;
			case INST_WSLEEP:
				strcat(strBuf, "WSLEEP\n");
				break;
			case INST_WSLEEPING:
				strcat(strBuf, "WSLEEPING xx\n");
				break;
			case INST_BEGIN_FOR:
				sprintf(tmpBuf, "BEGIN_FOR %d\n", SDL_SwapLE16(inst->i_for.count));
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->i_for.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_for_t)]);
				break;
			case INST_END_FOR:
				strcat(strBuf, "END_FOR\n");
				break;

			/* 0x10-0x1f */

			case INST_BEGIN_WHILE:
				strcat(strBuf, "BEGIN_WHILE\n");
				block_len = inst->begin_while.block_length;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_begin_while_t)]);
				break;
			case INST_END_WHILE:
				strcat(strBuf, "END_WHILE\n");
				break;
			case INST_DO:
				strcat(strBuf, "DO\n");
				block_len = SDL_SwapLE16(inst->i_do.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_do_t)]);
				break;
			case INST_WHILE:
				strcat(strBuf, "WHILE\n");
				block_len = inst->i_while.block_length;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_while_t)]);
				break;
			case INST_BEGIN_SWITCH:
				strcat(strBuf, "BEGIN_SWITCH\n");
				/*strcat(strBuf, tmpBuf);*/
				block_len = SDL_SwapLE16(inst->i_switch.block_length)+2;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_begin_switch_t)]);
				break;
			case INST_CASE:
				/*strcat(strBuf, "CASE\n");*/
				sprintf(tmpBuf, "CASE 0x%04x\n", SDL_SwapLE16(inst->i_case.value));
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->i_case.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_case_t)]);
				break;
			case INST_DEFAULT:
				strcat(strBuf, "DEFAULT\n");
				break;
			case INST_END_SWITCH:
				strcat(strBuf, "END_SWITCH\n");
				break;
			case INST_GOTO:
				strcat(strBuf, "GOTO xxx\n");
				break;
			case INST_GOSUB:
				sprintf(tmpBuf, "GOSUB func%02x()\n", inst->func.num_func);
				strcat(strBuf, tmpBuf);
				break;
			case INST_RETURN:
				strcat(strBuf, "RETURN\n");
				break;
			case INST_BREAK:
				strcat(strBuf, "BREAK\n");
				break;
			case INST_EVAL_CC:
				strcat(strBuf, "EVAL_CC xxx\n");
				break;
			/*case 0x1e:
			case 0x1f:
				strcat(strBuf, "set xxx\n");
				break;*/

			/* 0x20-0x2f */

			case INST_CALC_OP:
				strcat(strBuf, "CALC_OP xxx\n");
				break;
			case INST_EVT_CUT:
				strcat(strBuf, "EVT_CUT xxx\n");
				break;
			case INST_CHASER_EVT_CLR:
				strcat(strBuf, "CHASER_EVT_CLR xxx\n");
				break;
			case INST_MAP_OPEN:
				strcat(strBuf, "MAP_OPEN xxx\n");
				break;
			case INST_POINT_ADD:
				strcat(strBuf, "POINT_ADD xxx\n");
				break;
			case INST_DOOR_CK:
				strcat(strBuf, "DOOR_CK xxx\n");
				break;
			case INST_DIEDEMO_ON:
				strcat(strBuf, "DIEDEMO_ON xxx\n");
				break;
			case INST_DIR_CK:
				strcat(strBuf, "DIR_CK xxx\n");
				break;
			case INST_PARTS_SET:
				strcat(strBuf, "PARTS_SET xxx\n");
				break;
			case INST_VLOOP_SET:
				strcat(strBuf, "VLOOP_SET xxx\n");
				break;
			case INST_OTA_BE_SET:
				strcat(strBuf, "OTA_BE_SET xxx\n");
				break;
			case INST_LINE_BEGIN:
				strcat(strBuf, "LINE_BEGIN xxx\n");
				break;
			case INST_LINE_MAIN:
				strcat(strBuf, "LINE_MAIN xxx\n");
				break;
			case INST_LINE_END:
				strcat(strBuf, "LINE_END xxx\n");
				break;

			/* 0x30-0x3f */

			case INST_LIGHT_POS_SET:
				strcat(strBuf, "LIGHT_POS_SET xxx\n");
				break;
			case INST_LIGHT_KIDO_SET:
				strcat(strBuf, "LIGHT_KIDO_SET xxx\n");
				break;
			case INST_LIGHT_COLOR_SET:
				strcat(strBuf, "LIGHT_COLOR_SET xxx\n");
				break;
			case INST_AHEAD_ROOM_SET:
				sprintf(tmpBuf, "AHEAD_ROOM_SET 0x%04x\n", SDL_SwapLE16(inst->ahead_room_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_ESPR_CTR:
				strcat(strBuf, "ESPR_CTR xxx\n");
				break;
			case INST_BGM_TBL_CK:
				strcat(strBuf, "BGM_TBL_CK xxx\n");
				break;
			case INST_ITEM_GET_CK:
				strcat(strBuf, "ITEM_GET_CK xxx\n");
				break;
			case INST_OM_REV:
				strcat(strBuf, "OM_REV xxx\n");
				break;
			case INST_CHASER_LIFE_INIT:
				strcat(strBuf, "CHASER_LIFE_INIT xxx\n");
				break;
			case INST_PARTS_BOMB:
				strcat(strBuf, "PARTS_BOMB xxx\n");
				break;
			case INST_PARTS_DOWN:
				strcat(strBuf, "PARTS_DOWN xxx\n");
				break;
			case INST_CHASER_ITEM_SET:
				strcat(strBuf, "CHASER_ITEM_SET xxx\n");
				break;
			case INST_WEAPON_CHG_OLD:
				strcat(strBuf, "WEAPON_CHG_OLD xxx\n");
				break;
			case INST_SEL_EVT_ON:
				strcat(strBuf, "SEL_EVT_ON xxx\n");
				break;
			case INST_ITEM_LOST:
				strcat(strBuf, "ITEM_LOST xxx\n");
				break;
			case INST_FLOOR_SET:
				strcat(strBuf, "FLOOR_SET xxx\n");
				break;

			/* 0x40-0x4f */

			case INST_MEMB_SET:
				{
					script_var_set_t *varSet = (script_var_set_t *) inst;

					sprintf(tmpBuf, "MEMB_SET var%02x = %d\n", varSet->num_var, SDL_SwapLE16(varSet->value));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_MEMB_SET2:
				strcat(strBuf, "MEMB_SET2 xxx\n");
				break;
			case INST_MEMB_CPY:
				strcat(strBuf, "MEMB_CPY xxx\n");
				break;
			case INST_MEMB_CMP:
				strcat(strBuf, "MEMB_CMP xxx\n");
				break;
			case INST_MEMB_CALC:
				strcat(strBuf, "MEMB_CALC xxx\n");
				break;
			case INST_MEMB_CALC2:
				strcat(strBuf, "MEMB_CALC2 xxx\n");
				break;
			case INST_FADE_SET:
				strcat(strBuf, "FADE_SET xxx\n");
				break;
			case INST_WORK_SET:
				{
					const char *nameFromValue;

					nameFromValue = getNameFromValue(inst->work_set.unknown, work_set_0,
						sizeof(work_set_0)/sizeof(script_dump_t));
					if (nameFromValue) {
						sprintf(tmpBuf, "WORK_SET %s 0x%02x\n", nameFromValue, inst->work_set.object);
					} else {
						sprintf(tmpBuf, "WORK_SET 0x%02x 0x%02x\n", inst->work_set.unknown, inst->work_set.object);
					}
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_SPD_SET:
				strcat(strBuf, "SPD_SET xxx\n");
				break;
			case INST_ADD_SPD:
				strcat(strBuf, "ADD_SPD xxx\n");
				break;
			case INST_ADD_ASPD:
				strcat(strBuf, "ADD_ASPD xxx\n");
				break;
			case INST_ADD_VSPD:
				strcat(strBuf, "ADD_VSPD xxx\n");
				break;
			case INST_CK:
				strcat(strBuf, "CK xxx\n");
				break;
			case INST_SET:
				sprintf(tmpBuf, "SET 0x%02x object 0x%02x %s\n",
					inst->set_flag.flag, inst->set_flag.object,
					(inst->set_flag.value ? "on" : "off"));
				strcat(strBuf, tmpBuf);
				break;
			case INST_CMP:
				strcat(strBuf, "CMP xxx\n");
				break;
			case INST_RND:
				strcat(strBuf, "RND xxx\n");
				break;

			/* 0x50-0x5f */

			case INST_CUT_CHG:
				strcat(strBuf, "CUT_CHG xxx\n");
				break;
			case INST_CUT_OLD:
				strcat(strBuf, "CUT_OLD xxx\n");
				break;
			case INST_CUT_AUTO:
				strcat(strBuf, "CUT_AUTO xxx\n");
				break;
			case INST_CUT_REPLACE:
				strcat(strBuf, "CUT_REPLACE xxx\n");
				break;
			case INST_CUT_BE_SET:
				strcat(strBuf, "CUT_BE_SET xxx\n");
				break;
			case INST_POS_SET:
				strcat(strBuf, "POS_SET xxx\n");
				break;
			case INST_DIR_SET:
				strcat(strBuf, "DIR_SET xxx\n");
				break;
			case INST_SET_VIB0:
				strcat(strBuf, "SET_VIB0 xxx\n");
				break;
			case INST_SET_VIB1:
				strcat(strBuf, "SET_VIB1 xxx\n");
				break;
			case INST_SET_VIB_FADE:
				strcat(strBuf, "SET_VIB_FADE xxx\n");
				break;
			case INST_RBJ_SET:
				strcat(strBuf, "RBJ_SET xxx\n");
				break;
			case INST_MESSAGE_ON:
				strcat(strBuf, "MESSAGE_ON xxx\n");
				break;
			case INST_RAIN_SET:
				strcat(strBuf, "RAIN_SET xxx\n");
				break;
			case INST_MESSAGE_OFF:
				strcat(strBuf, "MESSAGE_OFF xxx\n");
				break;
			case INST_SHAKE_ON:
				strcat(strBuf, "SHAKE_ON xxx\n");
				break;
			case INST_WEAPON_CHG:
				strcat(strBuf, "WEAPON_CHG xxx\n");
				break;

			/* 0x60-0x6f */

			case INST_DOOR_SET:
				strcat(strBuf, "DOOR_SET xxx\n");
				break;
			case INST_AOT_SET:
				strcat(strBuf, "AOT_SET xxx\n");
				break;
			case INST_AOT_SET_4P:
				strcat(strBuf, "AOT_SET_4P xxx\n");
				break;
			case INST_AOT_RESET:
				strcat(strBuf, "AOT_RESET xxx\n");
				break;
			case INST_ITEM_AOT_SET:
				strcat(strBuf, "ITEM_AOT_SET xxx\n");
				break;
			case INST_KAGE_SET:
				strcat(strBuf, "KAGE_SET xxx\n");
				break;
			case INST_SUPER_SET:
				strcat(strBuf, "SUPER_SET xxx\n");
				break;
			case INST_SCA_ID_SET:
				strcat(strBuf, "SCA_ID_SET xxx\n");
				break;

			/* 0x70-0x7f */

			case INST_ESPR_ON:
				strcat(strBuf, "ESPR_ON xxx\n");
				break;
			case INST_ESPR3D_ON2:
				strcat(strBuf, "ESPR3D_ON2 xxx\n");
				break;
			case INST_ESPR_KILL:
				strcat(strBuf, "ESPR_KILL xxx\n");
				break;
			case INST_ESPR_KILL2:
				strcat(strBuf, "ESPR_KILL2 xxx\n");
				break;
			case INST_SE_ON:
				strcat(strBuf, "SE_ON xxx\n");
				break;
			case INST_BGM_CTL:
				strcat(strBuf, "BGM_CTL xxx\n");
				break;
			case INST_XA_ON:
				strcat(strBuf, "XA_ON xxx\n");
				break;
			case INST_BGM_TBL_SET:
				strcat(strBuf, "BGM_TBL_SET xxx\n");
				break;
			case INST_EM_SET:
				strcat(strBuf, "EM_SET xxx\n");
				break;
			case INST_OM_SET:
				strcat(strBuf, "OM_SET xxx\n");
				break;

			/* 0x80-0x8f */

			case INST_PLC_MOTION:
				strcat(strBuf, "PLC_MOTION xxx\n");
				break;
			case INST_PLC_DEST:
				strcat(strBuf, "PLC_DEST xxx\n");
				break;
			case INST_PLC_NECK:
				strcat(strBuf, "PLC_NECK xxx\n");
				break;
			case INST_PLC_RET:
				strcat(strBuf, "PLC_RET xxx\n");
				break;
			case INST_PLC_FLG:
				strcat(strBuf, "PLC_FLG xxx\n");
				break;
			case INST_PLC_STOP:
				strcat(strBuf, "PLC_STOP xxx\n");
				break;
			case INST_PLC_ROT:
				strcat(strBuf, "PLC_ROT xxx\n");
				break;
			case INST_PLC_CNT:
				strcat(strBuf, "PLC_CNT xxx\n");
				break;

			default:
				sprintf(tmpBuf, "Unknown opcode 0x%02x\n", inst->opcode);
				strcat(strBuf, tmpBuf);
				break;
		}

		logMsg(1, "0x%08x: %s", offset, strBuf);

		if (block_ptr) {
			int next_len = block_len - inst_len;
			if (inst->opcode == INST_CASE) next_len = block_len;
			if (inst->opcode == INST_BEGIN_WHILE) next_len = block_len - 2;
			if (inst->opcode == INST_BEGIN_FOR) next_len = block_len - 2;
			/*logMsg(1, " block 0x%04x inst 0x%04x next 0x%04x\n", block_len, inst_len, next_len);*/

			scriptDumpBlock(this, (script_inst_t *) block_ptr, offset+inst_len, next_len, indent+1);

			if (inst->opcode == INST_DO) next_len += sizeof(script_do_t);
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
