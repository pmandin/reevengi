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
#include "room_rdt2.h"

/*--- Defines ---*/

#define INST_NOP	0x00
#define INST_RETURN	0x01
#define INST_SLEEP_1	0x02
#define INST_EXEC	0x04
#define INST_IF		0x06
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
#define INST_SWITCH_END	0x17
#define INST_GOTO	0x18
#define INST_FUNC	0x19
#define INST_BREAK	0x1b
#define INST_VALUE_SET	0x1e
#define INST_SET1	0x1f

#define INST_CALC_ADD	0x20
#define INST_EVT_CUT	0x22
#define INST_LINE_BEGIN	0x2d
#define INST_LINE_MAIN	0x2e
#define INST_LINE_END	0x2f

#define INST_AHEAD_ROOM_SET	0x33
#define INST_FLOOR_SET	0x3f

#define INST_CALC_END	0x41
#define INST_CALC_BEGIN	0x42
#define INST_FADE_SET	0x46
#define INST_WORK_SET	0x47
#define INST_FLAG_SET	0x4d

#define INST_CUT_CHG	0x50
#define INST_CUT_AUTO	0x52
#define INST_CUT_REPLACE	0x53
#define INST_POS_SET	0x55
#define INST_DIR_SET	0x56
#define INST_RBJ_SET	0x5a
#define INST_MESSAGE_ON	0x5b

#define INST_DOOR_SET	0x61
#define INST_AOT_SET	0x63
#define INST_AOT_SET_4P	0x64
#define INST_AOT_RESET	0x65
#define INST_ITEM_AOT_SET	0x67
#define INST_KAGE_SET	0x69
#define INST_SUPER_SET	0x6a
#define INST_SCA_ID_SET	0x6e

#define INST_ESPR_ON	0x70
#define INST_ESPR3D_ON2	0x73
#define INST_ESPR_KILL	0x74
#define INST_ESPR_KILL2	0x75
#define INST_SE_ON	0x77
#define INST_BGM_CTL	0x78
#define INST_XA_ON	0x79
#define INST_BGM_TBL_SET	0x7b
#define INST_EM_SET	0x7d
#define INST_OM_SET	0x7f

#define INST_PLC_MOTION	0x80
#define INST_PLC_DEST	0x81
#define INST_PLC_NECK	0x82
#define INST_PLC_RET	0x83
#define INST_PLC_FLG	0x84
#define INST_PLC_STOP	0x87
#define INST_PLC_ROT	0x88
#define INST_PLC_CNT	0x89

#define INST_END_SCRIPT	0xff

#define CONDITION_CC		0x1d
#define CONDITION_CK		0x4c
#define CONDITION_CMP		0x4e

/*--- Types ---*/

typedef struct {
	Uint8 type;
	Uint8 flag;
} script_condition_27_t;

typedef struct {
	Uint8 type;
	Uint8 flag;
	Uint8 object;
	Uint8 value;
} script_condition_ck_t;

typedef struct {
	Uint8 type;
	Uint8 flag;
	Uint8 object;
	Uint8 value;
	Uint16 value2;
} script_condition_cmp_t;

typedef struct {
	Uint8 type;
	Uint8 flag;
	Uint8 object;
	Uint8 value;
	Uint8 unknown[6];
} script_condition_cc_t;

typedef union {
	Uint8 type;
	script_condition_ck_t	ck;
	script_condition_cmp_t	cmp;
	script_condition_cc_t	cc;
} script_condition_t;

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
} script_set_flag_t;

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
	Uint16 block_length;
} script_if_t;	/* always followed by script_condition_t */

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 block_length;
} script_else_t;

typedef struct {
	Uint8 opcode;
	Uint8 object;
	Uint16 block_length;
} script_switch_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 block_length;
	Uint16 value;
} script_case_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint16 block_length;
} script_while_t;	/* always followed by script_condition_t */

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
	Uint8 unknown;
} script_do_end_t;	/* always followed by script_condition_t */

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
	script_set_flag_t set_flag;
	script_make_door_t	make_door;
	script_floor_set_t	floor_set;
	script_cut_replace_t	cut_replace;
	script_if_t	begin_if;
	script_else_t	else_if;
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

