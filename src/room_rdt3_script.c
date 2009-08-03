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
#define INST_EXEC	0x04
#define INST_IF		0x06
#define INST_IF_CK		0x4c
#define INST_IF_CMP		0x4e
#define INST_ELSE	0x07
#define INST_END_IF	0x08
#define INST_SLEEP_N	0x09
#define INST_SLEEP_W	0x0b
#define INST_FOR	0x0d
#define INST_FOR_END	0x0f
#define INST_WHILE	0x10
#define INST_WHILE_END	0x11
#define INST_DO		0x12
#define INST_DO_END	0x13
#define INST_SWITCH	0x14
#define INST_CASE	0x15
#define INST_END_SWITCH	0x17
#define INST_FUNC	0x19
#define INST_BREAK	0x1b
#define INST_SET0	0x1e
#define INST_SET1	0x1f
#define INST_CALC_ADD	0x20
#define INST_LINE_BEGIN	0x2d
#define INST_LINE_MAIN	0x2e
#define INST_AHEAD_ROOM_SET	0x33
#define INST_FLOOR_SET	0x3f
#define INST_CALC_END	0x41
#define INST_CALC_BEGIN	0x42
#define INST_FADE_SET	0x46
#define INST_SETOBJFLAG	0x4d
#define INST_CUT_REPLACE	0x53
#define INST_MAKE_DOOR	0x61

#define INST_03		0x03
#define INST_03_LEN	2
#define INST_05		0x05
#define INST_05_LEN	2
#define INST_0E		0x0e
#define INST_0E_LEN	2
#define INST_18		0x18
#define INST_18_LEN	8
#define INST_1D		0x1d
#define INST_1D_LEN	4
#define INST_20		0x20
#define INST_20_LEN	6
#define INST_22		0x22
#define INST_22_LEN	2
#define INST_2F		0x2f
#define INST_2F_LEN	2
#define INST_40		0x40
#define INST_40_LEN	4
#define INST_41		0x41
#define INST_41_LEN	4
#define INST_42		0x42
#define INST_42_LEN	4
#define INST_47		0x47
#define INST_47_LEN	4
#define INST_50		0x50
#define INST_50_LEN	2
#define INST_52		0x52
#define INST_52_LEN	2
#define INST_55		0x55
#define INST_55_LEN	16
#define INST_57		0x57
#define INST_57_LEN	6
#define INST_58		0x58
#define INST_58_LEN	6
#define INST_59		0x59
#define INST_59_LEN	8
#define INST_5A		0x5a
#define INST_5A_LEN	2
#define INST_5B		0x5b
#define INST_5B_LEN	6
#define INST_60		0x60
#define INST_60_LEN	2
#define INST_63		0x63
#define INST_63_LEN	0x14
#define INST_64		0x64
#define INST_64_LEN	0x1c
#define INST_65		0x65
#define INST_65_LEN	10
#define INST_67		0x67
#define INST_67_LEN	0x16
#define INST_69		0x69
#define INST_69_LEN	0x0e
#define INST_6A		0x6a
#define INST_6A_LEN	16
#define INST_6E		0x6e
#define INST_6E_LEN	4
#define INST_70		0x70
#define INST_70_LEN	16
#define INST_73		0x73
#define INST_73_LEN	24
#define INST_74		0x74
#define INST_74_LEN	6
#define INST_75		0x75
#define INST_75_LEN	2
#define INST_77		0x77
#define INST_77_LEN	12
#define INST_78		0x78
#define INST_78_LEN	6
#define INST_79		0x79
#define INST_79_LEN	4
#define INST_7B		0x7b
#define INST_7B_LEN	6
#define INST_7D		0x7d
#define INST_7D_LEN	24
#define INST_7F		0x7f
#define INST_7F_LEN	40
#define INST_80		0x80
#define INST_80_LEN	4
#define INST_81		0x81
#define INST_81_LEN	8
#define INST_82		0x82
#define INST_82_LEN	10
#define INST_83		0x83
#define INST_83_LEN	2
#define INST_84		0x84
#define INST_84_LEN	2
#define INST_85		0x85
#define INST_85_LEN	8
#define INST_86		0x86
#define INST_86_LEN	(16*8+10)
#define INST_87		0x87
#define INST_87_LEN	2
#define INST_88		0x88
#define INST_88_LEN	4
#define INST_89		0x89
#define INST_89_LEN	2

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
	Uint8 unknown[31];
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

