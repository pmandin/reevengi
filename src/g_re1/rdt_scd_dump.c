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
/*#include "rdt_scd_common.h"*/

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

typedef struct {
	int model;
	const char *name;
} em_model_name_t;

/*--- Variables ---*/

static char strBuf[256];
static char tmpBuf[256];

static const em_model_name_t em_models[]={
	{0x00, "Zombie (Scientist)"},
	{0x01, "Zombie (Nude)"},
	{0x02, "Zombie (Dog)"},
	{0x03, "Spider (Brown)"},
	{0x04, "Spider (Grey)"},
	{0x05, "Crow"},
	{0x06, "Hunter"},
	{0x07, "Bee"},
	{0x08, "Tentacle"},
	{0x09, "Chimera"},
	{0x0a, "Snake"},
	{0x0b, "Shark"},
	{0x0c, "Tyran (Grey)"},
	{0x0d, "Yawn"},
	{0x0e, "Plant 42 (Roots)"},
	{0x0f, "Plant 42 (Tentacle)"},
	{0x10, "Tyran (Pink)"},
	{0x11, "Zombie"},
	{0x12, "Yawn (Injured)"},
	{0x13, "Web"},
	{0x14, "Arm"},
	{0x15, "Arm #2"},
	{0x20, "Chris Redfield"},
	{0x21, "Jill Valentine"},
	{0x22, "Barry Burton"},
	{0x23, "Rebecca Chambers"},
	{0x24, "Albert Wesker"},
	{0x25, "Kenneth J. Sullivan"},
	{0x26, "Forest Speyer"},
	{0x27, "Enrico Marini"},
	{0x28, "Character 4"},
	{0x29, "Kenneth (Injured)"},
	{0x2a, "Barry (Injured)"},
	{0x2b, "Barry (Prisoner?)"},
	{0x2c, "Rebecca (Prisoner?)"},
	{0x2d, "Barry (#2)"},
	{0x2e, "Wesker (#2)"},
	{0x30, "Chris (Special #1)"},
	{0x31, "Jill (Special #1)"},
	{0x32, "Chris (Special #2)"},
	{0x33, "Jill (Special #2)"}
};

static const char *door_anims[]={
	"door00", "door01", "door02", "door03",
	"door04", "door05", "door06", "door07",
	"door08", "door09", "door10", "door11",
	"door12", "door13", "door14", "mon",
	"ele03", "ele01", "ele01a", "ele01b",
	"ele02", "ele04", "kai01", "kai03",
	"kai02", "kai04", "lad00", "lad01",
	"door00k", "door01k", "door03k", "door05k",
	"door15"
};

/*--- Functions prototypes ---*/

static void reindent(int num_indent);
static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent);

static const char *getEmModelName(int num_model);

/*--- Functions ---*/

