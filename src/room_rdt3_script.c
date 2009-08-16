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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "room.h"
#include "room_rdt2.h"

/*--- Defines ---*/

#define INST_NOP	0x00
#define INST_RETURN	0x01
#define INST_SLEEP_1	0x02
#define INST_EVT_EXEC	0x04
#define INST_EVT_KILL	0x05
#define INST_IF		0x06
#define INST_ELSE	0x07
#define INST_END_IF	0x08
#define INST_SLEEP_N	0x0a
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
#define INST_EVAL_CC	0x1d
#define INST_VALUE_SET	0x1e
#define INST_SET1	0x1f

#define INST_CALC_OP	0x20
#define INST_EVT_CUT	0x22
#define INST_LINE_BEGIN	0x2d
#define INST_LINE_MAIN	0x2e
#define INST_LINE_END	0x2f

#define INST_LIGHT_COLOR_SET	0x32
#define INST_AHEAD_ROOM_SET	0x33
#define INST_EVAL_BGM_TBL_CK	0x35
#define INST_CHASER_ITEM_SET	0x3b
#define INST_FLOOR_SET	0x3f

#define INST_VAR_SET	0x40
#define INST_CALC_STORE	0x41
#define INST_CALC_LOAD	0x42
#define INST_FADE_SET	0x46
#define INST_WORK_SET	0x47
#define INST_EVAL_CK	0x4c
#define INST_FLAG_SET	0x4d
#define INST_EVAL_CMP	0x4e

#define INST_CUT_CHG	0x50
#define INST_CUT_AUTO	0x52
#define INST_CUT_REPLACE	0x53
#define INST_POS_SET	0x55
#define INST_DIR_SET	0x56
#define INST_SET_VIB0	0x57
#define INST_SET_VIB_FADE	0x59
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
	Uint8 delay;
	Uint8 unknown;
} script_sleepn_t;

typedef struct {
	Uint8 opcode;
	Uint8 flag;
	Uint8 object;
	Uint8 value;
} script_set_flag_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 id;
	Uint8 unknown1[3];
	Sint16 x,y,w,h;
	Sint16 next_x,next_y,next_z;
	Sint16 next_dir;
	Uint8 next_stage,next_room,next_camera;
	Uint8 unknown2;
	Uint8 door_type;
	Uint8 door_lock;
	Uint8 unknown3;
	Uint8 door_locked;
	Uint8 door_key;
	Uint8 unknown4;
} script_door_set_t;

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
	Uint8 unknown0[10];
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
	Uint8 num_var;
	Sint16 value;
} script_var_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown;
	Uint16 block_length;
	Uint16 count;
} script_for_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown;
	Uint8 object;
} script_work_set_t;

