/*
	Room description
	RE3 RDT script

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

/*--- Defines ---*/

#define INST_NOP	0x00
#define INST_RETURN	0x01
#define INST_SLEEP_1	0x02
#define INST_IF		0x06
#define INST_IF_CK		0x4c
#define INST_IF_CMP		0x4e
#define INST_ELSE	0x07
#define INST_END_IF	0x08
#define INST_SLEEP_N	0x09
#define INST_WHILE	0x10
#define INST_SWITCH	0x14
#define INST_CASE	0x15
#define INST_END_SWITCH	0x17
#define INST_FUNC	0x19
#define INST_BREAK	0x1b
#define INST_FLOOR_SET	0x3f
#define INST_SETOBJFLAG	0x4d
#define INST_CUT_REPLACE	0x53
#define INST_MAKE_DOOR	0x61

#define INST_12		0x12
#define INST_12_LEN	18
#define INST_1E		0x1e
#define INST_1E_LEN	4
#define INST_33		0x33
#define INST_33_LEN	4
#define INST_47		0x47
#define INST_47_LEN	4
#define INST_57		0x57
#define INST_57_LEN	6
#define INST_58		0x58
#define INST_58_LEN	6
#define INST_59		0x59
#define INST_59_LEN	8
#define INST_63		0x63
#define INST_63_LEN	20
#define INST_65		0x65
#define INST_65_LEN	10
#define INST_67		0x67
#define INST_67_LEN	0x3e
#define INST_70		0x70
#define INST_70_LEN	16
#define INST_73		0x73
#define INST_73_LEN	24
#define INST_75		0x75
#define INST_75_LEN	2
#define INST_77		0x77
#define INST_77_LEN	12
#define INST_78		0x78
#define INST_78_LEN	6
#define INST_7D		0x7d
#define INST_7D_LEN	24
#define INST_7F		0x7f
#define INST_7F_LEN	40
#define INST_81		0x81
#define INST_81_LEN	8
#define INST_85		0x85
#define INST_85_LEN	8
#define INST_86		0x86
#define INST_86_LEN	(16*8+10)

/*--- Types ---*/

typedef struct {
	Uint8 opcode;
	Uint8 num_func;
} script_func_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown;
	Uint16 delay;
} script_sleepn_t;

typedef struct {
	Uint8 opcode;
	Uint8 flag;
	Uint8 object;
	Uint8 value;
} script_setobjflag_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown[0x30-1];
} script_make_door_t;

typedef struct {
	Uint8 opcode;
	Uint8 before;
	Uint8 after;
} script_cut_replace_t;

typedef struct {
	Uint8 opcode;
	Uint8 num_floor;
	Uint8 value;
} script_floor_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 skip_if_fail;
	Uint8 cond_func;	/* = INST_IF_CK */
	Uint8 value_flag;
	Uint8 value_obj;
	Uint8 value_comp;
} script_if_ck_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 skip_if_fail;
	Uint8 cond_func;	/* = INST_IF_CMP */
	Uint8 value_flag;
	Uint8 value_obj;
	Uint8 value_comp;
	Uint16 value2_comp;
} script_if_cmp_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 skip_if_fail;
	Uint8 cond_func;
} script_if_base_t;

typedef union {
	script_if_base_t base;
	script_if_ck_t	check;
	script_if_cmp_t	compare;
} script_if_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 skip_if_fail;
} script_else_if_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 num_object;
} script_switch_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 cmp;
	Uint16 value;
} script_case_t;

typedef struct {
	script_if_base_t base;
	script_if_ck_t	check;
	script_if_cmp_t	compare;
} script_while_t;

typedef union {
	Uint8 opcode;
	script_func_t func;
	script_sleepn_t sleepn;
	script_setobjflag_t setobjflag;
	script_make_door_t	make_door;
	script_floor_set_t	floor_set;
	script_cut_replace_t	cut_replace;
	script_if_t	begin_if;
	script_else_if_t	else_if;
	script_switch_t		switch_case;
	script_case_t		case_case;
	script_while_t		i_while;
} script_inst_t;

/*--- Variables ---*/

static script_inst_t *cur_inst;

static char indentStr[256];

/*--- Functions prototypes ---*/

static script_inst_t *scriptResetInst(room_t *this);
static script_inst_t *scriptNextInst(room_t *this);