typedef struct {
	Uint8 opcode;
	Uint8 length;
} script_inst_len_t;

/*--- Variables ---*/

static const script_inst_len_t inst_length[]={
	/* 0x00-0x0f */
	{INST_NOP,	1},
	{INST_RETURN,	2},
	{INST_SLEEP_1,	1},
	{0x03,		2},
	{INST_EXEC,	sizeof(script_exec_t)},
	{0x05,		2},
	/*{INST_IF,	sizeof(script_if_t)},*/
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_END_IF,	2},
	{INST_SLEEP_N,	sizeof(script_sleepn_t)},
	{0x0a,		2},	/* maybe */
	{INST_SLEEP_W,	2},
	{0x0c,		2},	/* maybe */
	{INST_FOR,	sizeof(script_for_t)},
	{INST_FOR_END,	2},

	/* 0x10-0x1f */
	/*{INST_WHILE,	sizeof(script_while_t)},*/
	{INST_WHILE_END,	2},
	{INST_DO,	4},
	/*{INST_DO_END,	sizeof(script_do_end_t)},*/
	{INST_SWITCH,	sizeof(script_switch_t)},
	{INST_CASE,	sizeof(script_case_t)},
	{0x16,		2},	/* maybe */
	{INST_SWITCH_END,	2},
	{0x18,		6},
	{INST_FUNC,	2},
	{0x1a,		4},
	{INST_BREAK,	2},	
	{0x1c,		2},	/* maybe */
	{0x1d,		4},
	{INST_VALUE_SET,	4},
	{INST_SET1,	4},

	/* 0x20-0x2f */
	{INST_CALC_ADD,	6},
	{0x21,		4}, /* maybe */
	{INST_EVT_CUT,	4},
	{0x24,		2}, /* maybe */
	{0x25,		5}, /* maybe */
	{0x27,		4}, /* maybe */
	{0x28,		14}, /* maybe */
	{0x2a,		6}, /* maybe */
	{0x2c,		4}, /* maybe */
	{INST_LINE_BEGIN,	sizeof(script_line_begin_t)},
	{INST_LINE_MAIN,	sizeof(script_line_main_t)},
	{INST_LINE_END,		2},

	/* 0x30-0x3f */
	{0x30,		6},	/* maybe */
	{0x32,		6},	/* maybe */
	{INST_AHEAD_ROOM_SET,	sizeof(script_ahead_room_set_t)},
	{0x34,		10},	/* maybe */
	{0x37,		2},	/* maybe */
	{0x38,		4},	/* maybe */
	{0x3b,		4},	/* maybe */
	{0x3d,		2},
	{0x3e,		6},	/* maybe */
	{INST_FLOOR_SET,	sizeof(script_floor_set_t)},

	/* 0x40-0x4f */
	{0x40,		4},
	{INST_CALC_END,	4},
	{INST_CALC_BEGIN,	4},
	{0x43,		6},	/* maybe */
	{INST_FADE_SET,	sizeof(script_fade_set_t)},
	{INST_WORK_SET,	3},
	{0x48,		4},	/* maybe */
	{0x49,		4},	/* maybe */
	{0x4a,		2},	/* maybe */
	{0x4b,		2},	/* maybe */
	{0x4c,		4},	/* maybe */
	{INST_FLAG_SET,	sizeof(script_set_flag_t)},
	{0x4e,		6},	/* maybe */
	{0x4f,		2},	/* maybe */

	/* 0x50-0x5f */
	{INST_CUT_CHG,	2},
	{0x51,		4},
	{INST_CUT_AUTO,		2},
	{INST_CUT_REPLACE,	sizeof(script_cut_replace_t)},
	{0x54,		4 /*12*/},	/* maybe */
	{INST_POS_SET,		8},
	{INST_DIR_SET,		8},
	{0x57,		6},
	{0x58,		6},
	{0x59,		8},
	{INST_RBJ_SET,		2},
	{INST_MESSAGE_ON,	6},
	{0x5c,		2},	/* maybe */
	{0x5d,		2},	/* maybe */
	{0x5e,		4},	/* maybe */
	{0x5f,		2},	/* maybe */

	/* 0x60-0x6f */
	{0x60,			2},
	{INST_DOOR_SET,	sizeof(script_make_door_t)},
	{0x62,			40},	/* maybe */
	{INST_AOT_SET,		20},
	{INST_AOT_SET_4P,	28},
	{INST_AOT_RESET,	10},
	{0x66,			2 /*6*/}, /* maybe */
	{INST_ITEM_AOT_SET,	22},
	{0x68,			30}, /* maybe */
	{INST_KAGE_SET,	14},
	{INST_SUPER_SET,	16},
	{0x6c,			2}, /* maybe */
	{INST_SCA_ID_SET,	4},
	{0x6f,			2}, /* maybe */

	/* 0x70-0x7f */
	{INST_ESPR_ON,	16},
	{0x71, 18},	/* maybe */
	{0x72, 22},	/* maybe */
	{INST_ESPR3D_ON2,	24},
	{INST_ESPR_KILL,	5 /*6*/ /*8*/},
	{INST_ESPR_KILL2,	2},
	{0x76, 		3},	/* maybe */
	{INST_SE_ON,	12},
	{INST_BGM_CTL,	6},
	{INST_XA_ON,	4},
	{0x7a, 		2},	/* maybe */
	{INST_BGM_TBL_SET,	6},
	{0x7c,			2}, /*maybe */
	{INST_EM_SET,	24},
	{0x7e,			2}, /*maybe */
	{INST_OM_SET,	40},

	/* 0x80-0x8f */
	{INST_PLC_MOTION,	4},
	{INST_PLC_DEST,		8},
	{INST_PLC_NECK,		10},
	{INST_PLC_RET,		1},
	{INST_PLC_FLG,		4},
	{INST_PLC_STOP,	1},
	{INST_PLC_ROT,	4},
	{INST_PLC_CNT,	2},
	{0x8a,		2},	/* maybe */
	{0x8b,		1},	/* maybe */
	{0x8c,		1},	/* maybe */
	{0x8e,		2},	/* maybe */
	{0x8f,		2}	/* maybe */
};

