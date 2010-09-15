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

#include "room.h"
#include "room_rdt2.h"
#include "room_rdt2_script_common.h"
#include "log.h"
#include "parameters.h"

#ifndef ENABLE_SCRIPT_DISASM

void room_rdt2_scriptDump(room_t *this, int num_script)
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

movies	
	0x04	zmovie/r10b.bin
	0x05	zmovie/r200.bin
	0x06	zmovie/r505.bin
	0x07	zmovie/r602.bin
	0x08	plX/zmovie/r108X.bin
	0x09	zmovie/r109.bin
	0x0a	plX/zmovie/r204X.bin
	0x0b	plX/zmovie/r408.bin / plX/zmovie/r409.bin
	0x0c	plX/zmovie/r700X.bin
	0x0d	plX/zmovie/r703X.bin

inst 53
	02 02 07 00 fe: fade in
                    \--> speed
*/

/*--- Types ---*/

typedef struct {
	Uint8 id;
	Uint8 hexa;	/* display as : 0: decimal, 1:hexa */
	const char *name;
} em_var_name_t;

/*--- Constants ---*/

static const char *cmp_imm_name[7]={
	"EQ", "GT", "GE", "LT", "LE", "NE", "??"
};

static const em_var_name_t em_var_name[]={
	{0x07, 1, "#EM_POSE"},
	{0x0b, 0, "#EM_X_POS"},
	{0x0c, 0, "#EM_Y_POS"},
	{0x0d, 0, "#EM_Z_POS"},
	{0x0e, 0, "#EM_X_ANGLE"},
	{0x0f, 0, "#EM_Y_ANGLE"},
	{0x10, 0, "#EM_Z_ANGLE"}
};

static const char *item_name[]={
	/* 0x00-0x0f */
	"", "Knife", "HK VP70", "Browning",
	"Custom handgun", "Magnum", "Custom magnum", "Shotgun",
	"Custom shotgun", "Grenade launcher + grenade rounds", "Grenade launcher + fire rounds", "Grenade launcher + acid rounds",
	"Bowgun", "Calico M950", "Sparkshot", "Ingram",

	/* 0x10-0x1f */
	"Flamethrower", "Rocket launcher", "Gatling gun", "Machine gun",
	"Handgun ammo", "Shotgun ammo", "Magnum ammo", "Flamer fuel",
	"Grenade rounds", "Fire rounds", "Acid rounds", "SMG ammo",
	"Sparkshot ammo", "Bowgun ammo", "Ink ribbon", "Small key",

	/* 0x20-0x2f */
	"Handgun parts", "Magnum parts", "Shotgun parts", "First aid spray",
	"Chemical F-09", "Chemical ACw-32", "Green herb", "Red herb",
	"Blue herb", "Green + green herbs", "Green + red herbs", "Green + blue herbs",
	"Green + green + green herbs", "Green + green + blue herbs", "Green + red + blue herbs", "Lighter",

	/* 0x30-0x3f */
	"Lockpick", "Sherry's photo", "Valve handle", "Red jewel",
	"Red card", "Blue card", "Serpent stone", "Jaguar stone",
	"Jaguar stone (left part)", "Jaguar stone (right part)", "Eagle stone", "Rook plug",
	"King plug", "Bishop plug", "Knight plug", "Weapon storage key",

	/* 0x40-0x4f */
	"Detonator", "C4", "C4 + detonator", "Crank",
	"Film A", "Film B", "Film C", "Unicorn medal",
	"Eagle medal", "Wolf medal", "Cog", "Manhole opener",
	"Main fuse", "Fuse case", "Vaccine", "Vaccine container",

	/* 0x50-0x5f */
	"Firestarter", "Base vaccine", "G-Virus", "Base vaccine (case only)",
	"Joint S plug", "Joint N plug", "Wire", "Ada's photo",
	"Cabin key", "Spade key", "Diamond key", "Heart key",
	"Club key", "Control panel key (down)", "Control panel key (up)", "Power room key",

	/* 0x60-0x6f */
	"MO disk", "Umbrella keycard", "Master key", "Weapons locker key",
	"", "", "", "",
	"Chris's diary", "", "Memo to Leon", "Police memorandum",
	"Operation report 1", "", "Mail to the chief", "Secretary's diary A",

	/* 0x70-0x7f */
	"Secretary's diary B", "", "", "",
	"", "", "", "Watchman's diary",
	"Chief's diary", "Sewer manager diary", "Sewer manager fax", "",
	"", "", "P-epsilon report", ""
};