typedef struct {
	Uint8 opcode;
	Uint8 unknown0[9];
	Uint16 unknown1;
} script_fade_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 mask;
	Uint8 func[2];
} script_exec_t;

typedef struct {
	Uint8 opcode;
	Uint8 variable;
	Uint16 value;
} script_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 value;
} script_ahead_room_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 flag;
	Uint8 size;
	Uint8 dummy;
} script_line_begin_t;

typedef struct {
	Uint8 opcode;
	Uint8 flag;
	Uint16 p1, p2;
} script_line_main_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 cond_func;	/* = INST_IF_CK */
	Uint8 value_flag;
	Uint8 value_obj;
	Uint8 value_comp;
} script_do_end_ck_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 cond_func;	/* = INST_IF_CMP */
	Uint8 value_flag;
	Uint8 value_obj;
	Uint8 value_comp;
	Uint16 value2_comp;
} script_do_end_cmp_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 cond_func;
} script_do_end_base_t;

typedef union {
	script_do_end_base_t	base;
	script_do_end_ck_t	check;
	script_do_end_cmp_t	compare;
} script_do_end_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown;
	Uint16 block_length;
	Uint16 count;
} script_for_t;

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
	script_fade_set_t	fade_set;
	script_exec_t		exec;
	script_set_t		set;
	script_ahead_room_set_t	ahead_room_set;
	script_line_begin_t	line_begin;
	script_line_main_t	line_main;
	script_do_end_t		do_end;
	script_for_t		do_for;
} script_inst_t;

/*--- Variables ---*/

static script_inst_t *cur_inst;
static int cur_inst_offset;
static int script_length;