static char indentStr[256];

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this);
static int scriptGetInstLen(room_t *this);
static void scriptPrintInst(room_t *this);

static int scriptGetConditionLen(script_condition_t *conditionPtr);

static void reindent(int num_indent);

/*--- Functions ---*/

void room_rdt3_scriptInit(room_t *this)
{
	int i;
	rdt2_header_t *rdt_header = (rdt2_header_t *) this->file;
	Uint32 offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_INIT_SCRIPT]);
	Uint32 smaller_offset = this->file_length;

	/* Search smaller offset after script to calc length */
	for (i=0; i<21; i++) {
		Uint32 next_offset = SDL_SwapLE32(rdt_header->offsets[i]);
		if ((next_offset>0) && (next_offset<smaller_offset) && (next_offset>offset)) {
			smaller_offset = next_offset;
		}
	}

	if (smaller_offset>offset) {
		this->script_length = smaller_offset - offset;
	}

	logMsg(3, "rdt3: Init script at offset 0x%08x, length 0x%04x\n", offset, this->script_length);

	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivPrintInst = scriptPrintInst;
}

static Uint8 *scriptFirstInst(room_t *this)
{
	rdt2_header_t *rdt_header;
	Uint32 offset;
	Uint16 *functionArrayPtr;

	if (!this) {
		return NULL;
	}
	if (this->script_length == 0) {
		return NULL;
	}

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_INIT_SCRIPT]);

	/* Start of script is an array of offsets to the various script functions
	 * The first offset also gives the first instruction to execute
	 */
	functionArrayPtr = (Uint16 *) (& ((Uint8 *) this->file)[offset]);

	this->cur_inst_offset = SDL_SwapLE16(functionArrayPtr[0]);
	this->cur_inst = (& ((Uint8 *) this->file)[offset + this->cur_inst_offset]);
	return this->cur_inst;
}