void rdt1_scd_scriptDump(room_t *this, int num_script)
{
	rdt1_header_t *rdt_header;
	Uint32 offset, script_length;
	Uint8 *scriptPtr;
	int room_script = RDT1_OFFSET_INIT_SCRIPT;

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

	logMsg(1, "0x%08x: BEGIN_FUNC func00\n", offset+2);
	scriptDumpBlock(this, (script_inst_t *) &scriptPtr[2], offset+2, script_length-2, 1);
	logMsg(1, "          : END_FUNC\n\n");
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

#if 0
			/* 0x00-0x0f */

			case INST_NOP:
				strcat(strBuf, "nop\n");
				break;
			case INST_IF:
				strcat(strBuf, "BEGIN_IF\n");
				block_len = inst->i_if.block_length;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_if_t)]);
				break;
			case INST_ELSE:
				strcat(strBuf, "ELSE\n");
				block_len = inst->i_else.block_length;
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_else_t)]);
				break;
			case INST_END_IF:
				strcat(strBuf, "END_IF\n");
				break;
			case INST_BIT_TEST:
				{
					script_bit_test_t *evalCk = (script_bit_test_t *) inst;

					sprintf(tmpBuf, "BIT_TEST flag 0x%02x object 0x%02x %s\n",
						evalCk->flag, evalCk->object,
						evalCk->value ? "on" : "off");
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_BIT_OP:
				strcat(strBuf, "BIT_OP xxx\n");
				break;
			case INST_CMP06:
				strcat(strBuf, "OBJ06_TEST xxx\n");
				break;
			case INST_CMP07:
				strcat(strBuf, "OBJ07_TEST xxx\n");
				break;
			case INST_STAGEROOMCAM_SET:
				{
					const char *objName = "Unknown";

					switch(inst->stageroomcam_set.object) {
						case 0:
							objName = "Stage";
							break;
						case 1:
							objName = "Room";
							break;
						case 2:
							objName = "Camera";
							break;
						default:
							break;
					}
					
					sprintf(tmpBuf, "STAGEROOMCAM_SET 0x%02x (%s) = 0x%02x (%d)\n",
						inst->stageroomcam_set.object, objName,
						SDL_SwapLE16(inst->stageroomcam_set.value),
						SDL_SwapLE16(inst->stageroomcam_set.value));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CUT_SET09:
				sprintf(tmpBuf, "CUT_SET09 %d\n", inst->cut_set.unknown);
				strcat(strBuf, tmpBuf);
				break;
			case INST_CUT_SET0A:
				sprintf(tmpBuf, "CUT_SET0A %d\n", inst->cut_set.unknown);
				strcat(strBuf, tmpBuf);
				break;
#if 0
			case INST_PRINT_MSG:
				{
					sprintf(tmpBuf, "PRINT_MSG 0x%02x\n", inst->print_msg.id);
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					this->getText(this, 0, inst->print_msg.id, tmpBuf, sizeof(tmpBuf));
					sprintf(strBuf, "#\t\t%s\n", tmpBuf);
				}
				break;
#endif
			case INST_DOOR_SET:
				{
					int next_stage = (inst->i_door_set.next_stage_and_room>>5) & 7;

					switch(next_stage) {
						case 0:
							next_stage=game->num_stage;
							break;
						case 1:
							next_stage=game->num_stage-1;
							break;
						case 2:
							next_stage=game->num_stage+1;
							break;
					}

					sprintf(tmpBuf, "OBJECT #0x%02x = DOOR_SET x=%d,y=%d,w=%d,h=%d anim=%d (%s), stage=%d,room=%d, px=%d,py=%d,pz=%d,pa=%d\n",
						inst->door_set.id,
						SDL_SwapLE16(inst->i_door_set.x), SDL_SwapLE16(inst->door_set.y),
						SDL_SwapLE16(inst->i_door_set.w), SDL_SwapLE16(inst->door_set.h),
						inst->i_door_set.anim,
						(inst->i_door_set.anim < sizeof(door_anims)/sizeof(const char *) ? door_anims[inst->i_door_set.anim] : "???" ),
						next_stage, inst->i_door_set.next_stage_and_room & 31,
						SDL_SwapLE16(inst->i_door_set.next_x), SDL_SwapLE16(inst->i_door_set.next_y),
						SDL_SwapLE16(inst->i_door_set.next_z), SDL_SwapLE16(inst->i_door_set.next_dir)
					);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_ITEM_SET:
				sprintf(tmpBuf, "OBJECT #0x%02x = ITEM_SET xxx\n", inst->item_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_NOP0E:
				strcat(strBuf, "nop\n");
				break;

			/* 0x10-0x1f */

			case INST_CMP10:
				strcat(strBuf, "OBJ10_TEST xxx\n");
				break;
			case INST_CMP11:
				strcat(strBuf, "OBJ11_TEST xxx\n");
				break;
			case INST_ITEM_ATTR_SET:
				sprintf(tmpBuf, "ITEM_ATTR_SET object=0x%02x, u0:%d,%d u1:%d,%d,%d\n",
					inst->item_attr_set.id,
					inst->item_attr_set.unknown0[0], inst->item_attr_set.unknown0[1],
					SDL_SwapLE16(inst->item_attr_set.unknown1[0]),
					SDL_SwapLE16(inst->item_attr_set.unknown1[1]),
					SDL_SwapLE16(inst->item_attr_set.unknown1[1])
				);
				strcat(strBuf, tmpBuf);
				break;
			case INST_ITEM_ATTR2_SET:
				sprintf(tmpBuf, "ITEM_ATTR2_SET object=0x%02x u0:%d,%d\n",
					inst->item_attr2_set.id,
					inst->item_attr2_set.unknown[0], inst->item_attr2_set.unknown[1]);
				strcat(strBuf, tmpBuf);
				break;
			case INST_ITEM_MODEL_SET:
				sprintf(tmpBuf, "OBJECT #0x%02x = ITEM_MODEL_SET xxx\n", inst->item_model_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_EM_SET:
				{
					const char *model_name = getEmModelName(inst->em_set.model);

					sprintf(tmpBuf, "EM_SET id=%d model=0x%02x (%s), killed=%d, a=%d, x=%d,y=%d,z=%d\n",
						inst->em_set.id, inst->em_set.model,
						(model_name ? model_name : "???"),
						inst->em_set.killed,
						SDL_SwapLE16(inst->em_set.a),
						SDL_SwapLE16(inst->em_set.x),
						SDL_SwapLE16(inst->em_set.y),
						SDL_SwapLE16(inst->em_set.z)
					);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_OM_SET:
				sprintf(tmpBuf, "OM_SET #0x%02x, xxx\n", inst->om_set.id);
				strcat(strBuf, tmpBuf);
				break;

			/* 0x20-0x2f */

			case INST_PLAYER_POS_SET:
				sprintf(tmpBuf,
					"PLAYER_POS_SET"
					" unknown0=%d,player_a=%d,unknown1=%d"
					",player_x=%d,player_y=%d,player_z=%d\n",
					SDL_SwapLE16(inst->player_pos_set.unknown0),
					SDL_SwapLE16(inst->player_pos_set.a),
					SDL_SwapLE16(inst->player_pos_set.unknown1),
					SDL_SwapLE16(inst->player_pos_set.x),
					SDL_SwapLE16(inst->player_pos_set.y),
					SDL_SwapLE16(inst->player_pos_set.z)
				);
				strcat(strBuf, tmpBuf);
				break;
			case INST_EM_POS_SET:
				sprintf(tmpBuf,
					"EM_POS_SET id=%d a=%d,unknown=%d,x=%d,y=%d,z=%d\n",
					inst->em_pos_set.id,
					SDL_SwapLE16(inst->em_pos_set.a),
					SDL_SwapLE16(inst->em_pos_set.unknown),
					SDL_SwapLE16(inst->em_pos_set.x),
					SDL_SwapLE16(inst->em_pos_set.y),
					SDL_SwapLE16(inst->em_pos_set.z)
				);
				strcat(strBuf, tmpBuf);
				break;

			/* 0x30-0x3f */

			case INST_37:
				sprintf(tmpBuf,
					"INST37_ARRAY_SET[%d][%d] = %d\n",
					inst->inst37.row, inst->inst37.col,
					inst->inst37.value
				);
				strcat(strBuf, tmpBuf);
				break;

			default:
				sprintf(tmpBuf, "Unknown opcode 0x%02x\n", inst->opcode);
				strcat(strBuf, tmpBuf);
				break;
#else

#include "rdt_scd_dumps.gen.c"

#endif
		}

		logMsg(1, "0x%08x: %s", offset, strBuf);

		if (block_ptr) {
			int next_len = block_len - inst_len;
			/*if (inst->opcode == INST_CASE) next_len = block_len;
			if (inst->opcode == INST_BEGIN_WHILE) next_len = block_len - 2;
			if (inst->opcode == INST_BEGIN_LOOP) next_len = block_len - 2;*/
			/*logMsg(1, " block 0x%04x inst 0x%04x next 0x%04x\n", block_len, inst_len, next_len);*/

			scriptDumpBlock(this, (script_inst_t *) block_ptr, offset+inst_len, next_len, indent+1);

			/*if (inst->opcode == INST_DO) next_len += sizeof(script_do_t);*/
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

static const char *getEmModelName(int num_model)
{
	int i;

	for (i=0; i<sizeof(em_models)/sizeof(em_model_name_t); i++) {
		if (em_models[i].model == num_model) {
			return em_models[i].name;
		}
	}

	return NULL;
}

#endif /* ENABLE_SCRIPT_DISASM */