static int num_block_len;
static Uint8 stack_block_len[256];

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
	Uint32 *item_offset, offset, next_offset;

	if (!this) {
		return NULL;
	}
	
	item_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+16*4]);
	offset = SDL_SwapLE32(*item_offset);
	cur_inst = (script_inst_t *) &((Uint8 *) this->file)[offset];

	cur_inst_offset = 0;

	num_block_len = 0;
	memset(stack_block_len, 0, sizeof(stack_block_len));

	/* length using next item in rdt file */
	item_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+13*4]);
	next_offset = SDL_SwapLE32(*item_offset);
	script_length = next_offset - offset;

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
			if (cur_inst->func.num_func > 0) {
				item_length = 4;
			} else {
				item_length = 2;
			}
			break;
		case INST_END_SWITCH:
		case INST_BREAK:
		case INST_RETURN:
		case INST_SLEEP_1:
		case INST_WHILE_END:
		case INST_SLEEP_W:
		case INST_03:
		case INST_05:
		case INST_0E:
		case INST_FOR_END:
		case INST_22:
		case INST_2F:
		case INST_5A:
		case INST_83:
		case INST_84:
		case INST_87:
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
				if (cur_inst->i_while.base.unknown0 & 0x80) {
					item_length = 4;
					break;
				}
				if (cur_inst->i_while.base.unknown0 == 0) {
					item_length = 2;
					break;
				}

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
		case INST_DO_END:
			{
				switch(cur_inst->do_end.base.cond_func) {
					case INST_IF_CK:
						item_length = sizeof(script_do_end_ck_t);
						break;
					case INST_IF_CMP:
						item_length = sizeof(script_do_end_cmp_t);
						break;
				}
			}
			break;
		case INST_EXEC:
			item_length = sizeof(script_exec_t);
			break;
		case INST_FOR:
			item_length = sizeof(script_for_t);
			break;
		case INST_18:
			item_length = INST_18_LEN;
			break;
		case INST_1D:
			item_length = INST_1D_LEN;
			break;
		case INST_SET0:
		case INST_SET1:
		case INST_DO:
			item_length = 4;
			break;
		case INST_20:
			item_length = INST_20_LEN;
			break;
		case INST_AHEAD_ROOM_SET:
			item_length = sizeof(script_ahead_room_set_t);
			break;
		case INST_LINE_BEGIN:
			item_length = sizeof(script_line_begin_t);
			break;
		case INST_LINE_MAIN:
			item_length = sizeof(script_line_main_t);
			break;
		case INST_FADE_SET:
			item_length = sizeof(script_fade_set_t);
			break;
		case INST_40:
			item_length = INST_40_LEN;
			break;
		case INST_41:
			item_length = INST_41_LEN;
			break;
		case INST_42:
			item_length = INST_42_LEN;
			break;
		case INST_47:
			item_length = INST_47_LEN;
			break;
		case INST_50:
			item_length = INST_50_LEN;
			break;
		case INST_52:
			item_length = INST_52_LEN;
			break;
		case INST_55:
			item_length = INST_55_LEN;
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
		case INST_5B:
			item_length = INST_5B_LEN;
			break;
		case INST_60:
			item_length = INST_60_LEN;
			break;
		case INST_63:
			item_length = INST_63_LEN;
			break;
		case INST_64:
			item_length = INST_64_LEN;
			break;
		case INST_65:
			item_length = INST_65_LEN;
			break;
		case INST_67:
			item_length = INST_67_LEN;
			break;
		case INST_69:
			item_length = INST_69_LEN;
			break;
		case INST_6A:
			item_length = INST_6A_LEN;
			break;
		case INST_6E:
			item_length = INST_6E_LEN;
			break;
		case INST_70:
			item_length = INST_70_LEN;
			break;
		case INST_73:
			item_length = INST_73_LEN;
			break;
		case INST_74:
			item_length = INST_74_LEN;
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
		case INST_79:
			item_length = INST_79_LEN;
			break;
		case INST_7B:
			item_length = INST_7B_LEN;
			break;
		case INST_7D:
			item_length = INST_7D_LEN;
			break;
		case INST_7F:
			item_length = INST_7F_LEN;
			break;
		case INST_80:
			item_length = INST_80_LEN;
			break;
		case INST_81:
			item_length = INST_81_LEN;
			break;
		case INST_82:
			item_length = INST_82_LEN;
			break;
		case INST_85:
			item_length = INST_85_LEN;
			break;
		case INST_86:
			item_length = INST_86_LEN;
			break;
		case INST_88:
			item_length = INST_88_LEN;
			break;
		case INST_89:
			item_length = INST_89_LEN;
			break;
	}

	if (item_length == 0) {
		/* Unknown opcode */
		next_inst = NULL;
	} else if (cur_inst_offset >= script_length) {
		/* End of script */
		next_inst = NULL;
		logMsg(3, "End of script\n");
	} else {
		next_inst = &next_inst[item_length];
		cur_inst_offset += item_length;
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
					logMsg(3, "Unknown opcode 0x%04x, offset 0x%08x\n",
						(inst->func.num_func<<8)|inst->opcode,
						cur_inst_offset);
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
			case INST_SLEEP_1:
				{
					logMsg(3, "%ssleep 1\n", indentStr);
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
							logMsg(3, "%sif (CK flag 0x%02x object 0x%02x %s) {\n",
								indentStr,
								inst->begin_if.check.value_flag,
								inst->begin_if.check.value_obj,
								(inst->begin_if.check.value_comp ? "on" : "off")
							);
							reindent(++indent);
							break;
						case INST_IF_CMP:
							logMsg(3, "%sif (CMP) {\n", indentStr);
							reindent(++indent);
							break;
						default:
							logMsg(3, "%sif (xxx) {\n", indentStr);
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
					indent += 2;
					reindent(indent);
				}
				break;
			case INST_CASE:
				{
					reindent(--indent);
					logMsg(3,"%scase xxx:\n", indentStr);
					reindent(++indent);
				}
				break;
			case INST_END_SWITCH:
				{
					indent -= 2;
					reindent(indent);
					logMsg(3,"%s}\n", indentStr);
				}
				break;
			case INST_BREAK:
				{
					logMsg(3,"%sbreak\n", indentStr);
					/*reindent(--indent);*/
				}
				break;
			case INST_WHILE:
				{
					logMsg(3,"%swhile (xxx) {\n", indentStr);
					reindent(++indent);
				}
				break;
			case INST_WHILE_END:
				{
					reindent(--indent);
					logMsg(3,"%s}\n", indentStr);
				}
				break;
			case INST_FADE_SET:
				{
					logMsg(3, "%sfadeSet xxx\n", indentStr);
				}
				break;
			case INST_EXEC:
				{
					logMsg(3, "%seventExec flag 0x%02x Func%d()\n",
						indentStr,
						inst->exec.mask,
						inst->exec.func[1]
					);
				}
				break;
			case INST_SET0:
			case INST_SET1:
				{
					script_set_t new_set;

					memcpy(&new_set, inst, sizeof(script_set_t));

					logMsg(3, "%sset variable 0x%02x = %d\n",
						indentStr,
						new_set.variable,
						SDL_SwapLE16(new_set.value)
					);
				}
				break;
			case INST_AHEAD_ROOM_SET:
				{
					script_ahead_room_set_t ahead_room_set;

					memcpy(&ahead_room_set, inst, sizeof(script_ahead_room_set_t));

					logMsg(3, "%saheadRoomSet 0x%04x\n",
						indentStr,
						SDL_SwapLE16(ahead_room_set.value)
					);
				}
				break;
			case INST_DO:
				{
					logMsg(3, "%sdo {\n", indentStr);
					reindent(++indent);
				}
				break;
			case INST_DO_END:
				{
					reindent(--indent);
					switch(inst->do_end.base.cond_func) {
						case INST_IF_CK:
							logMsg(3, "%s} while (CK flag 0x%02x object 0x%02x %s) {\n",
								indentStr,
								inst->do_end.check.value_flag,
								inst->do_end.check.value_obj,
								(inst->do_end.check.value_comp ? "on" : "off")
							);
							reindent(++indent);
							break;
						case INST_IF_CMP:
							logMsg(3, "%s} while (CMP)\n", indentStr);
							break;
						default:
							logMsg(3, "%s} while (xxx)\n", indentStr);
							break;
					}
				}
				break;
			case INST_LINE_BEGIN:
				{
					logMsg(3, "%slineStart 0x%02x %d\n", indentStr,
						inst->line_begin.flag,
						inst->line_begin.size);
				}
				break;
			case INST_LINE_MAIN:
				{
					logMsg(3, "%slineMain\n", indentStr);
				}
				break;
			case INST_FOR:
				{
					script_for_t	do_for;
					
					memcpy(&do_for, inst, sizeof(script_for_t));
					logMsg(3, "%sfor (%d) {\n", indentStr, SDL_SwapLE16(do_for.count));
					reindent(++indent);
				}
				break;
			case INST_FOR_END:
				{
					reindent(--indent);
					logMsg(3, "%s}\n", indentStr);
				}
				break;
			default:
				logMsg(3, "Unknown opcode 0x%02x, offset 0x%08x\n", inst->opcode, cur_inst_offset);
				break;
		}

		inst = scriptNextInst(this);
	}
}