static int scriptGetConditionLen(script_condition_t *conditionPtr)
{
	int inst_len = 0;

	switch(conditionPtr->type) {
		case 0x0e:
		case 0x27:
		case 0x6b:
			inst_len = sizeof(script_condition_27_t);
			break;
		case CONDITION_CK:
		case 0x6c:
			inst_len = sizeof(script_condition_ck_t);
			break;
		case 0x35:
		case 0x36:
		case 0x43:
		case CONDITION_CMP:
			inst_len = sizeof(script_condition_cmp_t);
			break;
		case CONDITION_CC:
			inst_len = sizeof(script_condition_cc_t);
			break;
		default:
			logMsg(3, "Unknown condition type 0x%02x\n", conditionPtr->type);
			break;
	}

	return inst_len;
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
		script_inst_t *cur_inst = (script_inst_t *) this->cur_inst;
		switch(cur_inst->opcode) {
			case INST_IF:
				inst_len = sizeof(script_if_t) +
					scriptGetConditionLen(
						(script_condition_t *) (&this->cur_inst[sizeof(script_if_t)])
					);
				break;
			case INST_WHILE:
				inst_len = sizeof(script_while_t) +
					scriptGetConditionLen(
						(script_condition_t *) (&this->cur_inst[sizeof(script_while_t)])
					);
				break;
			case INST_DO_END:
				inst_len = sizeof(script_do_end_t) +
					scriptGetConditionLen(
						(script_condition_t *) (&this->cur_inst[sizeof(script_do_end_t)])
					);
				break;
			default:
				break;
		}
	}

	return inst_len;
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