static void reindent(int num_indent);
static void scriptDisasm(room_t *this);

/*--- Functions ---*/

void room_rdt3_scriptInit(room_t *this)
{
	scriptDisasm(this);
}

static script_inst_t *scriptResetInst(room_t *this)
{
	Uint32 *item_offset, offset;

	if (!this) {
		return NULL;
	}
	
	item_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+16*4]);
	offset = SDL_SwapLE32(*item_offset);
	cur_inst = (script_inst_t *) &((Uint8 *) this->file)[offset];

	return cur_inst;
}

static script_inst_t *scriptNextInst(room_t *this)
{
	int item_length = 0;
	Uint8 *next_inst = (Uint8 *) cur_inst;

	if (!this) {
		return NULL;
	}

	switch(cur_inst->opcode) {
		case INST_NOP:
			{
				if (cur_inst->func.num_func > 0) {		
					item_length = 1;
				} else {
					item_length = 2;
				}
			}
			break;
		case INST_END_SWITCH:
		case INST_BREAK:
		case INST_RETURN:
			item_length = 2;
			break;
		case INST_SLEEP_N:
			item_length = sizeof(script_sleepn_t);
			break;
		case INST_FUNC:
			item_length = sizeof(script_func_t);
			break;
		case INST_SETOBJFLAG:
			item_length = sizeof(script_setobjflag_t);
			break;
		case INST_MAKE_DOOR:
			item_length = sizeof(script_make_door_t);
			break;
		case INST_FLOOR_SET:
			item_length = sizeof(script_floor_set_t);
			break;
		case INST_CUT_REPLACE:
			item_length = sizeof(script_cut_replace_t);
			break;
		case INST_IF:
			{
				switch(cur_inst->begin_if.base.cond_func) {
					case INST_IF_CK:
						item_length = sizeof(script_if_ck_t);
						break;
					case INST_IF_CMP:
						item_length = sizeof(script_if_cmp_t);
						break;
				}
			}
			break;
		case INST_ELSE:
			item_length = sizeof(script_else_if_t);
			break;
		case INST_END_IF:
			item_length = 2;
			break;
		case INST_SWITCH:
			item_length = sizeof(script_switch_t);
			break;
		case INST_CASE:
			item_length = sizeof(script_case_t);
			break;
		case INST_WHILE:
			{
				switch(cur_inst->i_while.base.cond_func) {
					case INST_IF_CK:
						item_length = sizeof(script_if_ck_t);
						break;
					case INST_IF_CMP:
						item_length = sizeof(script_if_cmp_t);
						break;
				}
			}
			break;
		case INST_12:
			item_length = INST_12_LEN;
			break;
		case INST_1E:
			item_length = INST_1E_LEN;
			break;
		case INST_33:
			item_length = INST_33_LEN;
			break;
		case INST_47:
			item_length = INST_47_LEN;
			break;
		case INST_57:
			item_length = INST_57_LEN;
			break;
		case INST_58:
			item_length = INST_58_LEN;
			break;
		case INST_59:
			item_length = INST_59_LEN;
			break;
		case INST_63:
			item_length = INST_63_LEN;
			break;
		case INST_65:
			item_length = INST_65_LEN;
			break;
		case INST_67:
			item_length = INST_67_LEN;
			break;
		case INST_70:
			item_length = INST_70_LEN;
			break;
		case INST_73:
			item_length = INST_73_LEN;
			break;
		case INST_75:
			item_length = INST_75_LEN;
			break;
		case INST_77:
			item_length = INST_77_LEN;
			break;
		case INST_78:
			item_length = INST_78_LEN;
			break;
		case INST_7D:
			item_length = INST_7D_LEN;
			break;
		case INST_7F:
			item_length = INST_7F_LEN;
			break;
		case INST_81:
			item_length = INST_81_LEN;
			break;
		case INST_85:
			item_length = INST_85_LEN;
			break;
		case INST_86:
			item_length = INST_86_LEN;
			break;
	}

	if (item_length == 0) {
		/* End of list, or unknown item */
		next_inst = NULL;
	} else {
		next_inst = &next_inst[item_length];
	}
	cur_inst = (script_inst_t *) next_inst;

	return cur_inst;
}

static void reindent(int num_indent)
{
	int i;

	memset(indentStr, 0, sizeof(indentStr));
	for (i=0; (i<num_indent) && (i<255); i++) {
		indentStr[i<<1]=' ';
		indentStr[(i<<1)+1]=' ';
	}
}

