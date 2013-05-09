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

#include "log.h"
#include "parameters.h"

#include "../g_common/room.h"

#include "rdt.h"
#include "rdt_scd_common.h"

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

typedef struct {
	Uint8 array;
	Uint8 bit;
	const char *name;
} bitarray_name_t;

/*--- Constants ---*/

static const char *cmp_imm_name[7]={
	"EQ", "GT", "GE", "LT", "LE", "NE", "??"
};

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
	{0x10, 0, "#EM_Z_ANGLE"}	/* em[0x78].w */
	/*
	{0x11, 1, ""},	em[0x106].b
	{0x12, 1, ""},	em[0x154].w
	{0x13, 1, ""},	em[0x1c2].w
	{0x14, 1, ""},	em[0x1c4].w
	{0x15, 1, ""},	em[0x1c6].w
	{0x16, 1, ""},	em[0x1cc].w
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

static char strBuf[256];
static char tmpBuf[256];

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

		switch(inst->opcode) {
			/* Nops */

			case INST_NOP:
			case INST_NOP1E:
			case INST_NOP1F:
			case INST_NOP20:
			case INST_NOP8A:
			case INST_NOP8B:
			case INST_NOP8C:
				strcat(strBuf, "Nop\n");
				break;

			/* 0x00-0x0f */

			case INST_RETURN:
				strcat(strBuf, "Evt_end\n");
				break;
			case INST_DO_EVENTS:
				strcat(strBuf, "Evt_next\n");
				break;
			case INST_RESET:
				sprintf(tmpBuf, "Evt_chain event%02x()\n",
					inst->reset.num_func);
				strcat(strBuf, tmpBuf);
				break;
			case INST_EVT_EXEC:
				if (inst->evtexec.ex_opcode == INST_FUNC) {
					sprintf(tmpBuf, "Evt_exec 0x%02x, Gosub event%02x()\n",
						inst->evtexec.cond, inst->evtexec.num_func);
				} else {
					sprintf(tmpBuf, "Evt_exec 0x%02x, xxx\n",
						inst->evtexec.cond);
				}
				strcat(strBuf, tmpBuf);
				break;
			case 0x05:
				strcat(strBuf, "Evt_kill\n");
				break;
			case INST_IF:
				strcat(strBuf, "Ifel_ck\n");
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
				strcat(strBuf, "Else_ck\n");
				block_len = SDL_SwapLE16(inst->i_else.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_else_t)]);
				break;
			case INST_END_IF:
				strcat(strBuf, "Endif\n");
				break;
			case INST_SLEEP_INIT:
				sprintf(tmpBuf, "Sleep %d\n", SDL_SwapLE16(inst->sleep_init.count));
				strcat(strBuf, tmpBuf);
				break;
			case 0x0a:
				strcat(strBuf, "Sleeping\n");
				break;
			case 0x0b:
				strcat(strBuf, "Wsleep\n");
				break;
			case 0x0c:
				strcat(strBuf, "Wsleeping\n");
				break;
			case INST_BEGIN_LOOP:
				sprintf(tmpBuf, "For %d\n", SDL_SwapLE16(inst->loop.count));
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->loop.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_loop_t)]);
				break;
			case INST_END_LOOP:
				strcat(strBuf, "Next\n");
				break;
			case INST_BEGIN_WHILE:
				strcat(strBuf, "While\n");
				block_len = inst->begin_while.block_length;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_begin_while_t)]);
				break;

			/* 0x10-0x1f */

			case INST_END_WHILE:
				strcat(strBuf, "Ewhile\n");
				break;
			case INST_DO:
				strcat(strBuf, "Do\n");
				block_len = SDL_SwapLE16(inst->i_do.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_do_t)]);
				break;
			case INST_WHILE:
				strcat(strBuf, "Edwhile\n");
				block_len = inst->i_while.block_length;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_while_t)]);
				break;
			case INST_BEGIN_SWITCH:
				sprintf(tmpBuf, "Switch var%02x\n", inst->i_switch.varw);
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->i_switch.block_length)+2;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_switch_t)]);
				break;
			case INST_CASE:
				sprintf(tmpBuf, "Case 0x%04x\n", SDL_SwapLE16(inst->i_case.value));
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->i_case.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_case_t)]);
				break;
			case INST_DEFAULT:
				strcat(strBuf, "Default\n");
				break;
			case INST_END_SWITCH:
				strcat(strBuf, "Eswitch\n");
				break;
			case INST_GOTO:
				sprintf(tmpBuf, "Goto [0x%08x]\n", offset + /*sizeof(script_goto_t) +*/
					(Sint16) SDL_SwapLE16(inst->i_goto.rel_offset));
				strcat(strBuf, tmpBuf);
				break;
			case INST_FUNC:
				sprintf(tmpBuf, "Gosub event%02x\n", inst->func.num_func);
				strcat(strBuf, tmpBuf);
				break;
			case 0x19:
				strcat(strBuf, "Return\n");
				break;
			case INST_BREAK:
				strcat(strBuf, "Break\n");
				break;
			case 0x1b:
				strcat(strBuf, "For2\n");
				break;
			case 0x1c:
				strcat(strBuf, "Break_point\n");
				break;
			case INST_CHG_SCRIPT:
				{
					char myTmpBuf[512];

					sprintf(myTmpBuf, "Work_copy script[0x%08x] = var%02x.W & 0xff\n",
						offset + sizeof(script_chg_script_t) + inst->chg_script.offset,
						inst->chg_script.varw);
					strcat(strBuf, myTmpBuf);

					if (inst->chg_script.flag == 1) {
						logMsg(1, "0x%08x: %s", offset, strBuf);

						memset(strBuf, 0, sizeof(strBuf));
						reindent(indent);
						sprintf(tmpBuf, "Work_copy script[0x%08x] = (var%02x.W >> 8) & 0xff\n",
							offset + sizeof(script_chg_script_t) + inst->chg_script.offset + 1,
							inst->chg_script.varw);
						strcat(strBuf, tmpBuf);
					}
				}				
				break;

			/* 0x20-0x2f */

			case INST_BIT_TEST:
				{
					char myTmpBuf[40];

					getBitArrayName(myTmpBuf, inst->bittest.num_array, inst->bittest.bit_number);

					sprintf(tmpBuf, "Ck %s = %d\n", myTmpBuf, inst->bittest.value);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_BIT_CHG:
				{
					char myTmpBuf[40];

					getBitArrayName(myTmpBuf, inst->bitchg.num_array, inst->bitchg.bit_number);

					sprintf(tmpBuf, "Set %s %s\n",
						(inst->bitchg.op_chg == 0 ? "CLEAR" :
							(inst->bitchg.op_chg == 1 ? "SET" :
								(inst->bitchg.op_chg == 7 ? "CHG" :
								"INVALID")
							)
						), myTmpBuf
					);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CMP_VARW:
				{
					int compare = inst->cmp_varw.compare;
					if (compare>6) {
						compare = 6;
					}

					sprintf(tmpBuf, "Cmp var%02x.W %s %d\n",
						inst->cmp_varw.varw,
						cmp_imm_name[compare],
						(Sint16) SDL_SwapLE16(inst->cmp_varw.value));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_SET_VARW:
				sprintf(tmpBuf, "Save var%02x.W = %d\n", inst->set_varw.varw, (Sint16) SDL_SwapLE16(inst->set_varw.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_COPY_VARW:
				sprintf(tmpBuf, "Copy var%02x.W = var%02x.w\n", inst->copy_varw.dst, inst->copy_varw.src);
				strcat(strBuf, tmpBuf);
				break;
			case INST_OP_VARW_IMM:
				{
					switch(inst->op_varw_imm.operation) {
						case 0:
							sprintf(tmpBuf, "Calc var%02x.W += %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 1:
							sprintf(tmpBuf, "Calc var%02x.W -= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 2:
							sprintf(tmpBuf, "Calc var%02x.W *= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 3:
							sprintf(tmpBuf, "Calc var%02x.W /= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 4:
							sprintf(tmpBuf, "Calc var%02x.W %%= %d\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 5:
							sprintf(tmpBuf, "Calc var%02x.W |= 0x%04x\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 6:
							sprintf(tmpBuf, "Calc var%02x.W &= 0x%04x\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 7:
							sprintf(tmpBuf, "Calc var%02x.W ^= 0x%04x\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 8:
							sprintf(tmpBuf, "Calc var%02x.W = !var%02x.W\n", inst->op_varw_imm.varw, inst->op_varw_imm.varw);
							break;
						case 9:
							sprintf(tmpBuf, "Calc var%02x.W >>= %d /*logical */ \n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 10:
							sprintf(tmpBuf, "Calc var%02x.W <<= %d /*logical */\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
							break;
						case 11:
							sprintf(tmpBuf, "Calc var%02x.W >>= %d /* arithmetical */\n", inst->op_varw_imm.varw, (Sint16) (SDL_SwapLE16(inst->op_varw_imm.value)));
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
							sprintf(tmpBuf, "Calc2 var%02x.W += var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 1:
							sprintf(tmpBuf, "Calc2 var%02x.W -= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 2:
							sprintf(tmpBuf, "Calc2 var%02x.W *= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 3:
							sprintf(tmpBuf, "Calc2 var%02x.W /= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 4:
							sprintf(tmpBuf, "Calc2 var%02x.W %%= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 5:
							sprintf(tmpBuf, "Calc2 var%02x.W |= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 6:
							sprintf(tmpBuf, "Calc2 var%02x.W &= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 7:
							sprintf(tmpBuf, "Calc2 var%02x.W ^= var%02x.W\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 8:
							sprintf(tmpBuf, "Calc2 var%02x.W = !var%02x.W\n", inst->op_varw.varw, inst->op_varw.varw);
							break;
						case 9:
							sprintf(tmpBuf, "Calc2 var%02x.W >>= var%02x.W /*logical */ \n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 10:
							sprintf(tmpBuf, "Calc2 var%02x.W <<= var%02x.W /*logical */\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						case 11:
							sprintf(tmpBuf, "Calc2 var%02x.W >>= var%02x.W /* arithmetical */\n", inst->op_varw.varw, inst->op_varw.srcw);
							break;
						default:
							sprintf(tmpBuf, "# Invalid operation 0x%02x\n", inst->op_varw.operation);
							break;
					}

					strcat(strBuf, tmpBuf);
				}
				break;
			case 0x28:
				strcat(strBuf, "Sce_rnd\n");
				break;
			case INST_CAM_SET:
				sprintf(tmpBuf, "Cut_chg %d\n", inst->cam_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case 0x2a:
				strcat(strBuf, "Cut_old\n");
				break;
			case INST_PRINT_TEXT:
				{
					char tmpBuf[512];

					sprintf(tmpBuf, "Message_on 0x%02x\n", inst->print_text.id);
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					this->getText(this, 0, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
					logMsg(1, "0x%08x: #\tL0\t%s\n", offset, tmpBuf);

					this->getText(this, 1, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
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

					sprintf(tmpBuf, "Aot_set id=0x%02x, xxx, examine %s, activate %s, ??? %s\n",
						inst->espr3d_set.id, myTmpBuf[0], myTmpBuf[1], myTmpBuf[2]);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_TRIGGER_SET:
				sprintf(tmpBuf, "Obj_model_set 0x%02x, xxx\n", inst->trigger_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_REG_MEM:
				sprintf(tmpBuf, "Work_set %d,%d\n",
					inst->set_reg_mem.component, inst->set_reg_mem.index);
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_REG_IMM:
				sprintf(tmpBuf, "Speed_set %d,%d\n",
					inst->set_reg_imm.component, SDL_SwapLE16(inst->set_reg_imm.value));
				strcat(strBuf, tmpBuf);
				break;

			/* 0x30-0x3f */

			case INST_SET_REG_TMP:
				strcat(strBuf, "Add_speed\n");
				break;
			case INST_ADD_REG:
				strcat(strBuf, "Add_aspeed\n");
				break;
			case INST_EM_SET_POS:
				sprintf(tmpBuf, "Pos_set x=%d, y=%d, z=%d\n",
					SDL_SwapLE16(inst->set_reg_3w.value[0]),
					SDL_SwapLE16(inst->set_reg_3w.value[1]),
					SDL_SwapLE16(inst->set_reg_3w.value[2]));
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_REG3:
				sprintf(tmpBuf, "Dir_set %d,%d,%d\n",
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
						hexa ? "Member_set %s,0x%04x\n" : "Member_set %s,%d\n",
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

					sprintf(tmpBuf, "Member_set2 %s, var%02x.W\n", varname,
						inst->em_set_var_varw.varw);
					strcat(strBuf, tmpBuf);
				}
				break;
			case 0x36:
				sprintf(tmpBuf, "Se_on %d, 0x%04x,0x%04x x=%d,y=%d,z=%d\n",
					inst->inst36.unknown0, SDL_SwapLE16(inst->inst36.unknown1[0]),
					SDL_SwapLE16(inst->inst36.unknown1[1]), SDL_SwapLE16(inst->inst36.x),
					SDL_SwapLE16(inst->inst36.y), SDL_SwapLE16(inst->inst36.z));
				strcat(strBuf, tmpBuf);
				break;
			case INST_CAM_CHG:
				sprintf(tmpBuf, "Sca_id_set %d,%d\n",
					inst->cam_chg.unknown0, inst->cam_chg.camera);
				strcat(strBuf, tmpBuf);
				break;
			case INST_FLOOR_SET:
				sprintf(tmpBuf, "Flr_set %d,%d\n",
					inst->floor_set.id, inst->floor_set.unknown);
				strcat(strBuf, tmpBuf);
				break;
			case 0x39:
				strcat(strBuf, "Dir_ck\n");
				break;
			case INST_ESPR_SET:
				sprintf(tmpBuf, "Sce_espr_on 0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x\n",
					SDL_SwapLE16(inst->espr_set.unknown[0]), SDL_SwapLE16(inst->espr_set.unknown[1]),
					SDL_SwapLE16(inst->espr_set.unknown[2]), SDL_SwapLE16(inst->espr_set.unknown[3]),
					SDL_SwapLE16(inst->espr_set.unknown[4]), SDL_SwapLE16(inst->espr_set.unknown[5]),
					SDL_SwapLE16(inst->espr_set.unknown[6]));
				strcat(strBuf, tmpBuf);
				break;
			case INST_DOOR_SET:
				sprintf(tmpBuf, "Door_aot_set id=0x%02x, x=%d, y=%d, w=%d, h=%d\n",
					inst->door_set.id,
					(Sint16) SDL_SwapLE16(inst->door_set.x), (Sint16) SDL_SwapLE16(inst->door_set.y),
					(Sint16) SDL_SwapLE16(inst->door_set.w), (Sint16) SDL_SwapLE16(inst->door_set.h));
				strcat(strBuf, tmpBuf);
				break;
			case INST_STATUS_SET:
				sprintf(tmpBuf, "Cut_auto %s\n",
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

					sprintf(tmpBuf, "Member_copy var%02x.W = %s\n", 
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

					sprintf(tmpBuf, "Member_cmp %s xxx,0x%04x\n",
						cmp_imm_name[compare],
						SDL_SwapLE16(inst->cmp_imm.value));
					strcat(strBuf, tmpBuf);
				}
				break;
			case 0x3f:
				strcat(strBuf, "Plc_motion\n");
				break;

			/* 0x40-0x4f */

			case 0x40:
				strcat(strBuf, "Plc_dest\n");
				break;
			case 0x41:
				strcat(strBuf, "Plc_neck\n");
				break;
			case INST_STATUS_SHOW:
				strcat(strBuf, "Plc_ret\n");
				break;
			case 0x43:
				strcat(strBuf, "Plc_flg\n");
				break;
			case INST_EM_SET:
				sprintf(tmpBuf, "Sce_em_set id=0x%02x, model=0x%02x, pose=0x%04x, sound_bank=%d, killed=0x%02x, x=%d, y=%d, z=%d\n",
					inst->em_set.id, inst->em_set.model,
					SDL_SwapLE16(inst->em_set.pose), inst->em_set.sound_bank,
					inst->em_set.killed,
					(Sint16) SDL_SwapLE16(inst->em_set.x), (Sint16) SDL_SwapLE16(inst->em_set.y),
					(Sint16) SDL_SwapLE16(inst->em_set.z));
				strcat(strBuf, tmpBuf);
				break;
			case 0x45:
				strcat(strBuf, "Col_chg_set\n");
				break;
			case 0x46:
				{
					char myTmpBuf[32];

					sprintf(myTmpBuf, "0x%04x", SDL_SwapLE16(inst->inst46.unknown1[1]));
					if ((SDL_SwapLE16(inst->inst46.unknown1[1]) & 0xff) == 0x18) {
						sprintf(myTmpBuf, "function 0x%02x", (SDL_SwapLE16(inst->inst46.unknown1[1])>>8) & 0xff);
					}

					sprintf(tmpBuf, "Aot_reset id=0x%02x, %d,%d 0x%04x,%s,0x%04x\n",
						inst->inst46.id, inst->inst46.unknown0[0], inst->inst46.unknown0[1],
						SDL_SwapLE16(inst->inst46.unknown1[0]), myTmpBuf,
						SDL_SwapLE16(inst->inst46.unknown1[2]));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_ACTIVATE_OBJECT:
				sprintf(tmpBuf, "Aot_on id=0x%02x\n", inst->set_cur_obj.id);
				strcat(strBuf, tmpBuf);
				break;
			case 0x48:
				strcat(strBuf, "Super_set\n");
				break;
			case 0x49:
				strcat(strBuf, "Super_reset\n");
				break;
			case 0x4a:
				strcat(strBuf, "Plc_gun\n");
				break;
			case INST_CAMSWITCH_SWAP:
				sprintf(tmpBuf, "Cut_replace %d,%d\n",
					inst->camswitch_swap.cam[0], inst->camswitch_swap.cam[1]);
				strcat(strBuf, tmpBuf);
				break;
			case 0x4c:
				strcat(strBuf, "Sce_espr_kill\n");
				break;
			/*case 0x4d:
				strcat(strBuf, "???\n");
				break;*/
			case INST_ITEM_SET:
				{
					sprintf(tmpBuf, "Item_aot_set id=0x%02x, %d, amount %d\n",
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
			case 0x4f:
				strcat(strBuf, "Sce_key_ck\n");
				break;

			/* 0x50-0x5f */

			case 0x50:
				strcat(strBuf, "Sce_trg_ck\n");
				break;
			case INST_SND_SET:
				sprintf(tmpBuf, "Sce_bgm_control %d,%d,%d,%d,%d\n", inst->snd_set.unknown[0], inst->snd_set.unknown[1],
					inst->snd_set.unknown[2], inst->snd_set.unknown[3], inst->snd_set.unknown[4]);
				strcat(strBuf, tmpBuf);
				break;
			case 0x52:
				strcat(strBuf, "Sce_espr_control\n");
				break;
			case 0x53:
				strcat(strBuf, "Sce_fade_set\n");
				break;
			case 0x54:
				strcat(strBuf, "Sce_espr3d_on\n");
				break;
			case 0x55:
				strcat(strBuf, "Member_calc\n");
				break;
			case 0x56:
				strcat(strBuf, "Member_calc2\n");
				break;
			case 0x57:
				strcat(strBuf, "Sce_bgmtbl_set\n");
				break;
			case 0x58:
				strcat(strBuf, "Plc_rot\n");
				break;
			case INST_SND_PLAY:
				sprintf(tmpBuf, "Xa_on %d,%d\n", inst->snd_play.id, SDL_SwapLE16(inst->snd_play.value));
				strcat(strBuf, tmpBuf);
				break;
			case 0x5a:
				strcat(strBuf, "Weapon_chg\n");
				break;
			case 0x5b:
				strcat(strBuf, "Plc_cnt\n");
				break;
			case 0x5c:
				strcat(strBuf, "Sce_shake_on\n");
				break;
			case 0x5d:
				strcat(strBuf, "Mizu_div_set\n");
				break;
			case INST_ITEM_HAVE:
				sprintf(tmpBuf, "Keep_Item_ck %d\n", inst->item_have.id);
				strcat(strBuf, tmpBuf);
				logMsg(1, "0x%08x: %s", offset, strBuf);

				if (inst->item_have.id < 0x80) {
					sprintf(strBuf, "#\t%s\n", item_name[inst->item_have.id]);
				} else {
					sprintf(strBuf, "#\tUnknown item\n");
				}
				break;
			case 0x5f:
				strcat(strBuf, "Xa_vol\n");
				break;

			/* 0x60-0x6f */

			case 0x60:
				strcat(strBuf, "Kage_set\n");
				break;
			case 0x61:
				strcat(strBuf, "Cut_be_set\n");
				break;
			case INST_ITEM_REMOVE:
				sprintf(tmpBuf, "Sce_Item_lost %d\n", inst->item_remove.id);
				strcat(strBuf, tmpBuf);
				logMsg(1, "0x%08x: %s", offset, strBuf);

				if (inst->item_remove.id < 0x80) {
					sprintf(strBuf, "#\t%s\n", item_name[inst->item_remove.id]);
				} else {
					sprintf(strBuf, "#\tUnknown item\n");
				}
				break;
			case 0x63:
				strcat(strBuf, "Plc_gun_eff\n");
				break;
			case 0x64:
				strcat(strBuf, "Sce_espr_on2\n");
				break;
			case 0x65:
				strcat(strBuf, "Sce_espr_kill2\n");
				break;
			case 0x66:
				strcat(strBuf, "Plc_stop\n");
				break;
			case INST_WALL_SET:
				{
					int i;
					char v[32];

					sprintf(tmpBuf, "Aot_set_4p id=0x%02x, 0x%04x",
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
			case 0x68:
				strcat(strBuf, "Door_aot_set_4p\n");
				break;
			case 0x69:
				strcat(strBuf, "Item_aot_set_4p\n");
				break;
			case INST_LIGHT_POS_SET:
				sprintf(tmpBuf, "Light_pos_set light=%d, %c=%d\n",
					inst->light_pos_set.id,
					'x'+inst->light_pos_set.param-11,
					SDL_SwapLE16(inst->light_pos_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_RANGE_SET:
				sprintf(tmpBuf, "Light_kido_set light=%d, range=%d\n",
					inst->light_range_set.id,
					SDL_SwapLE16(inst->light_range_set.range));
				strcat(strBuf, tmpBuf);
				break;
			case 0x6c:
				strcat(strBuf, "Rbj_reset\n");
				break;
			case INST_BG_YPOS_SET:
				sprintf(tmpBuf, "Sce_scr_move #0x%04x\n", SDL_SwapLE16(inst->bg_ypos_set.y));
				strcat(strBuf, tmpBuf);
				break;
			case 0x6e:
				strcat(strBuf, "Parts_set\n");
				break;
			case INST_MOVIE_PLAY:
				sprintf(tmpBuf, "Movie_on #0x%02x\n", inst->movie_play.id);
				strcat(strBuf, tmpBuf);
				break;

			/* 0x70-0x7f */

			case 0x70:
				strcat(strBuf, "Splc_ret\n");
				break;
			case 0x71:
				strcat(strBuf, "Splc_sce\n");
				break;
			case 0x72:
				strcat(strBuf, "Super_on\n");
				break;
			case 0x73:
				strcat(strBuf, "Mirror_set\n");
				break;
			case 0x74:
				strcat(strBuf, "Sce_fade_adjust\n");
				break;
			case 0x75:
				strcat(strBuf, "Sce_espr3d_on2\n");
				break;
			case INST_ITEM_ADD:
				{
					sprintf(tmpBuf, "Sce_Item_get %d, amount %d\n", inst->item_add.id, inst->item_add.amount);
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					if (inst->item_add.id < 0x80) {
						sprintf(strBuf, "#\t%s\n", item_name[inst->item_add.id]);
					} else {
						sprintf(strBuf, "#\tUnknown item\n");
					}
				}
				break;
			case 0x77:
				strcat(strBuf, "Sce_line_start\n");
				break;
			case 0x78:
				strcat(strBuf, "Sce_line_main\n");
				break;
			case 0x79:
				strcat(strBuf, "Sce_line_end\n");
				break;
			case 0x7a:
				strcat(strBuf, "Sce_parts_bomb\n");
				break;
			case 0x7b:
				strcat(strBuf, "Sce_parts_down\n");
				break;
			case INST_LIGHT_COLOR_SET:
				sprintf(tmpBuf, "Light_color_set light=%d, r=0x%02x, g=0x%02x, b=0x%02x\n",
					inst->light_color_set.id, inst->light_color_set.r,
					inst->light_color_set.g, inst->light_color_set.b);
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_POS_CAM_SET:
				sprintf(tmpBuf, "Light_pos_set2 camera=%d, light=%d, %c=%d\n",
					inst->light_pos_cam_set.camera,
					inst->light_pos_cam_set.id,
					'x'+inst->light_pos_cam_set.param-11,
					SDL_SwapLE16(inst->light_pos_cam_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_RANGE_CAM_SET:
				sprintf(tmpBuf, "Light_kido_set2 camera=%d, light=%d, range=%d\n",
					inst->light_range_cam_set.camera,
					inst->light_range_cam_set.id,
					SDL_SwapLE16(inst->light_range_cam_set.range));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_COLOR_CAM_SET:
				sprintf(tmpBuf, "Light_color_set2 camera=%d, light=%d, r=0x%02x, g=0x%02x, b=0x%02x\n",
					inst->light_color_cam_set.camera,
					inst->light_color_cam_set.id, inst->light_color_cam_set.r,
					inst->light_color_cam_set.g, inst->light_color_cam_set.b);
				strcat(strBuf, tmpBuf);
				break;

			/* 0x80-0x8f */

			case 0x80:
				strcat(strBuf, "Se_vol\n");
				break;
			case INST_POISON_CHECK:
				strcat(strBuf, "Poison_ck\n");
				break;
			case INST_POISON_CLEAR:
				strcat(strBuf, "Poison_clr\n");
				break;
			case INST_ITEM_HAVE_AND_REMOVE:
				sprintf(tmpBuf, "Sce_Item_ck_Lost %d\n", inst->item_have_and_remove.id);
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