static void scriptPrintInst(room_t *this)
{
	int indent = 0, numFunc = 0;
	script_inst_t *inst;

	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	reindent(indent);

	inst = (script_inst_t *) this->cur_inst;

	switch(inst->opcode) {
		case INST_NOP:
			logMsg(3, "%snop\n", indentStr);
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
			logMsg(3, "%ssleep 1\n", indentStr);
			break;
		case INST_SLEEP_N:
			{
				script_sleepn_t sleepn;
	
				memcpy(&sleepn, inst, sizeof(script_sleepn_t));
				logMsg(3, "%ssleep %d\n", indentStr, SDL_SwapLE16(sleepn.delay));
			}
			break;
		case INST_FUNC:
			logMsg(3, "%sFunc%02x()\n", indentStr, inst->func.num_func);
			break;
		case INST_FLAG_SET:
			logMsg(3, "%sFLAG_SET 0x%02x object 0x%02x %s\n", indentStr,
				inst->set_flag.flag,
				inst->set_flag.object,
				(inst->set_flag.value ? "on" : "off"));
			break;
		case INST_DOOR_SET:
			logMsg(3, "%sDOOR_SET xxx\n", indentStr);
			break;
		case INST_FLOOR_SET:
			logMsg(3, "%sFLR_SET %d %s\n", indentStr,
				inst->floor_set.num_floor,
				(inst->floor_set.value ? "on" : "off")
			);
			break;
		case INST_CUT_REPLACE:
			logMsg(3, "%sCUT_REPLACE 0x%02x 0x%02x\n", indentStr,
				inst->cut_replace.before,
				inst->cut_replace.after
			);
			break;
		case INST_IF:
			{
				logMsg(3, "%sif (xxx) {\n", indentStr);
				reindent(++indent);
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
		case INST_SWITCH_END:
			{
				indent -= 2;
				reindent(indent);
				logMsg(3,"%s}\n", indentStr);
			}
			break;
		case INST_BREAK:
			logMsg(3,"%sbreak\n", indentStr);
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
			logMsg(3, "%sFADE_SET xxx\n", indentStr);
			break;
		case INST_EXEC:
			logMsg(3, "%sEVT_EXEC flag 0x%02x Func%d()\n",
				indentStr,
				inst->exec.mask,
				inst->exec.func[1]
			);
			break;
		case INST_VALUE_SET:
		case INST_SET1:
			{
				script_set_t new_set;

				memcpy(&new_set, inst, sizeof(script_set_t));

				logMsg(3, "%svar%02x = 0x%04x\n",
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

				logMsg(3, "%sAHEAD_ROOM_SET 0x%04x\n",
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
				logMsg(3, "%s} while (xxx)\n", indentStr);
			}
			break;
		case INST_LINE_BEGIN:
			logMsg(3, "%sLINE_START 0x%02x %d\n", indentStr,
				inst->line_begin.flag,
				inst->line_begin.size
			);
			break;
		case INST_LINE_MAIN:
			logMsg(3, "%sLINE_MAIN\n", indentStr);
			break;
		case INST_LINE_END:
			logMsg(3, "%sLINE_END\n", indentStr);
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
		case INST_WORK_SET:
			logMsg(3, "%sWORK_SET xxx\n", indentStr);
			break;
		case INST_AOT_SET:
			logMsg(3, "%sAOT_SET xxx\n", indentStr);
			break;
		case INST_ESPR3D_ON2:
			logMsg(3, "%sESPR3D_ON2 xxx\n", indentStr);
			break;
		case INST_ESPR_KILL2:
			logMsg(3, "%sESPR_KILL2 %d\n", indentStr, this->cur_inst[1]);
			break;
		case INST_EM_SET:
			logMsg(3, "%sEM_SET xxx\n", indentStr);
			break;
		case INST_OM_SET:
			logMsg(3, "%sOM_SET xxx\n", indentStr);
			break;
		case INST_SUPER_SET:
			logMsg(3, "%sSUPER_SET xxx\n", indentStr);
			break;
		case INST_BGM_CTL:
			logMsg(3, "%sBGM_CTL xxx\n", indentStr);
			break;
		case INST_MESSAGE_ON:
			logMsg(3, "%sMESSAGE_ON xxx\n", indentStr);
			break;
		case INST_ESPR_KILL:
			logMsg(3, "%sESPR_KILL xxx\n", indentStr);
			break;
		case INST_CUT_CHG:
			logMsg(3, "%sCUT_CHG xxx\n", indentStr);
			break;
		case INST_PLC_DEST:
			logMsg(3, "%sPLC_DEST xxx\n", indentStr);
			break;
		case INST_PLC_MOTION:
			logMsg(3, "%sPLC_MOTION xxx\n", indentStr);
			break;
		case INST_PLC_CNT:
			logMsg(3, "%sPLC_CNT 0x%02x\n", indentStr, this->cur_inst[1]);
			break;
		case INST_POS_SET:
			logMsg(3, "%sPOS_SET xxx\n", indentStr);
			break;
		case INST_DIR_SET:
			logMsg(3, "%sDIR_SET xxx\n", indentStr);
			break;
		case INST_CUT_AUTO:
			logMsg(3, "%sCUT_AUTO xxx\n", indentStr);
			break;
		case INST_ESPR_ON:
			logMsg(3, "%sESPR_ON xxx\n", indentStr);
			break;
		case INST_XA_ON:
			logMsg(3, "%sXA_ON xxx\n", indentStr);
			break;
		case INST_SLEEP_W:
			logMsg(3, "%sSLEEPW xxx\n", indentStr);
			break;
		case INST_ITEM_AOT_SET:
			logMsg(3, "%sITEM_AOT_SET xxx\n", indentStr);
			break;
		case INST_GOTO:
			logMsg(3, "%sGOTO xxx\n", indentStr);
			break;
		case INST_RBJ_SET:
			logMsg(3, "%sRBJ_SET xxx\n", indentStr);
			break;
		case INST_AOT_SET_4P:
			logMsg(3, "%sAOT_SET_4P xxx\n", indentStr);
			break;
		case INST_SCA_ID_SET:
			logMsg(3, "%sSCA_ID_SET xxx\n", indentStr);
			break;
		case INST_KAGE_SET:
			logMsg(3, "%sKAGE_SET xxx\n", indentStr);
			break;
		case INST_PLC_NECK:
			logMsg(3, "%sPLC_NECK xxx\n", indentStr);
			break;
		case INST_EVT_CUT:
			logMsg(3, "%sEVT_CUT xxx\n", indentStr);
			break;
		case INST_PLC_STOP:
			logMsg(3, "%sPLC_STOP xxx\n", indentStr);
			break;
		/*default:
			logMsg(3, "Unknown opcode 0x%02x offset 0x%08x\n", inst->opcode, this->cur_inst_offset);
			break;*/

	}
}