/*113,115,116,117,123 = Operation report 2 ? extra costumes? room 2080 */
/*114,125 = User registration,Lab security manual ? room 60a0 */

/*--- Variables ---*/

static char strBuf[256];
static char tmpBuf[256];

/*--- Functions prototypes ---*/

static void reindent(int num_indent);
static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent);

/*--- Functions ---*/

void room_rdt2_scriptDump(room_t *this, int num_script)
{
	rdt2_header_t *rdt_header;
	Uint32 offset, smaller_offset, script_length;
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

static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent)
{
	script_inst_t *block_ptr;
	int inst_len, block_len;

	while (length>0) {
		block_ptr = NULL;

		memset(strBuf, 0, sizeof(strBuf));

		inst_len = this->scriptPrivGetInstLen((Uint8 *) inst);

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
			/* Nops */

			case INST_NOP:
			case INST_NOP1C:
			case INST_NOP1E:
			case INST_NOP1F:
			case INST_NOP20:
			case INST_NOP63:
			case INST_NOP8A:
			case INST_NOP8B:
			case INST_NOP8C:
				strcat(strBuf, "nop\n");
				break;

			/* 0x00-0x0f */

			case INST_RETURN:
				strcat(strBuf, "RETURN\n");
				break;
			case INST_DO_EVENTS:
				strcat(strBuf, "PROCESS_EVENTS\n");
				break;
			case INST_RESET:
				sprintf(tmpBuf, "RESET func%02x()\n",
					inst->reset.num_func);
				strcat(strBuf, tmpBuf);
				break;
			case INST_EVT_EXEC:
				if (inst->evtexec.ex_opcode == INST_FUNC) {
					sprintf(tmpBuf, "EVT_EXEC 0x%02x, func%02x()\n",
						inst->evtexec.cond, inst->evtexec.num_func);
				} else {
					sprintf(tmpBuf, "EVT_EXEC 0x%02x, xxx\n",
						inst->evtexec.cond);
				}
				strcat(strBuf, tmpBuf);
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
				strcat(strBuf, "ELSE_IF\n");
				block_len = SDL_SwapLE16(inst->i_else.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_else_t)]);
				break;
			case INST_END_IF:
				strcat(strBuf, "END_IF\n");
				break;
			case INST_SLEEP_INIT:
				sprintf(tmpBuf, "SLEEP_INIT %d\n", SDL_SwapLE16(inst->sleep_init.count));
				strcat(strBuf, tmpBuf);
				break;
			case INST_SLEEP_LOOP:
				strcat(strBuf, "SLEEP_LOOP\n");
				break;
			case INST_BEGIN_LOOP:
				sprintf(tmpBuf, "BEGIN_LOOP %d\n", SDL_SwapLE16(inst->loop.count));
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->loop.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_loop_t)]);
				break;
			case INST_END_LOOP:
				strcat(strBuf, "END_LOOP\n");
				break;
			case INST_BEGIN_WHILE:
				strcat(strBuf, "BEGIN_WHILE\n");
				block_len = inst->begin_while.block_length;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_begin_while_t)]);
				break;

			/* 0x10-0x1f */

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
				sprintf(tmpBuf, "BEGIN_SWITCH var%02x\n", inst->i_switch.varw);
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->i_switch.block_length)+2;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_switch_t)]);
				break;
			case INST_CASE:
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
				sprintf(tmpBuf, "GOTO [0x%08x]\n", offset + /*sizeof(script_goto_t) +*/
					(Sint16) SDL_SwapLE16(inst->i_goto.rel_offset));
				strcat(strBuf, tmpBuf);
				break;
			case INST_FUNC:
				sprintf(tmpBuf, "func%02x()\n", inst->func.num_func);
				strcat(strBuf, tmpBuf);
				break;
			case INST_BREAK:
				strcat(strBuf, "BREAK\n");
				break;
			case INST_CHG_SCRIPT:
				{
					char myTmpBuf[512];

					sprintf(myTmpBuf, "script[0x%08x] = var%02x.W & 0xff\n",
						offset + sizeof(script_chg_script_t) + inst->chg_script.offset,
						inst->chg_script.varw);
					strcat(strBuf, myTmpBuf);

					if (inst->chg_script.flag == 1) {
						logMsg(1, "0x%08x: %s", offset, strBuf);

						memset(strBuf, 0, sizeof(strBuf));
						reindent(indent);
						sprintf(tmpBuf, "script[0x%08x] = (var%02x.W >> 8) & 0xff\n",
							offset + sizeof(script_chg_script_t) + inst->chg_script.offset + 1,
							inst->chg_script.varw);
						strcat(strBuf, tmpBuf);
					}
				}				
				break;

			/* 0x20-0x2f */

			case INST_BIT_TEST:
				sprintf(tmpBuf, "BIT_TEST array 0x%02x, bit 0x%02x = %d\n",
					inst->bittest.num_array,
					inst->bittest.bit_number,
					inst->bittest.value);
				strcat(strBuf, tmpBuf);
				break;
			case INST_BIT_CHG:
				sprintf(tmpBuf, "BIT_CHG %s array 0x%02x, bit 0x%02x\n",
					(inst->bitchg.op_chg == 0 ? "CLEAR" :
						(inst->bitchg.op_chg == 1 ? "SET" :
							(inst->bitchg.op_chg == 7 ? "CHG" :
							"INVALID")
						)
					),
					inst->bitchg.num_array,
					inst->bitchg.bit_number);
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_VARW:
				sprintf(tmpBuf, "var%02x.W = %d\n", inst->set_varw.varw, (Sint16) SDL_SwapLE16(inst->set_varw.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_COPY_VARW:
				sprintf(tmpBuf, "var%02x.W = var%02x.w\n", inst->copy_varw.dst, inst->copy_varw.src);
				strcat(strBuf, tmpBuf);
				break;
			case INST_OP_VARW_IMM:
				{
					switch(inst->op_varw_imm.operation) {
						case 0:
							sprintf(tmpBuf, "var%02x.W += %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 1:
							sprintf(tmpBuf, "var%02x.W -= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 2:
							sprintf(tmpBuf, "var%02x.W *= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 3:
							sprintf(tmpBuf, "var%02x.W /= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 4:
							sprintf(tmpBuf, "var%02x.W %%= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 5:
							sprintf(tmpBuf, "var%02x.W |= 0x%04x\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 6:
							sprintf(tmpBuf, "var%02x.W &= 0x%04x\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 7:
							sprintf(tmpBuf, "var%02x.W ^= 0x%04x\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 8:
							sprintf(tmpBuf, "var%02x.W = !var%02x.W\n", inst->op_varw_imm.varw, inst->op_varw_imm.varw);
							break;
						case 9:
							sprintf(tmpBuf, "var%02x.W >>= %d /*logical */ \n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 10:
							sprintf(tmpBuf, "var%02x.W <<= %d /*logical */\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 11:
							sprintf(tmpBuf, "var%02x.W >>= %d /* arithmetical */\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						default:
							sprintf(tmpBuf, "# Invalid operation 0x%02x\n", inst->op_varw_imm.operation);
							break;
					}

					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_OP_VARW:
				{
					switch(inst->op_varw.operation) {
						case 0:
							sprintf(tmpBuf, "var%02x.W += var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 1:
							sprintf(tmpBuf, "var%02x.W -= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 2:
							sprintf(tmpBuf, "var%02x.W *= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 3:
							sprintf(tmpBuf, "var%02x.W /= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 4:
							sprintf(tmpBuf, "var%02x.W %%= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 5:
							sprintf(tmpBuf, "var%02x.W |= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 6:
							sprintf(tmpBuf, "var%02x.W &= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 7:
							sprintf(tmpBuf, "var%02x.W ^= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 8:
							sprintf(tmpBuf, "var%02x.W = !var%02x.W\n", inst->op_varw.varw, inst->op_varw.varw);
							break;
						case 9:
							sprintf(tmpBuf, "var%02x.W >>= var%02x.W /*logical */ \n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 10:
							sprintf(tmpBuf, "var%02x.W <<= var%02x.W /*logical */\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 11:
							sprintf(tmpBuf, "var%02x.W >>= var%02x.W /* arithmetical */\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						default:
							sprintf(tmpBuf, "# Invalid operation 0x%02x\n", inst->op_varw.operation);
							break;
					}

					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CMP_VARW:
				{
					int compare = inst->cmp_varw.compare;
					if (compare>6) {
						compare = 6;
					}

					sprintf(tmpBuf, "CMP_VARW var%02x.W %s %d\n",
						inst->cmp_varw.varw,
						cmp_imm_name[compare],
						(Sint16) SDL_SwapLE16(inst->cmp_varw.value));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CAM_SET:
				sprintf(tmpBuf, "CAM_SET %d\n", inst->cam_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_PRINT_TEXT:
				{
					char tmpBuf[512];

					sprintf(tmpBuf, "PRINT_TEXT 0x%02x\n", inst->print_text.id);
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					room_rdt2_getText(this, 0, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
					logMsg(1, "#\tL0\t%s\n", tmpBuf);

					room_rdt2_getText(this, 1, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
					sprintf(strBuf, "#\tL1\t%s\n", tmpBuf);
				}
				break;
			case INST_ESPR3D_SET:
				{
					char myTmpBuf[3][32];
					int i;

					for (i=0; i<3; i++) {
						sprintf(myTmpBuf[i], "0x%04x", SDL_SwapLE16(inst->espr3d_set.inst[i]));
						if ((SDL_SwapLE16(inst->espr3d_set.inst[i]) & 0xff) == 0x18) {
							sprintf(myTmpBuf[i], "function 0x%02x", (SDL_SwapLE16(inst->espr3d_set.inst[i])>>8) & 0xff);
						}	
					}

					sprintf(tmpBuf, "OBJECT 0x%02x = ESPR3D_SET xxx, examine %s, activate %s, ??? %s\n",
						inst->espr3d_set.id, myTmpBuf[0], myTmpBuf[1], myTmpBuf[2]);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_TRIGGER_SET:
				sprintf(tmpBuf, "TRIGGER 0x%02x = TRIGGER_SET xxx\n", inst->trigger_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_REG_MEM:
				sprintf(tmpBuf, "SET_ACTIVE_OBJECT %d,%d\n",
					inst->set_reg_mem.component, inst->set_reg_mem.index);
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_REG_IMM:
				sprintf(tmpBuf, "SET_REG_IMM %d,%d\n",
					inst->set_reg_imm.component, SDL_SwapLE16(inst->set_reg_imm.value));
				strcat(strBuf, tmpBuf);
				break;

			/* 0x30-0x3f */

			case INST_SET_REG_TMP:
				strcat(strBuf, "SET_REG_TMP\n");
				break;
			case INST_ADD_REG:
				strcat(strBuf, "ADD_REG\n");
				break;
			case INST_EM_SET_POS:
				sprintf(tmpBuf, "EM_SET_POS x=%d, y=%d, z=%d\n",
					SDL_SwapLE16(inst->set_reg_3w.value[0]),
					SDL_SwapLE16(inst->set_reg_3w.value[1]),
					SDL_SwapLE16(inst->set_reg_3w.value[2]));
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_REG3:
				sprintf(tmpBuf, "SET_REG3 %d,%d,%d\n",
					SDL_SwapLE16(inst->set_reg_3w.value[0]),
					SDL_SwapLE16(inst->set_reg_3w.value[1]),
					SDL_SwapLE16(inst->set_reg_3w.value[2]));
				strcat(strBuf, tmpBuf);
				break;
			case INST_EM_SET_VAR:
				{
					int i, hexa = 0;
					char varname[32];

					sprintf(varname, "0x%02x", inst->set_var.id);
					for (i=0; i<sizeof(em_var_name)/sizeof(em_var_name_t); i++) {
						if (em_var_name[i].id == inst->set_var.id) {
							sprintf(varname, em_var_name[i].name);
							hexa = em_var_name[i].hexa;
							break;
						}
					}

					sprintf(tmpBuf,
						hexa ? "EM_SET_VAR %s,0x%04x\n" : "EM_SET_VAR %s,%d\n",
						varname,
						SDL_SwapLE16(inst->set_var.value));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_EM_SET_VAR_VARW:
				{
					int i;
					char varname[32];

					sprintf(varname, "0x%02x", inst->em_set_var_varw.id);
					for (i=0; i<sizeof(em_var_name)/sizeof(em_var_name_t); i++) {
						if (em_var_name[i].id == inst->em_set_var_varw.id) {
							sprintf(varname, em_var_name[i].name);
							break;
						}
					}

					sprintf(tmpBuf, "EM_SET_VAR %s, var%02x.W\n", varname,
						inst->em_set_var_varw.varw);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CAM_CHG:
				sprintf(tmpBuf, "CAM_CHG %d,%d\n",
					inst->cam_chg.unknown0, inst->cam_chg.camera);
				strcat(strBuf, tmpBuf);
				break;
			case INST_DOOR_SET:
				sprintf(tmpBuf, "OBJECT 0x%02x = DOOR_SET x=%d, y=%d, w=%d, h=%d\n",
					inst->door_set.id,
					(Sint16) SDL_SwapLE16(inst->door_set.x), (Sint16) SDL_SwapLE16(inst->door_set.y),
					(Sint16) SDL_SwapLE16(inst->door_set.w), (Sint16) SDL_SwapLE16(inst->door_set.h));
				strcat(strBuf, tmpBuf);
				break;
			case INST_STATUS_SET:
				sprintf(tmpBuf, "STATUS_SET %s\n",
					(inst->status_set.screen == 0 ? "item" : "map"));
				strcat(strBuf, tmpBuf);
				break;
			case INST_EM_GET_VAR_VARW:
				{
					int i;
					char varname[32];

					sprintf(varname, "0x%02x", inst->em_get_var_varw.id);
					for (i=0; i<sizeof(em_var_name)/sizeof(em_var_name_t); i++) {
						if (em_var_name[i].id == inst->em_get_var_varw.id) {
							sprintf(varname, em_var_name[i].name);
							break;
						}
					}

					sprintf(tmpBuf, "var%02x.W = EM_GET_VAR %s\n", 
						inst->em_get_var_varw.varw, varname);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CMP_IMM:
				{
					int compare = inst->cmp_imm.compare;
					if (compare>6) {
						compare = 6;
					}

					sprintf(tmpBuf, "CMP_IMM %s xxx,0x%04x\n",
						cmp_imm_name[compare],
						SDL_SwapLE16(inst->cmp_imm.value));
					strcat(strBuf, tmpBuf);
				}
				break;

			/* 0x40-0x4f */

			case INST_STATUS_SHOW:
				strcat(strBuf, "STATUS_SHOW\n");
				break;
			case INST_EM_SET:
				sprintf(tmpBuf, "ENTITY 0x%02x = EM_SET model=0x%02x, pose=0x%04x, sound_bank=%d, killed=0x%02x, x=%d, y=%d, z=%d\n",
					inst->em_set.id, inst->em_set.model,
					SDL_SwapLE16(inst->em_set.pose), inst->em_set.sound_bank,
					inst->em_set.killed,
					(Sint16) SDL_SwapLE16(inst->em_set.x), (Sint16) SDL_SwapLE16(inst->em_set.y),
					(Sint16) SDL_SwapLE16(inst->em_set.z));
				strcat(strBuf, tmpBuf);
				break;
			case 0x46:
				{
					char myTmpBuf[32];

					sprintf(myTmpBuf, "0x%04x", SDL_SwapLE16(inst->inst46.unknown1[1]));
					if ((SDL_SwapLE16(inst->inst46.unknown1[1]) & 0xff) == 0x18) {
						sprintf(myTmpBuf, "function 0x%02x", (SDL_SwapLE16(inst->inst46.unknown1[1])>>8) & 0xff);
					}

					sprintf(tmpBuf, "TRIGGER_SET_ACTION OBJECT 0x%02x, %d,%d 0x%04x,%s,0x%04x\n",
						inst->inst46.id, inst->inst46.unknown0[0], inst->inst46.unknown0[1],
						SDL_SwapLE16(inst->inst46.unknown1[0]), myTmpBuf,
						SDL_SwapLE16(inst->inst46.unknown1[2]));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_ACTIVATE_OBJECT:
				sprintf(tmpBuf, "ACTIVATE_OBJECT 0x%02x\n", inst->set_cur_obj.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_CAMSWITCH_SWAP:
				sprintf(tmpBuf, "CAMSWITCH_SWAP %d,%d\n",
					inst->camswitch_swap.cam[0], inst->camswitch_swap.cam[1]);
				strcat(strBuf, tmpBuf);
				break;
			case INST_ITEM_SET:
				{
					sprintf(tmpBuf, "OBJECT 0x%02x = ITEM_SET %d, amount %d\n",
						inst->item_set.id,
						SDL_SwapLE16(inst->item_set.type),
						SDL_SwapLE16(inst->item_set.amount));
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					if (inst->item_set.type < 0x80) {
						sprintf(strBuf, "#\t%s\n", item_name[inst->item_set.type]);
					} else {
						sprintf(strBuf, "#\tUnknown item\n");
					}
				}
				break;

			/* 0x50-0x5f */

			case INST_SND_SET:
				sprintf(tmpBuf, "SND_SET %d,%d,%d,%d,%d\n", inst->snd_set.unknown[0], inst->snd_set.unknown[1],
					inst->snd_set.unknown[2], inst->snd_set.unknown[3], inst->snd_set.unknown[4]);
				strcat(strBuf, tmpBuf);
				break;
			case INST_SND_PLAY:
				sprintf(tmpBuf, "SND_PLAY %d,%d\n", inst->snd_play.id, SDL_SwapLE16(inst->snd_play.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_ITEM_HAVE:
				sprintf(tmpBuf, "ITEM_HAVE %d\n", inst->item_have.id);
				strcat(strBuf, tmpBuf);
				logMsg(1, "0x%08x: %s", offset, strBuf);

				if (inst->item_have.id < 0x80) {
					sprintf(strBuf, "#\t%s\n", item_name[inst->item_have.id]);
				} else {
					sprintf(strBuf, "#\tUnknown item\n");
				}
				break;

			/* 0x60-0x6f */

			case INST_ITEM_REMOVE:
				sprintf(tmpBuf, "ITEM_REMOVE %d\n", inst->item_remove.id);
				strcat(strBuf, tmpBuf);
				logMsg(1, "0x%08x: %s", offset, strBuf);

				if (inst->item_remove.id < 0x80) {
					sprintf(strBuf, "#\t%s\n", item_name[inst->item_remove.id]);
				} else {
					sprintf(strBuf, "#\tUnknown item\n");
				}
				break;
			case INST_WALL_SET:
				{
					int i;
					char v[32];

					sprintf(tmpBuf, "OBJECT 0x%02x = WALL_SET 0x%04x",
						inst->wall_set.id, SDL_SwapLE16(inst->wall_set.unknown0[0]));
					for (i=0; i<4; i++) {
						sprintf(v," x%d=%d y%d=%d",
							i, SDL_SwapLE16(inst->wall_set.xycoords[i<<1]),
							i, SDL_SwapLE16(inst->wall_set.xycoords[(i<<1)+1]));
						strcat(tmpBuf, v);
					}
					strcat(tmpBuf, "\n");
				}
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_POS_SET:
				sprintf(tmpBuf, "LIGHT_POS_SET light=%d, %c=%d\n",
					inst->light_pos_set.id,
					'x'+inst->light_pos_set.param-11,
					SDL_SwapLE16(inst->light_pos_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_RANGE_SET:
				sprintf(tmpBuf, "LIGHT_RANGE_SET light=%d, range=%d\n",
					inst->light_range_set.id,
					SDL_SwapLE16(inst->light_range_set.range));
				strcat(strBuf, tmpBuf);
				break;
			case INST_BG_YPOS_SET:
				sprintf(tmpBuf, "BG_YPOS_SET #0x%04x\n", SDL_SwapLE16(inst->bg_ypos_set.y));
				strcat(strBuf, tmpBuf);
				break;
			case INST_MOVIE_PLAY:
				sprintf(tmpBuf, "MOVIE_PLAY #0x%02x\n", inst->movie_play.id);
				strcat(strBuf, tmpBuf);
				break;

			/* 0x70-0x7f */

			case INST_ITEM_ADD:
				{
					sprintf(tmpBuf, "ITEM_ADD %d, amount %d\n", inst->item_add.id, inst->item_add.amount);
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					if (inst->item_add.id < 0x80) {
						sprintf(strBuf, "#\t%s\n", item_name[inst->item_add.id]);
					} else {
						sprintf(strBuf, "#\tUnknown item\n");
					}
				}
				break;
			case INST_LIGHT_COLOR_SET:
				sprintf(tmpBuf, "LIGHT_COLOR_SET light=%d, r=0x%02x, g=0x%02x, b=0x%02x\n",
					inst->light_color_set.id, inst->light_color_set.r,
					inst->light_color_set.g, inst->light_color_set.b);
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_POS_CAM_SET:
				sprintf(tmpBuf, "LIGHT_POS_CAM_SET camera=%d, light=%d, %c=%d\n",
					inst->light_pos_cam_set.camera,
					inst->light_pos_cam_set.id,
					'x'+inst->light_pos_cam_set.param-11,
					SDL_SwapLE16(inst->light_pos_cam_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_RANGE_CAM_SET:
				sprintf(tmpBuf, "LIGHT_RANGE_CAM_SET camera=%d, light=%d, range=%d\n",
					inst->light_range_cam_set.camera,
					inst->light_range_cam_set.id,
					SDL_SwapLE16(inst->light_range_cam_set.range));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_COLOR_CAM_SET:
				sprintf(tmpBuf, "LIGHT_COLOR_CAM_SET camera=%d, light=%d, r=0x%02x, g=0x%02x, b=0x%02x\n",
					inst->light_color_cam_set.camera,
					inst->light_color_cam_set.id, inst->light_color_cam_set.r,
					inst->light_color_cam_set.g, inst->light_color_cam_set.b);
				strcat(strBuf, tmpBuf);
				break;

			/* 0x80-0x8f */

			case INST_POISON_CHECK:
				strcat(strBuf, "POISON_CHECK\n");
				break;
			case INST_POISON_CLEAR:
				strcat(strBuf, "POISON_CLEAR\n");
				break;
			case INST_ITEM_HAVE_AND_REMOVE:
				sprintf(tmpBuf, "ITEM_HAVE_AND_REMOVE %d\n", inst->item_have_and_remove.id);
				strcat(strBuf, tmpBuf);
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
			if (inst->opcode == INST_BEGIN_LOOP) next_len = block_len - 2;
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