typedef union {
	Uint8 opcode;
	script_if_t	i_if;
	script_else_t	i_else;
	script_switch_t		i_switch;
	script_case_t		i_case;
	script_while_t		i_while;
	script_do_end_t		i_end_do;
	script_for_t		i_for;
	script_func_t func;
	script_sleepn_t sleepn;
	script_set_flag_t set_flag;
	script_door_set_t	door_set;
	script_floor_set_t	floor_set;
	script_cut_replace_t	cut_replace;
	script_fade_set_t	fade_set;
	script_exec_t		exec;
	script_set_t		set;
	script_ahead_room_set_t	ahead_room_set;
	script_line_begin_t	line_begin;
	script_line_main_t	line_main;
	script_work_set_t	work_set;
	script_var_set_t	var_set;
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
	{INST_EVT_EXEC,	sizeof(script_exec_t)},
	{INST_EVT_KILL,	2},
	{INST_IF,	sizeof(script_if_t)},
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_END_IF,	2},
	{0x09,		1},
	{INST_SLEEP_N,	sizeof(script_sleepn_t)},
	{INST_SLEEP_W,	1},
	{0x0c,		1},
	{INST_FOR,	sizeof(script_for_t)},
	{0x0e,		5},
	{INST_FOR_END,	2},

	/* 0x10-0x1f */
	{INST_WHILE,	sizeof(script_while_t)},
	{INST_WHILE_END,	2},
	{INST_DO,	4},
	{INST_DO_END,	sizeof(script_do_end_t)},
	{INST_SWITCH,	sizeof(script_switch_t)},
	{INST_CASE,	sizeof(script_case_t)},
	{0x16,		2},
	{INST_SWITCH_END,	2},
	{0x18,		6},
	{INST_FUNC,	2},
	{0x1a,		4},
	{INST_BREAK,	2},	
	{0x1c,		1},
	{INST_EVAL_CC,	4},
	{INST_VALUE_SET,	4},
	{INST_SET1,	4},

	/* 0x20-0x2f */
	{INST_CALC_OP,	6},
	{0x21,		4},
	{INST_EVT_CUT,	4},
	{0x23,		1},
	{0x24,		2},
	{0x25,		4},
	{0x26,		6},
	{0x27,		1},
	{0x28,		1},
	{0x29,		8},
	{0x2a,		6},
	{0x2c,		2},
	{INST_LINE_BEGIN,	sizeof(script_line_begin_t)},
	{INST_LINE_MAIN,	sizeof(script_line_main_t)},
	{INST_LINE_END,		2},

	/* 0x30-0x3f */
	{0x30,		6},
	{0x31,		6},
	{INST_LIGHT_COLOR_SET,		6},
	{INST_AHEAD_ROOM_SET,	sizeof(script_ahead_room_set_t)},
	{0x34,		10},
	{INST_EVAL_BGM_TBL_CK,		6},
	{0x36,		3},
	{0x37,		2},
	{0x38,		2},
	{0x39,		16},
	{0x3a,		16},
	{INST_CHASER_ITEM_SET,		3},
	{0x3c,		1},
	{0x3d,		2},
	{0x3e,		2},
	{INST_FLOOR_SET,	sizeof(script_floor_set_t)},

	/* 0x40-0x4f */
	{INST_VAR_SET,		4},
	{INST_CALC_STORE,	3},
	{INST_CALC_LOAD,	3},
	{0x43,		6},
	{0x44,		6},
	{0x45,		4},
	{INST_FADE_SET,	sizeof(script_fade_set_t)},
	{INST_WORK_SET,	sizeof(script_work_set_t)},
	{0x48,		4},
	{0x49,		1},
	{0x4a,		1},
	{0x4b,		1},
	{INST_EVAL_CK,	4},
	{INST_FLAG_SET,	sizeof(script_set_flag_t)},
	{INST_EVAL_CMP,	6},
	{0x4f,		1},

	/* 0x50-0x5f */
	{INST_CUT_CHG,	2},
	{0x51,		1},
	{INST_CUT_AUTO,		2},
	{INST_CUT_REPLACE,	sizeof(script_cut_replace_t)},
	{0x54,		4},
	{INST_POS_SET,		8},
	{INST_DIR_SET,		8},
	{INST_SET_VIB0,		6},
	{0x58,		6},
	{INST_SET_VIB_FADE,	8},
	{INST_RBJ_SET,		2},
	{INST_MESSAGE_ON,	6},
	{0x5c,		2},
	{0x5d,		1},
	{0x5e,		3},
	{0x5f,		2},

	/* 0x60-0x6f */
	{0x60,			22},
	{INST_DOOR_SET,	sizeof(script_door_set_t)},
	{0x62,			40},
	{INST_AOT_SET,		20},
	{INST_AOT_SET_4P,	28},
	{INST_AOT_RESET,	10},
	{0x66,			2},
	{INST_ITEM_AOT_SET,	22},
	{0x68,			30},
	{INST_KAGE_SET,	14},
	{INST_SUPER_SET,	16},
	{0x6b,			2},
	{0x6c,			4},
	{0x6d,			4},
	{INST_SCA_ID_SET,	4},
	{0x6f,			2},

	/* 0x70-0x7f */
	{INST_ESPR_ON,	16},
	{0x71, 		18},
	{0x72, 		22},
	{INST_ESPR3D_ON2,	24},
	{INST_ESPR_KILL,	5},
	{INST_ESPR_KILL2,	2},
	{0x76, 		3},
	{INST_SE_ON,	12},
	{INST_BGM_CTL,	6},
	{INST_XA_ON,	4},
	{0x7a, 		2},
	{INST_BGM_TBL_SET,	6},
	{0x7c,		1},
	{INST_EM_SET,	24},
	{0x7e,		2},
	{INST_OM_SET,	40},

	/* 0x80-0x8f */
	{INST_PLC_MOTION,	4},
	{INST_PLC_DEST,		8},
	{INST_PLC_NECK,		10},
	{INST_PLC_RET,		1},
	{INST_PLC_FLG,		4},
	{0x85,		2},
	{0x86,		1},
	{INST_PLC_STOP,	1},
	{INST_PLC_ROT,	4},
	{INST_PLC_CNT,	2},
	{0x8a,		1},
	{0x8b,		1},
	{0x8c,		1},
	{0x8e,		4},
	{0x8f,		2}
};

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this);
static int scriptGetInstLen(room_t *this);
static void scriptPrintInst(room_t *this);
static void scriptExecInst(room_t *this);