static void scriptDisasm(room_t *this)
{
	int indent = 0, numFunc = 0;
	script_inst_t *inst;

	reindent(indent);

	logMsg(3, "Disasm script\n");

	logMsg(3, "function Func%d()\n{\n", numFunc++);
	reindent(++indent);

	inst = scriptResetInst(this);
	while (inst) {
		switch(inst->opcode) {
			case INST_NOP:
				if (cur_inst->func.num_func > 0) {		
					logMsg(3, "%snop\n", indentStr);
				} else {
					reindent(--indent);
					logMsg(3, "%s}\n", indentStr);
				}
				break;
			case INST_RETURN:
				if (indent>1) {
					logMsg(3, "%sreturn\n", indentStr);
				}
				reindent(--indent);
				logMsg(3, "%s}\n", indentStr);
				if (indent==0) {
					logMsg(3, "\nfunction Func%d()\n{\n", numFunc++);
					reindent(++indent);
				}
				break;
			case INST_SLEEP_N:
				{
					script_sleepn_t sleepn;
					
					memcpy(&sleepn, inst, sizeof(script_sleepn_t));
					logMsg(3, "%ssleep %d\n", indentStr, SDL_SwapLE16(sleepn.delay));
				}
				break;
			case INST_FUNC:
				{
					logMsg(3, "%sFunc%d()\n", indentStr, inst->func.num_func);
				}
				break;
			case INST_SETOBJFLAG:
				{
					logMsg(3, "%sset flag 0x%02x object 0x%02x %s\n", indentStr,
						inst->setobjflag.flag,
						inst->setobjflag.object,
						(inst->setobjflag.value ? "on" : "off"));
				}
				break;
			case INST_MAKE_DOOR:
				{
					logMsg(3, "%smakeDoor()\n", indentStr);
				}
				break;
			case INST_FLOOR_SET:
				{
					logMsg(3, "%sfloorSet %d %s\n", indentStr,
						inst->floor_set.num_floor,
						(inst->floor_set.value ? "on" : "off")
					);
				}
				break;
			case INST_CUT_REPLACE:
				{
					logMsg(3, "%scutReplace %d %d\n", indentStr,
						inst->cut_replace.before,
						inst->cut_replace.after
					);
				}
				break;
			case INST_IF:
				{
					switch(inst->begin_if.base.cond_func) {
						case INST_IF_CK:
							logMsg(3, "%sif (CK) {\n", indentStr);
							reindent(++indent);
							break;
						case INST_IF_CMP:
							logMsg(3, "%sif (CMP) {\n", indentStr);
							reindent(++indent);
							break;
						default:
							logMsg(3, "%sif (???) {\n", indentStr);
							reindent(++indent);
							break;
					}
				}
				break;
			case INST_ELSE:
				{
					reindent(--indent);
					logMsg(3,"%s} else {\n", indentStr);
					reindent(++indent);
				}
				break;
			case INST_END_IF:
				{
					reindent(--indent);
					logMsg(3,"%s}\n", indentStr);
				}
				break;
			case INST_SWITCH:
				{
					logMsg(3,"%sswitch(object xxx) {\n", indentStr);
					reindent(++indent);
				}
				break;
			case INST_CASE:
				{
					logMsg(3,"%scase xxx:\n", indentStr);
					reindent(++indent);
				}
				break;
			case INST_END_SWITCH:
				{
					reindent(--indent);
					logMsg(3,"%s}\n", indentStr);
				}
				break;
			case INST_BREAK:
				{
					logMsg(3,"%sbreak\n", indentStr);
				}
				break;
			case INST_WHILE:
				{
					logMsg(3,"%swhile (xxx) {\n", indentStr);
					reindent(++indent);
				}
				break;
			default:
				logMsg(3, "Unknown opcode 0x%02x\n", inst->opcode);
				break;
		}

		inst = scriptNextInst(this);
	}
}

/*
BE_FLG = BE_FLG + DISP_OFF ;
42 10 00 00 20 00 00 10
10 00 41 00 10 00

BE_FLG = BE_FLG + SC_AT + OB_AT + DISP_OFF;
42 10 00 00 20 00 00 10
00 20 20 00 00 10
00 40 20 00 00 10
10 00 41 00 10 00
*/