static void scriptDisasmInit(void);

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

	logMsg(2, "rdt3: Init script at offset 0x%08x, length 0x%04x\n", offset, this->script_length);

	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivPrintInst = scriptPrintInst;
	this->scriptPrivExecInst = scriptExecInst;
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

	scriptDisasmInit();

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

				roomDoor.x = SDL_SwapLE16(doorSet->x);
				roomDoor.y = SDL_SwapLE16(doorSet->y);
				roomDoor.w = SDL_SwapLE16(doorSet->w);
				roomDoor.h = SDL_SwapLE16(doorSet->h);

				roomDoor.next_x = SDL_SwapLE16(doorSet->next_x);
				roomDoor.next_y = SDL_SwapLE16(doorSet->next_y);
				roomDoor.next_z = SDL_SwapLE16(doorSet->next_z);
				roomDoor.next_dir = SDL_SwapLE16(doorSet->next_dir);

				roomDoor.next_stage = doorSet->next_stage+1;
				roomDoor.next_room = doorSet->next_room;
				roomDoor.next_camera = doorSet->next_camera;

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

static void scriptDisasmInit(void)
{
}

#else

/*--- Types ---*/

typedef struct {
	Uint8 value;
	char *name;
} script_dump_t;

/*--- Constants ---*/

static const script_dump_t work_set_0[]={
	{1, "PL_WK"},
	{3, "EM_WK"},
	{4, "OM_WK"}
};

/*--- Variables ---*/

static int numFunc;
static int indentLevel;

static char strBuf[256];
static char tmpBuf[256];

/*--- Functions ---*/

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

static void scriptDisasmInit(void)
{
	indentLevel = 0;
	numFunc = 0;
}

static void scriptPrintInst(room_t *this)
{
	int i;
	script_inst_t *inst;
	const char *nameFromValue;

	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	inst = (script_inst_t *) this->cur_inst;

	memset(strBuf, 0, sizeof(strBuf));

	if ((indentLevel==0) && (inst->opcode!=0xff)) {
		sprintf(strBuf, "func%02x() {\n", numFunc++);
		++indentLevel;
	}

	switch(inst->opcode) {

		/* 0x00-0x0f */

		case INST_NOP:
			reindent(indentLevel);
			strcat(strBuf, "nop\n");
			break;
		case INST_RETURN:
			if (indentLevel>1) {
				reindent(indentLevel);
				strcat(strBuf, "return\n");
			} else {
				reindent(--indentLevel);
				strcat(strBuf, "}\n\n");
			}
			break;
		case INST_SLEEP_1:
			reindent(indentLevel);
			strcat(strBuf, "sleep 1\n");
			break;
		case INST_EVT_EXEC:
			reindent(indentLevel);
			sprintf(tmpBuf, "EVT_EXEC 0x%02x func%02x()\n",
				inst->exec.mask, inst->exec.func[1]);
			strcat(strBuf, tmpBuf);
			break;
		case INST_EVT_KILL:
			reindent(indentLevel);
			strcat(strBuf, "EVT_KILL xxx\n");
			break;
		case INST_IF:
			reindent(indentLevel++);
			strcat(strBuf, "if (xxx) {\n");
			break;
		case INST_ELSE:
			reindent(--indentLevel);
			strcat(strBuf, "} else {\n");
			break;
		case INST_END_IF:
			reindent(--indentLevel);
			strcat(strBuf, "}\n");
			break;
		case INST_SLEEP_N:
			reindent(indentLevel);
			sprintf(tmpBuf, "sleep %d\n", SDL_SwapLE16(inst->sleepn.delay));
			strcat(strBuf, tmpBuf);
			break;
		case INST_SLEEP_W:
			reindent(indentLevel);
			strcat(strBuf, "sleepw\n");
			break;
		case INST_FOR:
			reindent(indentLevel++);
			sprintf(tmpBuf, "for (%d) {\n", SDL_SwapLE16(inst->i_for.count));
			strcat(strBuf, tmpBuf);
			break;
		case INST_FOR_END:
			reindent(--indentLevel);
			strcat(strBuf, "}\n");
			break;

		/* 0x10-0x1f */

		case INST_WHILE:
			reindent(indentLevel++);
			strcat(strBuf, "while (xxx) {\n");
			break;
		case INST_WHILE_END:
			reindent(--indentLevel);
			strcat(strBuf, "}\n");
			break;
		case INST_DO:
			reindent(indentLevel++);
			strcat(strBuf, "do {\n");
			break;
		case INST_DO_END:
			reindent(--indentLevel);
			strcat(strBuf, "} while (xxx)\n");
			break;
		case INST_SWITCH:
			reindent(indentLevel);
			strcat(strBuf, "switch(xxx) {\n");
			indentLevel += 2;
			break;
		case INST_CASE:
			reindent(indentLevel-1);
			strcat(strBuf, "case xxx:\n");
			break;
		case INST_SWITCH_END:
			indentLevel -= 2;
			reindent(indentLevel);
			strcat(strBuf, "}\n");
			break;
		case INST_GOTO:
			reindent(indentLevel);
			strcat(strBuf, "goto xxx\n");
			break;
		case INST_FUNC:
			reindent(indentLevel);
			sprintf(tmpBuf, "func%02x()\n", inst->func.num_func);
			strcat(strBuf, tmpBuf);
			break;
		case INST_BREAK:
			reindent(indentLevel);
			strcat(strBuf, "break\n");
			break;
		case INST_EVAL_CC:
			reindent(indentLevel);
			strcat(strBuf, "EVAL_CC xxx\n");
			break;
		case 0x1e:
		case 0x1f:
			reindent(indentLevel);
			strcat(strBuf, "set xxx\n");
			break;

		/* 0x20-0x2f */

		case INST_CALC_OP:
			reindent(indentLevel);
			strcat(strBuf, "CALC_OP xxx\n");
			break;
		case INST_EVT_CUT:
			reindent(indentLevel);
			strcat(strBuf, "EVT_CUT xxx\n");
			break;
		case INST_LINE_BEGIN:
			reindent(indentLevel);
			strcat(strBuf, "LINE_BEGIN xxx\n");
			break;
		case INST_LINE_MAIN:
			reindent(indentLevel);
			strcat(strBuf, "LINE_MAIN xxx\n");
			break;
		case INST_LINE_END:
			reindent(indentLevel);
			strcat(strBuf, "LINE_END xxx\n");
			break;

		/* 0x30-0x3f */

		case INST_LIGHT_COLOR_SET:
			reindent(indentLevel);
			strcat(strBuf, "LIGHT_COLOR_SET xxx\n");
			break;
		case INST_AHEAD_ROOM_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "AHEAD_ROOM_SET 0x%04x\n", SDL_SwapLE16(inst->ahead_room_set.value));
			strcat(strBuf, tmpBuf);
			break;
		case INST_EVAL_BGM_TBL_CK:
			reindent(indentLevel);
			strcat(strBuf, "EVAL_BGM_TBL_CK xxx\n");
			break;
		case INST_CHASER_ITEM_SET:
			reindent(indentLevel);
			strcat(strBuf, "CHASER_ITEM_SET xxx\n");
			break;
		case INST_FLOOR_SET:
			reindent(indentLevel);
			strcat(strBuf, "FLOOR_SET xxx\n");
			break;

		/* 0x40-0x4f */

		case INST_VAR_SET:
			{
				script_var_set_t *varSet = (script_var_set_t *) inst;

				reindent(indentLevel);
				sprintf(tmpBuf, "SET var%02x = %d\n", varSet->num_var, SDL_SwapLE16(varSet->value));
				strcat(strBuf, tmpBuf);
			}
			break;
		case INST_CALC_STORE:
			reindent(indentLevel);
			strcat(strBuf, "CALC_STORE xxx\n");
			break;
		case INST_CALC_LOAD:
			reindent(indentLevel);
			strcat(strBuf, "CALC_LOAD xxx\n");
			break;
		case INST_FADE_SET:
			reindent(indentLevel);
			strcat(strBuf, "FADE_SET xxx\n");
			break;
		case INST_WORK_SET:
			{
				reindent(indentLevel);

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
		case INST_EVAL_CK:
			reindent(indentLevel);
			strcat(strBuf, "EVAL_CK xxx\n");
			break;
		case INST_FLAG_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET 0x%02x object 0x%02x %s\n",
				inst->set_flag.flag,
				inst->set_flag.object,
				(inst->set_flag.value ? "on" : "off"));
			strcat(strBuf, tmpBuf);
			break;
		case INST_EVAL_CMP:
			reindent(indentLevel);
			strcat(strBuf, "EVAL_CMP xxx\n");
			break;

		/* 0x50-0x5f */

		case INST_CUT_CHG:
			reindent(indentLevel);
			strcat(strBuf, "CUT_CHG xxx\n");
			break;
		case INST_CUT_AUTO:
			reindent(indentLevel);
			strcat(strBuf, "CUT_AUTO xxx\n");
			break;
		case INST_CUT_REPLACE:
			reindent(indentLevel);
			strcat(strBuf, "CUT_REPLACE xxx\n");
			break;
		case INST_POS_SET:
			reindent(indentLevel);
			strcat(strBuf, "POS_SET xxx\n");
			break;
		case INST_DIR_SET:
			reindent(indentLevel);
			strcat(strBuf, "DIR_SET xxx\n");
			break;
		case INST_SET_VIB0:
			reindent(indentLevel);
			strcat(strBuf, "SET_VIB0 xxx\n");
			break;
		case INST_SET_VIB_FADE:
			reindent(indentLevel);
			strcat(strBuf, "SET_VIB_FADE xxx\n");
			break;
		case INST_RBJ_SET:
			reindent(indentLevel);
			strcat(strBuf, "RBJ_SET xxx\n");
			break;
		case INST_MESSAGE_ON:
			reindent(indentLevel);
			strcat(strBuf, "MESSAGE_ON xxx\n");
			break;

		/* 0x60-0x6f */

		case INST_DOOR_SET:
			reindent(indentLevel);
			strcat(strBuf, "DOOR_SET xxx\n");
			break;
		case INST_AOT_SET:
			reindent(indentLevel);
			strcat(strBuf, "AOT_SET xxx\n");
			break;
		case INST_AOT_SET_4P:
			reindent(indentLevel);
			strcat(strBuf, "AOT_SET_4P xxx\n");
			break;
		case INST_AOT_RESET:
			reindent(indentLevel);
			strcat(strBuf, "AOT_RESET xxx\n");
			break;
		case INST_ITEM_AOT_SET:
			reindent(indentLevel);
			strcat(strBuf, "ITEM_AOT_SET xxx\n");
			break;
		case INST_KAGE_SET:
			reindent(indentLevel);
			strcat(strBuf, "KAGE_SET xxx\n");
			break;
		case INST_SUPER_SET:
			reindent(indentLevel);
			strcat(strBuf, "SUPER_SET xxx\n");
			break;
		case INST_SCA_ID_SET:
			reindent(indentLevel);
			strcat(strBuf, "SCA_ID_SET xxx\n");
			break;

		/* 0x70-0x7f */

		case INST_ESPR_ON:
			reindent(indentLevel);
			strcat(strBuf, "ESPR_ON xxx\n");
			break;
		case INST_ESPR3D_ON2:
			reindent(indentLevel);
			strcat(strBuf, "ESPR3D_ON2 xxx\n");
			break;
		case INST_ESPR_KILL:
			reindent(indentLevel);
			strcat(strBuf, "ESPR_KILL xxx\n");
			break;
		case INST_ESPR_KILL2:
			reindent(indentLevel);
			strcat(strBuf, "ESPR_KILL2 xxx\n");
			break;
		case INST_SE_ON:
			reindent(indentLevel);
			strcat(strBuf, "SE_ON xxx\n");
			break;
		case INST_BGM_CTL:
			reindent(indentLevel);
			strcat(strBuf, "BGM_CTL xxx\n");
			break;
		case INST_XA_ON:
			reindent(indentLevel);
			strcat(strBuf, "XA_ON xxx\n");
			break;
		case INST_BGM_TBL_SET:
			reindent(indentLevel);
			strcat(strBuf, "BGM_TBL_SET xxx\n");
			break;
		case INST_EM_SET:
			reindent(indentLevel);
			strcat(strBuf, "EM_SET xxx\n");
			break;
		case INST_OM_SET:
			reindent(indentLevel);
			strcat(strBuf, "OM_SET xxx\n");
			break;

		/* 0x80-0x8f */

		case INST_PLC_MOTION:
			reindent(indentLevel);
			strcat(strBuf, "PLC_MOTION xxx\n");
			break;
		case INST_PLC_DEST:
			reindent(indentLevel);
			strcat(strBuf, "PLC_DEST xxx\n");
			break;
		case INST_PLC_NECK:
			reindent(indentLevel);
			strcat(strBuf, "PLC_NECK xxx\n");
			break;
		case INST_PLC_RET:
			reindent(indentLevel);
			strcat(strBuf, "PLC_RET xxx\n");
			break;
		case INST_PLC_FLG:
			reindent(indentLevel);
			strcat(strBuf, "PLC_FLG xxx\n");
			break;
		case INST_PLC_STOP:
			reindent(indentLevel);
			strcat(strBuf, "PLC_STOP xxx\n");
			break;
		case INST_PLC_ROT:
			reindent(indentLevel);
			strcat(strBuf, "PLC_ROT xxx\n");
			break;
		case INST_PLC_CNT:
			reindent(indentLevel);
			strcat(strBuf, "PLC_CNT xxx\n");
			break;

		case INST_END_SCRIPT:
			break;

		default:
			reindent(indentLevel);
			sprintf(tmpBuf, "Unknown opcode 0x%02x\n", inst->opcode);
			strcat(strBuf, tmpBuf);
			break;

	}

	logMsg(1, "%s", strBuf);
}

#endif /* ENABLE_SCRIPT_DISASM */
