/*
	Room description
	RE2 RDT script

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <assert.h>

#include "room.h"
#include "room_rdt2.h"
#include "log.h"

/*--- Defines ---*/

/* Instructions */

#define INST_NOP	0x00
#define INST_RETURN	0x01
#define INST_DO_EVENTS	0x02
#define INST_RESET	0x03
#define INST_EVT_EXEC	0x04
#define INST_IF		0x06
#define INST_ELSE	0x07
#define INST_END_IF	0x08
#define INST_SLEEP_INIT	0x09	/* 0x09 and 0x0a are mixed in byte code this way : 0x09 0x0a 0xNN 0xNN */
#define INST_SLEEP_LOOP	0x0a
#define INST_BEGIN_LOOP	0x0d
#define INST_END_LOOP	0x0e

/*#define INST_EXIT_LOOP	0x12*/
#define INST_BEGIN_SWITCH	0x13
#define INST_CASE	0x14
#define INST_END_SWITCH	0x16
#define INST_GOTO	0x17
#define INST_FUNC	0x18
#define INST_BREAK	0x1a
#define INST_NOP1C	0x1c
#define INST_CHG_SCRIPT	0x1d
#define INST_NOP1E	0x1e
#define INST_NOP1F	0x1f

#define INST_NOP20	0x20
#define INST_BIT_TEST	0x21
#define INST_BIT_CHG	0x22
#define INST_CMP_VARW	0x23	/* var1a is current camera */
#define INST_SET_VARW	0x24
#define INST_COPY_VARW	0x25
#define INST_OP_VARW_IMM	0x26
#define INST_OP_VARW	0x27
#define INST_CAM_SET	0x29
#define INST_PRINT_TEXT	0x2b
#define INST_ESPR_SET	0x2c
#define INST_TRIGGER_SET	0x2d
#define INST_SET_REG_MEM	0x2e
#define INST_SET_REG_IMM	0x2f

#define INST_SET_REG_TMP	0x30
#define INST_ADD_REG	0x31
#define INST_EM_SET_POS	0x32
#define INST_SET_REG3	0x33
#define INST_EM_SET_VAR	0x34
#define INST_CAM_CHG	0x37
#define INST_DOOR_SET	0x3b
#define INST_BCHG8	0x3c
#define INST_CMP_IMM	0x3e

#define INST_EM_SET	0x44
#define INST_ACTIVATE_OBJECT	0x47
#define INST_CAMSWITCH_SWAP	0x4b
#define INST_ITEM_SET	0x4e

#define INST_SND_SET	0x51
#define INST_SND_PLAY	0x59
#define INST_ITEM_HAVE	0x5e

#define INST_ITEM_BELOW	0x62
#define INST_NOP63	0x63
#define INST_WALL_SET	0x67
#define INST_LIGHT_POS_SET	0x6a
#define INST_LIGHT3_POS_SET	0x6b
#define INST_MOVIE_PLAY	0x6f

#define INST_ITEM_ADD	0x76
#define INST_LIGHT_COLOR_SET	0x7c
#define INST_LIGHT_POS_CAM_SET	0x7d
#define INST_LIGHT3_POS_CAM_SET	0x7e
#define INST_LIGHT_COLOR_CAM_SET	0x7f

#define INST_ITEM_ABOVE	0x88
#define INST_NOP8A	0x8a
#define INST_NOP8B	0x8b
#define INST_NOP8C	0x8c

/* Effect sprites */

#define ESPR_LIGHT	0x03
#define ESPR_OBSTACLE	0x04
#define ESPR_TYPEWRITER	0x09
#define ESPR_BOX	0x0a
#define ESPR_FIRE	0x0b

/* possible room objects:
	inst2c: espr , fire can do damage
	inst3b: door
	inst4e: item
	inst67: wall
	inst68: ?, ptr+2 stored in object list, like others
	inst69: ?, ptr+2 stored in object list, like others
	inst8d: ?, ptr+2 stored in object list, like others
	inst51	1-
		22334455
*/

/*--- Types ---*/

typedef struct {
	Uint8 type;
	Uint8 unknown[3];
} script_condition_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown[2];
	Uint8 num_func;
} script_reset_t;

typedef struct {
	Uint8 opcode;
	Uint8 cond;
	Uint8 ex_opcode;	/* 0x18: execute function */
	Uint8 num_func;
} script_evtexec_t;

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
	Uint8 dummy;
	Uint16 count;
} script_sleep_init_t;

typedef struct {
	Uint8 opcode;
	Uint8 count[2];
} script_sleep_loop_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 block_length;
	Uint16 count;
} script_loop_t;

typedef struct {
	Uint8 opcode;
	Uint8 num_func;
} script_func_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 type;	/* ptr to this stored in room object list */
	Uint8 unknown0[3];
	Sint16 x,y,w,h;
	Uint16 inst[3];	/* Instructions to execute. 0:examine 1:activate 2:? or 0xff */
} script_espr_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown0[2];	/* ptr to this stored in room object list */
	Sint16 x,y,w,h;
	Sint16 next_x,next_y,next_z;
	Sint16 next_dir;
	Uint8 next_stage,next_room,next_camera;
	Uint8 unknown1;
	Uint8 door_type;
	Uint8 door_lock;
	Uint8 unknown2;
	Uint8 door_locked;
	Uint8 door_key;
	Uint8 unknown3;
} script_door_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 id;
	Uint8 unknown1[3];
} script_print_text_t;

typedef struct {
	Uint8 opcode;
	Uint8 component;
	Uint8 index;
} script_setregmem_t;

typedef struct {
	Uint8 opcode;
	Uint8 component;
	Sint16 value;
} script_setregimm_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown;
	Sint16 value[3];
} script_setreg3w_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 camera;
	Uint8 unknown1;
} script_cam_chg_t;

typedef struct {
	Uint8 opcode;
	Uint8 operation;
} script_bchg8_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 unknown1;
	Uint8 compare;
	Sint16 value;
} script_cmp_imm_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_set_cur_obj_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown0[2];	/* ptr to this stored in room object list */
	Sint16 x,y,w,h;
	Uint16 type;
	Uint16 amount;
	Uint16 unknown1[2];
} script_item_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 id;
	Uint8 model;

	Uint8 state;
	Uint8 unknown1[2];
	Uint8 sound_bank;

	Uint8 texture;
	Uint8 killed;
	Sint16 x,y,z,dir;

	Uint16 unknown2[2];
} script_em_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown[13];	/* ptr to this stored in room object list */
} script_wall_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown[18];
} script_trigger_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_cam_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 value;
} script_set_var_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 value;
} script_snd_play_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 amount;
} script_item_add_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_movie_play_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_item_have_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_item_below_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 unknown;
} script_item_above_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint8 id;
	Uint8 param;
	Sint16 value;
} script_light_pos_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 param;
	Sint16 value;
} script_light3_pos_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 r,g,b;
	Uint8 dummy;
} script_light_color_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 camera;
	Uint8 id;
	Uint8 param;
	Sint16 value;
} script_light_pos_cam_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint8 camera;
	Uint8 param;
	Sint16 value;
} script_light3_pos_cam_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 camera;
	Uint8 id;
	Uint8 r,g,b;
} script_light_color_cam_set_t;

/* 21 00 19 00: skip if player=claire */
/* 21 06 23 00: skip if entity 23 killed */

typedef struct {
	Uint8 opcode;
	Uint8 num_array;
	Uint8 bit_number;
	Uint8 value;
} script_bittest_t;

typedef struct {
	Uint8 opcode;
	Uint8 num_array;
	Uint8 bit_number;
	Uint8 op_chg;
} script_bitchg_t;

typedef struct {
	Uint8 opcode;
	Uint8 anim[4];
} script_inst4c_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy0;
	Uint8 anim[4];	/* Anim ID, same as inst 4c */
	Uint8 dummy1;
	Uint8 w,h;	/* Width,height of animation */
} script_inst3a_t;

typedef struct {
	Uint8 opcode;
	Uint8 varw;
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
	Uint8 dst;
	Uint8 src;
} script_copy_varw_t;

typedef struct {
	Uint8 opcode;
	Uint8 varw;
	Uint16 value;
} script_set_varw_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint8 operation; /* add,sub,mul,div, mod,or,and,xor, not,lsl,lsr,asr */
	Uint8 varw;
	Uint16 value;
} script_op_varw_imm_t;

typedef struct {
	Uint8 opcode;
	Uint8 operation; /* add,sub,mul,div, mod,or,and,xor, not,lsl,lsr,asr */
	Uint8 varw;
	Uint8 srcw;
} script_op_varw_t;

typedef struct {
	Uint8 opcode;
	Uint8 varw;
	Uint8 offset;
	Uint8 flag; /* 0 byte, 1 word */
} script_chg_script_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint8 varw;
	Uint8 compare;
	Uint16 value;
} script_cmp_varw_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown[5];
} script_snd_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 unknown0[2];
	Uint16 unknown1[3];	/* instructions to execute? 3*2 or 1*6 ? */
} script_inst46_t;

/* inst 3a,4c
0x0d 0x00 0x00 0x00: autre flamme, crash
0x0e 0x00 0x00 0x00: grande flamme
0x04 0x12 0x00 0x00: fumee
*/

/*typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 unknown[7];
} script_inst3a_t;*/

typedef struct {
	Uint8 opcode;
	Uint8 unknown[3];
	Sint16 rel_offset;
} script_goto_t;

typedef struct {
	Uint8 opcode;
	Uint8 cam[2];
} script_camswitch_swap_t;

typedef union {
	Uint8 opcode;
	script_reset_t		reset;
	script_evtexec_t	evtexec;
	script_if_t		i_if;
	script_else_t		i_else;
	script_sleep_init_t	sleep_init;
	script_sleep_loop_t	sleep_loop;
	script_loop_t		loop;
	script_switch_t		i_switch;
	script_case_t		i_case;
	script_func_t		func;
	script_bittest_t	bittest;
	script_bitchg_t		bitchg;
	script_door_set_t	door_set;
	script_espr_set_t	espr_set;
	script_print_text_t	print_text;
	script_setregmem_t	set_reg_mem;
	script_setregimm_t	set_reg_imm;
	script_setreg3w_t	set_reg_3w;
	script_cam_chg_t	cam_chg;
	script_bchg8_t		bchg8;
	script_cmp_imm_t	cmp_imm;
	script_set_cur_obj_t	set_cur_obj;
	script_item_set_t	item_set;
	script_em_set_t		em_set;
	script_wall_set_t	wall_set;
	script_trigger_set_t	trigger_set;
	script_cam_set_t	cam_set;
	script_set_var_t	set_var;
	script_snd_play_t	snd_play;
	script_item_add_t	item_add;
	script_movie_play_t	movie_play;
	script_item_have_t	item_have;
	script_item_below_t	item_below;
	script_item_above_t	item_above;
	script_light_pos_set_t	light_pos_set;
	script_light3_pos_set_t	light3_pos_set;
	script_light_color_set_t	light_color_set;
	script_light_pos_cam_set_t	light_pos_cam_set;
	script_light3_pos_cam_set_t	light3_pos_cam_set;
	script_light_color_cam_set_t	light_color_cam_set;
	script_copy_varw_t	copy_varw;
	script_set_varw_t	set_varw;
	script_op_varw_imm_t	op_varw_imm;
	script_op_varw_t	op_varw;
	script_chg_script_t	chg_script;
	script_cmp_varw_t	cmp_varw;
	script_snd_set_t	snd_set;
	script_inst46_t		inst46;
	script_inst3a_t		inst3a;
	script_goto_t		i_goto;
	script_camswitch_swap_t	camswitch_swap;
} script_inst_t;

typedef struct {
	Uint8 opcode;
	Uint8 length;
} script_inst_len_t;

/*--- Variables ---*/

static const char *cmp_imm_name[7]={
	"EQ", "GT", "GE", "LT", "LE", "NE", "??"
};

static const char *item_name[]={
	"", "Knife", "HK VP70", "Browning",
	"Custom handgun", "Magnum", "Custom magnum", "Shotgun",
	"Custom shotgun", "Grenade launcher + grenade rounds", "Grenade launcher + fire rounds", "Grenade launcher + acid rounds",
	"Bowgun", "Calico M950", "Sparkshot", "Ingram",
	"Flamethrower", "Rocket launcher", "Gatling gun", "Machine gun",
	"Handgun ammo", "Shotgun ammo", "Magnum ammo", "Flamer fuel",
	"Grenade rounds", "Fire rounds", "Acid rounds", "SMG ammo",
	"Sparkshot ammo", "Bowgun ammo", "Ink ribbon", "Small key",
	"Handgun parts", "Magnum parts", "Shotgun parts", "First aid spray",
	"Chemical F-09", "Chemical ACw-32", "Green herb", "Red herb",
	"Blue herb", "Green + green herbs", "Green + red herbs", "Green + blue herbs",
	"Green + green + green herbs", "Green + green + blue herbs", "Green + red + blue herbs", "Lighter",
	"Lockpick", "Sherry's photo", "Valve handle", "Red jewel",
	"Red card", "Blue card", "Serpent stone", "Jaguar stone",
	"Jaguar stone (left part)", "Jaguar stone (right part)", "Eagle stone", "Rook plug",
	"King plug", "Bishop plug", "Knight plug", "Weapon storage key",
	"Detonator", "C4", "C4 + detonator", "Crank",
	"Film A", "Film B", "Film C", "Unicord medal",
	"Eagle medal", "Wolf medal", "Cog", "Manhole opener",
	"Main fuse", "Fuse case", "Vaccine", "Vaccine container",
	"Firestarter", "Base vaccine", "G-Virus", "Base vaccine (case only)",
	"Joint S plug", "Joint N plug", "Wire", "Ada's photo",
	"Cabin key", "Spade key", "Diamond key", "Heart key",
	"Club key", "Control panel key (down)", "Control panel key (up)", "Power room key",
	"MO disk", "Umbrella keycard", "Master key", "Weapons locker key"
};

static const script_inst_len_t inst_length[]={
	/* 0x00-0x0f */
	{INST_NOP,	1},
	{INST_RETURN,	2},
	{INST_DO_EVENTS,	1},
	{INST_RESET,	sizeof(script_reset_t)},
	{INST_EVT_EXEC,	sizeof(script_evtexec_t)},
	{0x05,		2},
	{INST_IF,	sizeof(script_if_t)},
	{INST_ELSE,	sizeof(script_else_t)},
	{INST_END_IF,	2},
	{INST_SLEEP_INIT,	1},
	{INST_SLEEP_LOOP,	sizeof(script_sleep_loop_t)},
	{0x0b,		1},
	{0x0c,		1},
	{INST_BEGIN_LOOP,	sizeof(script_loop_t)},
	{INST_END_LOOP,	2},
	{0x0f,		2},

	/* 0x10-0x1f */
	{0x10,		2},
	{0x11,		4},
	{0x12,	2},
	{INST_BEGIN_SWITCH,	sizeof(script_switch_t)},
	{INST_CASE,	sizeof(script_case_t)},
	{INST_END_SWITCH,	2},
	{INST_GOTO,	sizeof(script_goto_t)},
	{INST_FUNC,	sizeof(script_func_t)},
	/*{0x19,		2},*/
	{INST_BREAK,	2},
	{0x1b,		6},
	{INST_NOP1C,	1},
	{INST_CHG_SCRIPT,	sizeof(script_chg_script_t)},
	{INST_NOP1E,	1},
	{INST_NOP1F,	1},

	/* 0x20-0x2f */
	{INST_NOP20,	1},
	{INST_BIT_TEST,	sizeof(script_bittest_t)},
	{INST_BIT_CHG,	sizeof(script_bitchg_t)},
	{INST_CMP_VARW,	sizeof(script_cmp_varw_t)},
	{INST_SET_VARW,		sizeof(script_set_varw_t)},
	{INST_COPY_VARW,	sizeof(script_copy_varw_t)},
	{INST_OP_VARW_IMM,	sizeof(script_op_varw_imm_t)},
	{INST_OP_VARW,		sizeof(script_op_varw_t)},
	{0x28,		1},
	{INST_CAM_SET,	sizeof(script_cam_set_t)},
	{0x2a,		1},
	{INST_PRINT_TEXT,	sizeof(script_print_text_t)},
	{INST_ESPR_SET,		sizeof(script_espr_set_t)},
	{INST_TRIGGER_SET,	sizeof(script_trigger_set_t)},
	{INST_SET_REG_MEM,	sizeof(script_setregmem_t)},
	{INST_SET_REG_IMM,	sizeof(script_setregimm_t)},

	/* 0x30-0x3f */
	{INST_SET_REG_TMP,	1},
	{INST_ADD_REG,		1},
	{INST_EM_SET_POS,	sizeof(script_setreg3w_t)},
	{INST_SET_REG3,		sizeof(script_setreg3w_t)},
	{INST_EM_SET_VAR,	sizeof(script_set_var_t)},
	{0x35,		3},
	{0x36,		12},
	{INST_CAM_CHG,	sizeof(script_cam_chg_t)},
	{0x38,		3},
	{0x39,		8},
	{0x3a,		sizeof(script_inst3a_t)},
	{INST_DOOR_SET,	sizeof(script_door_set_t)},
	{INST_BCHG8,	sizeof(script_bchg8_t)},
	{0x3d,		3},
	{INST_CMP_IMM,	sizeof(script_cmp_imm_t)},
	{0x3f,		4},

	/* 0x40-0x4f */
	{0x40,		8},
	{0x41,		10},
	{0x42,		1},
	{0x43,		4},
	{INST_EM_SET,	sizeof(script_em_set_t)},
	{0x45,		5},
	{0x46,		sizeof(script_inst46_t)},
	{INST_ACTIVATE_OBJECT,	sizeof(script_set_cur_obj_t)},
	{0x48,		16},
	{0x49,		8},
	{0x4a,		2},
	{INST_CAMSWITCH_SWAP,	sizeof(script_camswitch_swap_t)},
	{0x4c,		5},
	{0x4d,		22},
	{INST_ITEM_SET,	sizeof(script_item_set_t)},
	{0x4f,		4},

	/* 0x50-0x5f */
	{0x50,		4},
	{INST_SND_SET,	sizeof(script_snd_set_t)},
	{0x52,		6},
	{0x53,		6},
	{0x54,		22},
	{0x55,		6},
	{0x56,		4},
	{0x57,		8},
	{0x58,		4},
	{INST_SND_PLAY,		sizeof(script_snd_play_t)},
	{0x5a,		2},
	{0x5b,		2},
	{0x5c,		3},
	{0x5d,		2},
	{INST_ITEM_HAVE,	sizeof(script_item_have_t)},
	{0x5f,		2},

	/* 0x60-0x6f */
	{0x60,		14},
	{0x61,		4},
	{INST_ITEM_BELOW,	sizeof(script_item_below_t)},
	{INST_NOP63,	1},
	{0x64,		16},
	{0x65,		2},
	{0x66,		1},
	{INST_WALL_SET,		sizeof(script_wall_set_t)},
	{0x68,		40},
	/*{0x69,		2},*/
	{INST_LIGHT_POS_SET,	sizeof(script_light_pos_set_t)},
	{INST_LIGHT3_POS_SET,	sizeof(script_light3_pos_set_t)},
	{0x6c,		1},
	{0x6d,		4},
	{0x6e,		6},
	{INST_MOVIE_PLAY,	sizeof(script_movie_play_t)},

	/* 0x70-0x7f */
	{0x70,		1},
	{0x71,		1},
	{0x72,		16},
	{0x73,		8},
	{0x74,		4},
	{0x75,		22},
	{INST_ITEM_ADD,		sizeof(script_item_add_t)},
	{0x77,		4},
	{0x78,		6},
	{0x79,		1},
	{0x7a,		16},
	{0x7b,		16},
	{INST_LIGHT_COLOR_SET,	sizeof(script_light_color_set_t)},
	{INST_LIGHT_POS_CAM_SET,	sizeof(script_light_pos_cam_set_t)},
	{INST_LIGHT3_POS_CAM_SET,	sizeof(script_light3_pos_cam_set_t)},
	{INST_LIGHT_COLOR_CAM_SET,	sizeof(script_light_color_cam_set_t)},
	
	/* 0x80-0x8f */
	{0x80,		2},
	/*{0x81,		3},*/
	{0x82,		3},
	{0x83,		1},
	{0x84,		2},
	{0x85,		6},
	{0x86,		1},
	{0x87,		1},
	{INST_ITEM_ABOVE,	sizeof(script_item_above_t)},
	{0x89,		1},
	{INST_NOP8A,	6},
	{INST_NOP8B,	6},
	{INST_NOP8C,	8},
	{0x8d,		24},
	{0x8e,		24}
};

static char indentStr[256];

/*--- Functions prototypes ---*/

static Uint8 *scriptFirstInst(room_t *this, int num_script);
static int scriptGetInstLen(room_t *this);
static void scriptExecInst(room_t *this);
static void scriptPrintInst(room_t *this);

static void scriptDisasmInit(void);

static void scriptDumpAll(room_t *this, int num_script);
static int scriptDumpGetInstLen(Uint8 opcode);
static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent);

/*--- Functions ---*/

void room_rdt2_scriptInit(room_t *this)
{
	this->scriptPrivFirstInst = scriptFirstInst;
	this->scriptPrivGetInstLen = scriptGetInstLen;
	this->scriptPrivExecInst = scriptExecInst;
	this->scriptPrivPrintInst = scriptPrintInst;
}

static Uint8 *scriptFirstInst(room_t *this, int num_script)
{
	rdt2_header_t *rdt_header;
	Uint32 offset, smaller_offset;
	Uint16 *functionArrayPtr;
	int i, room_script = RDT2_OFFSET_INIT_SCRIPT;

	if (!this) {
		return NULL;
	}
	if (num_script == ROOM_SCRIPT_RUN) {
		room_script = RDT2_OFFSET_ROOM_SCRIPT;
	}

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[room_script]);

	this->script_length = this->cur_inst_offset = 0;
	this->cur_inst = NULL;

	if (offset>0) {
		/* Search smaller offset after script to calc length */
		smaller_offset = this->file_length;
		for (i=0; i<21; i++) {
			Uint32 next_offset = SDL_SwapLE32(rdt_header->offsets[i]);
			if ((next_offset>0) && (next_offset<smaller_offset) && (next_offset>offset)) {
				smaller_offset = next_offset;
			}
		}
		if (smaller_offset>offset) {
			this->script_length = smaller_offset - offset;
		}

		/* Start of script is an array of offsets to the various script functions
		 * The first offset also gives the first instruction to execute
		 */
		functionArrayPtr = (Uint16 *) (& ((Uint8 *) this->file)[offset]);

		this->cur_inst_offset = SDL_SwapLE16(functionArrayPtr[0]);
		this->cur_inst = (& ((Uint8 *) this->file)[offset + this->cur_inst_offset]);
	}

	logMsg(1, "rdt2: Script %d at offset 0x%08x, length 0x%04x\n", num_script, offset, this->script_length);

	scriptDisasmInit();

	scriptDumpAll(this, num_script);

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
#if 0
		case INST_ITEM_SET:
			{
				script_item_set_t *itemSet = (script_item_set_t *) inst;
				room_item_t item;

				item.x = SDL_SwapLE16(itemSet->x);
				item.y = SDL_SwapLE16(itemSet->y);
				item.w = SDL_SwapLE16(itemSet->w);
				item.h = SDL_SwapLE16(itemSet->h);

				if (itemSet->type == ITEM_OBSTACLE) {
					room_obstacle_t obstacle;

					obstacle.x = item.x;
					obstacle.y = item.y;
					obstacle.w = item.w;
					obstacle.h = item.h;

					this->addObstacle(this, &obstacle);
				} else {
					this->addItem(this, &item);
				}
			}
			break;
#endif
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

static void scriptDumpAll(room_t *this, int num_script)
{
}

static int scriptDumpGetInstLen(Uint8 opcode)
{
	return 0;
}

static void scriptDumpBlock(script_inst_t *inst, Uint32 offset, int length, int indent)
{
}

#else

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

static void scriptDisasmInit(void)
{
	indentLevel = 0;
	numFunc = 0;
}

static void scriptPrintInst(room_t *this)
{
	script_inst_t *inst;

	return;

	if (!this) {
		return;
	}
	if (!this->cur_inst) {
		return;
	}

	inst = (script_inst_t *) this->cur_inst;

	memset(strBuf, 0, sizeof(strBuf));

	if ((indentLevel==0) && (inst->opcode!=0xff)) {
		logMsg(1, "BEGIN_FUNC func%02x\n", numFunc++);
		++indentLevel;
	}

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
			reindent(indentLevel);
			strcat(strBuf, "nop\n");
			break;

		/* 0x00-0x0f */

		case INST_RETURN:
			if (indentLevel>1) {
				reindent(indentLevel);
				strcat(strBuf, "EXIT_FUNC\n");
			} else {
				reindent(--indentLevel);
				strcat(strBuf, "EXIT_FUNC\n\n");
			}
			break;
		case INST_DO_EVENTS:
			reindent(indentLevel);
			strcat(strBuf, "PROCESS_EVENTS\n");
			break;
		case INST_RESET:
			reindent(indentLevel);
			sprintf(tmpBuf, "RESET func%02x()\n",
				inst->reset.num_func);
			break;
		case INST_EVT_EXEC:
			reindent(indentLevel);
			if (inst->evtexec.ex_opcode == INST_FUNC) {
				sprintf(tmpBuf, "EVT_EXEC #0x%02x, func%02x()\n",
					inst->evtexec.cond, inst->evtexec.num_func);
			} else {
				sprintf(tmpBuf, "EVT_EXEC #0x%02x, xxx\n",
					inst->evtexec.cond);
			}
			strcat(strBuf, tmpBuf);
			break;
		case INST_IF:
			reindent(indentLevel++);
			strcat(strBuf, "BEGIN_IF\n");
			break;
		case INST_ELSE:
			reindent(--indentLevel);
			strcat(strBuf, "ELSE_IF\n");
			break;
		case INST_END_IF:
			reindent(--indentLevel);
			strcat(strBuf, "END_IF\n");
			break;
		case INST_SLEEP_INIT:
			reindent(indentLevel);
			sprintf(tmpBuf, "SLEEP_INIT #%d\n", SDL_SwapLE16(inst->sleep_init.count));
			strcat(strBuf, tmpBuf);
			break;
		case INST_SLEEP_LOOP:
			reindent(indentLevel);
			strcat(strBuf, "SLEEP_LOOP\n");
			break;
		case INST_BEGIN_LOOP:
			reindent(indentLevel);
			sprintf(tmpBuf, "BEGIN_LOOP #%d\n", SDL_SwapLE16(inst->loop.count));
			strcat(strBuf, tmpBuf);
			break;
		case INST_END_LOOP:
			reindent(indentLevel);
			strcat(strBuf, "END_LOOP\n");
			break;

		/* 0x10-0x1f */

		case INST_FUNC:
			reindent(indentLevel);
			sprintf(tmpBuf, "func%02x()\n", inst->func.num_func);
			strcat(strBuf, tmpBuf);
			break;

		/* 0x20-0x2f */

		case INST_BIT_TEST:
			reindent(indentLevel);
			sprintf(tmpBuf, "BIT_TEST array #0x%02x, bit #0x%04x\n", inst->bittest.num_array, inst->bittest.bit_number);
			strcat(strBuf, tmpBuf);
			break;
		case INST_BIT_CHG:
			reindent(indentLevel);
			sprintf(tmpBuf, "BIT_CHG %s array #0x%02x, bit #0x%04x\n",
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
		case INST_CAM_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "CAM_SET #0x%02x\n", inst->cam_set.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_PRINT_TEXT:
			{
				char tmpBuf[512];

				reindent(indentLevel);
				sprintf(tmpBuf, "PRINT_TEXT #0x%02x\n", inst->print_text.id);
				strcat(strBuf, tmpBuf);
				logMsg(1, "%s", strBuf);

				room_rdt2_getText(this, 0, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
				logMsg(1, "#\tL0\t%s\n", tmpBuf);

				room_rdt2_getText(this, 1, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
				sprintf(strBuf, "#\tL1\t%s\n", tmpBuf);
			}
			break;
		case INST_ESPR_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "OBJECT #0x%02x = ESPR_SET xxx\n", inst->espr_set.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_TRIGGER_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "OBJECT #0x%02x = TRIGGER_SET xxx\n", inst->trigger_set.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_SET_REG_MEM:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET_REG_MEM %d,%d\n",
				inst->set_reg_mem.component, inst->set_reg_mem.index);
			strcat(strBuf, tmpBuf);
			break;
		case INST_SET_REG_IMM:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET_REG_IMM %d,%d\n",
				inst->set_reg_imm.component, SDL_SwapLE16(inst->set_reg_imm.value));
			strcat(strBuf, tmpBuf);
			break;

		/* 0x30-0x3f */

		case INST_SET_REG_TMP:
			reindent(indentLevel);
			strcat(strBuf, "SET_REG_TMP\n");
			break;
		case INST_ADD_REG:
			reindent(indentLevel);
			strcat(strBuf, "ADD_REG\n");
			break;
		case INST_EM_SET_POS:
			reindent(indentLevel);
			sprintf(tmpBuf, "EM_SET_POS %d,%d,%d\n",
				SDL_SwapLE16(inst->set_reg_3w.value[0]),
				SDL_SwapLE16(inst->set_reg_3w.value[1]),
				SDL_SwapLE16(inst->set_reg_3w.value[2]));
			strcat(strBuf, tmpBuf);
			break;
		case INST_SET_REG3:
			reindent(indentLevel);
			sprintf(tmpBuf, "SET_REG3 %d,%d,%d\n",
				SDL_SwapLE16(inst->set_reg_3w.value[0]),
				SDL_SwapLE16(inst->set_reg_3w.value[1]),
				SDL_SwapLE16(inst->set_reg_3w.value[2]));
			strcat(strBuf, tmpBuf);
			break;
		case INST_EM_SET_VAR:
			{
				const char *varname = "";

				if (inst->set_var.id == 0x0f) {
					varname = "/* angle */";
				}

				reindent(indentLevel);
				sprintf(tmpBuf, "EM_SET_VAR #0x%02x,%d %s\n",
					inst->set_var.id,
					SDL_SwapLE16(inst->set_var.value),
					varname);
				strcat(strBuf, tmpBuf);
			}
			break;
		case INST_CAM_CHG:
			reindent(indentLevel);
			sprintf(tmpBuf, "CAM_CHG %d,%d\n",
				inst->cam_chg.unknown0, inst->cam_chg.camera);
			strcat(strBuf, tmpBuf);
			break;
		case INST_DOOR_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "OBJECT #0x%02x = DOOR_SET xxx\n", inst->door_set.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_BCHG8:
			reindent(indentLevel);
			sprintf(tmpBuf, "B%s #8,xxx\n",
				(inst->bchg8.operation == 1 ? "SET" : "CLR"));
			strcat(strBuf, tmpBuf);
			break;
		case INST_CMP_IMM:
			{
				int compare = inst->cmp_imm.compare;
				if (compare>6) {
					compare = 6;
				}

				reindent(indentLevel);
				sprintf(tmpBuf, "CMP_IMM %s xxx,0x%04x\n",
					cmp_imm_name[compare],
					SDL_SwapLE16(inst->cmp_imm.value));
				strcat(strBuf, tmpBuf);
			}
			break;

		/* 0x40-0x4f */

		case INST_EM_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "ENTITY #0x%02x = EM_SET model 0x%02x, killed 0x%02x\n",
				inst->em_set.id, inst->em_set.model, inst->em_set.killed);
			strcat(strBuf, tmpBuf);
			break;
		case INST_ACTIVATE_OBJECT:
			reindent(indentLevel);
			sprintf(tmpBuf, "ACTIVATE_OBJECT #0x%02x\n", inst->set_cur_obj.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_ITEM_SET:
			{
				reindent(indentLevel);
				sprintf(tmpBuf, "OBJECT #0x%02x = ITEM_SET %d, amount %d\n",
					inst->item_set.id,
					SDL_SwapLE16(inst->item_set.type),
					SDL_SwapLE16(inst->item_set.amount));
				strcat(strBuf, tmpBuf);
				logMsg(1, "%s", strBuf);

				if (inst->item_set.type < 64) {
					sprintf(strBuf, "#\t%s\n", item_name[inst->item_set.type]);
				} else {
					sprintf(strBuf, "#\tUnknown item\n");
				}
			}
			break;

		/* 0x50-0x5f */

		case INST_SND_PLAY:
			reindent(indentLevel);
			sprintf(tmpBuf, "SND_PLAY %d,%d\n", inst->snd_play.id, SDL_SwapLE16(inst->snd_play.value));
			strcat(strBuf, tmpBuf);
			break;
		case INST_ITEM_HAVE:
			reindent(indentLevel);
			sprintf(tmpBuf, "ITEM_HAVE %d\n", inst->item_have.id);
			strcat(strBuf, tmpBuf);
			break;

		/* 0x60-0x6f */

		case INST_ITEM_BELOW:
			reindent(indentLevel);
			sprintf(tmpBuf, "ITEM_BELOW %d\n", inst->item_below.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_WALL_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "OBJECT #0x%02x = WALL_SET xxx\n", inst->wall_set.id);
			strcat(strBuf, tmpBuf);
			break;
		case INST_LIGHT_POS_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "LIGHT_POS_SET %d,%c=%d\n",
				inst->light_pos_set.id,
				'x'+inst->light_pos_set.param-11,
				SDL_SwapLE16(inst->light_pos_set.value));
			strcat(strBuf, tmpBuf);
			break;
		case INST_LIGHT3_POS_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "LIGHT3_POS_SET %c=%d\n",
				'x'+inst->light3_pos_set.param,
				SDL_SwapLE16(inst->light3_pos_set.value));
			strcat(strBuf, tmpBuf);
			break;
		case INST_MOVIE_PLAY:
			reindent(indentLevel);
			sprintf(tmpBuf, "MOVIE_PLAY #0x%02x\n", inst->movie_play.id);
			strcat(strBuf, tmpBuf);
			break;

		/* 0x70-0x7f */

		case INST_ITEM_ADD:
			{
				reindent(indentLevel);
				sprintf(tmpBuf, "ITEM_ADD %d, amount %d\n", inst->item_add.id, inst->item_add.amount);
				strcat(strBuf, tmpBuf);
				logMsg(1, "%s", strBuf);

				if (inst->item_add.id < 64) {
					sprintf(strBuf, "#\t%s\n", item_name[inst->item_add.id]);
				} else {
					sprintf(strBuf, "#\tUnknown item\n");
				}
			}
			break;
		case INST_LIGHT_COLOR_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "LIGHT_COLOR_SET %d,r=0x%02x,g=0x%02x,b=0x%02x\n",
				inst->light_color_set.id, inst->light_color_set.r,
				inst->light_color_set.g, inst->light_color_set.b);
			strcat(strBuf, tmpBuf);
			break;
		case INST_LIGHT_POS_CAM_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "LIGHT_POS_CAM_SET camera %d,%d,%c=%d\n",
				inst->light_pos_cam_set.camera,
				inst->light_pos_cam_set.id,
				'x'+inst->light_pos_cam_set.param-11,
				SDL_SwapLE16(inst->light_pos_cam_set.value));
			strcat(strBuf, tmpBuf);
			break;
		case INST_LIGHT3_POS_CAM_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "LIGHT3_POS_CAM_SET camera %d,%c=%d\n",
				inst->light3_pos_cam_set.camera,
				'x'+inst->light3_pos_cam_set.param,
				SDL_SwapLE16(inst->light3_pos_cam_set.value));
			strcat(strBuf, tmpBuf);
			break;
		case INST_LIGHT_COLOR_CAM_SET:
			reindent(indentLevel);
			sprintf(tmpBuf, "LIGHT_COLOR_CAM_SET camera %d,%d,r=0x%02x,g=0x%02x,b=0x%02x\n",
				inst->light_color_cam_set.camera,
				inst->light_color_cam_set.id, inst->light_color_cam_set.r,
				inst->light_color_cam_set.g, inst->light_color_cam_set.b);
			strcat(strBuf, tmpBuf);
			break;

		/* 0x80-0x8f */

		case INST_ITEM_ABOVE:
			reindent(indentLevel);
			sprintf(tmpBuf, "ITEM_ABOVE %d\n", inst->item_above.id);
			strcat(strBuf, tmpBuf);
			break;

		default:
			reindent(indentLevel);
			sprintf(tmpBuf, "Unknown opcode 0x%02x\n", inst->opcode);
			strcat(strBuf, tmpBuf);
			break;

	}

	logMsg(1, "%s", strBuf);
}

static void scriptDumpAll(room_t *this, int num_script)
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

	num_funcs = SDL_SwapLE16(functionArrayPtr[0]) >> 1;
	logMsg(1, "Dump all\n");
	for (i=0; i<num_funcs; i++) {
		Uint16 func_offset = SDL_SwapLE16(functionArrayPtr[i]);
		Uint32 func_len = script_length - func_offset;
		script_inst_t *startInst = (script_inst_t *) (& ((Uint8 *) this->file)[offset + func_offset]);

		if (i<num_funcs-1) {
			func_len = SDL_SwapLE16(functionArrayPtr[i+1]) - func_offset;
		}

		logMsg(1, "BEGIN_FUNC func%02x\n", i);
		scriptDumpBlock(this, startInst, func_offset, func_len, 1);
		logMsg(1, "END_FUNC\n\n");
	}
}

static int scriptDumpGetInstLen(Uint8 opcode)
{
	int i;

	for (i=0; i< sizeof(inst_length)/sizeof(script_inst_len_t); i++) {
		if (inst_length[i].opcode == opcode) {
			return inst_length[i].length;
		}
	}

	return 0;
}

static void scriptDumpBlock(room_t *this, script_inst_t *inst, Uint32 offset, int length, int indent)
{
	while (length>0) {
		script_inst_t *block_ptr = NULL;
		int inst_len = 0, block_len;

		memset(strBuf, 0, sizeof(strBuf));
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
					sprintf(tmpBuf, "EVT_EXEC #0x%02x, func%02x()\n",
						inst->evtexec.cond, inst->evtexec.num_func);
				} else {
					sprintf(tmpBuf, "EVT_EXEC #0x%02x, xxx\n",
						inst->evtexec.cond);
				}
				strcat(strBuf, tmpBuf);
				break;
			case INST_IF:
				strcat(strBuf, "BEGIN_IF\n");
				block_len = SDL_SwapLE16(inst->i_if.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_if_t)]);
				{
					script_inst_t *end_block_ptr = (script_inst_t *) (&((Uint8 *) inst)[block_len]);
					if (end_block_ptr->opcode != INST_ELSE) {
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
				sprintf(tmpBuf, "SLEEP_INIT #%d\n", SDL_SwapLE16(inst->sleep_init.count));
				strcat(strBuf, tmpBuf);
				break;
			case INST_SLEEP_LOOP:
				strcat(strBuf, "SLEEP_LOOP\n");
				break;
			case INST_BEGIN_LOOP:
				sprintf(tmpBuf, "BEGIN_LOOP #%d\n", SDL_SwapLE16(inst->loop.count));
				strcat(strBuf, tmpBuf);
				block_len = SDL_SwapLE16(inst->loop.block_length);
				block_ptr = (script_inst_t *) (&((Uint8 *) inst)[sizeof(script_loop_t)]);
				break;
			case INST_END_LOOP:
				strcat(strBuf, "END_LOOP\n");
				break;

			/* 0x10-0x1f */

			/*case INST_EXIT_LOOP:
				strcat(strBuf, "EXIT_LOOP\n");
				break;*/
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
				sprintf(tmpBuf, "BIT_TEST array #0x%02x, bit #0x%02x = %d\n",
					inst->bittest.num_array,
					inst->bittest.bit_number,
					inst->bittest.value);
				strcat(strBuf, tmpBuf);
				break;
			case INST_BIT_CHG:
				sprintf(tmpBuf, "BIT_CHG %s array #0x%02x, bit #0x%02x\n",
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

					reindent(indentLevel);
					sprintf(tmpBuf, "CMP_VARW var%02x.W %s %d\n",
						inst->cmp_varw.varw,
						cmp_imm_name[compare],
						(Sint16) SDL_SwapLE16(inst->cmp_varw.value));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CAM_SET:
				sprintf(tmpBuf, "CAM_SET #0x%02x\n", inst->cam_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_PRINT_TEXT:
				{
					char tmpBuf[512];

					sprintf(tmpBuf, "PRINT_TEXT #0x%02x\n", inst->print_text.id);
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					room_rdt2_getText(this, 0, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
					logMsg(1, "#\tL0\t%s\n", tmpBuf);

					room_rdt2_getText(this, 1, inst->print_text.id, tmpBuf, sizeof(tmpBuf));
					sprintf(strBuf, "#\tL1\t%s\n", tmpBuf);
				}
				break;
			case INST_ESPR_SET:
				{
					char myTmpBuf[3][32];
					int i;

					for (i=0; i<3; i++) {
						sprintf(myTmpBuf[i], "0x%04x", SDL_SwapLE16(inst->espr_set.inst[i]));
						if ((SDL_SwapLE16(inst->espr_set.inst[i]) & 0xff) == 0x18) {
							sprintf(myTmpBuf[i], "function 0x%02x", (SDL_SwapLE16(inst->espr_set.inst[i])>>8) & 0xff);
						}	
					}

					sprintf(tmpBuf, "OBJECT #0x%02x = ESPR_SET xxx, examine %s, activate %s, ??? %s\n",
						inst->espr_set.id, myTmpBuf[0], myTmpBuf[1], myTmpBuf[2]);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_TRIGGER_SET:
				sprintf(tmpBuf, "TRIGGER #0x%02x = TRIGGER_SET xxx\n", inst->trigger_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_SET_REG_MEM:
				sprintf(tmpBuf, "SET_ACTIVE_OBJECT #%d,#%d\n",
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
				sprintf(tmpBuf, "EM_SET_POS %d,%d,%d\n",
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
					const char *varname = "";

					if (inst->set_var.id == 0x0f) {
						varname = "/* angle */";
					}

					sprintf(tmpBuf, "EM_SET_VAR #0x%02x,%d %s\n",
						inst->set_var.id,
						SDL_SwapLE16(inst->set_var.value),
						varname);
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_CAM_CHG:
				sprintf(tmpBuf, "CAM_CHG %d,%d\n",
					inst->cam_chg.unknown0, inst->cam_chg.camera);
				strcat(strBuf, tmpBuf);
				break;
			case INST_DOOR_SET:
				sprintf(tmpBuf, "OBJECT #0x%02x = DOOR_SET %d,%d %dx%d\n",
					inst->door_set.id,
					(Sint16) SDL_SwapLE16(inst->door_set.x), (Sint16) SDL_SwapLE16(inst->door_set.y),
					(Sint16) SDL_SwapLE16(inst->door_set.w), (Sint16) SDL_SwapLE16(inst->door_set.h));
				strcat(strBuf, tmpBuf);
				break;
			case INST_BCHG8:
				sprintf(tmpBuf, "B%s #8,xxx\n",
					(inst->bchg8.operation == 1 ? "SET" : "CLR"));
				strcat(strBuf, tmpBuf);
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

			case INST_EM_SET:
				sprintf(tmpBuf, "ENTITY #0x%02x = EM_SET model 0x%02x, killed 0x%02x, %d,%d,%d\n",
					inst->em_set.id, inst->em_set.model, inst->em_set.killed,
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

					sprintf(tmpBuf, "TRIGGER_SET_ACTION TRIGGER #0x%02x, %d,%d 0x%04x,%s,0x%04x\n",
						inst->inst46.id, inst->inst46.unknown0[0], inst->inst46.unknown0[1],
						SDL_SwapLE16(inst->inst46.unknown1[0]), myTmpBuf,
						SDL_SwapLE16(inst->inst46.unknown1[2]));
					strcat(strBuf, tmpBuf);
				}
				break;
			case INST_ACTIVATE_OBJECT:
				sprintf(tmpBuf, "ACTIVATE_OBJECT #0x%02x\n", inst->set_cur_obj.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_CAMSWITCH_SWAP:
				sprintf(tmpBuf, "CAMSWITCH_SWAP %d,%d\n",
					inst->camswitch_swap.cam[0], inst->camswitch_swap.cam[1]);
				strcat(strBuf, tmpBuf);
				break;
			case INST_ITEM_SET:
				{
					sprintf(tmpBuf, "OBJECT #0x%02x = ITEM_SET %d, amount %d\n",
						inst->item_set.id,
						SDL_SwapLE16(inst->item_set.type),
						SDL_SwapLE16(inst->item_set.amount));
					strcat(strBuf, tmpBuf);
					logMsg(1, "0x%08x: %s", offset, strBuf);

					if (inst->item_set.type < 64) {
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
				break;

			/* 0x60-0x6f */

			case INST_ITEM_BELOW:
				sprintf(tmpBuf, "ITEM_BELOW %d\n", inst->item_below.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_WALL_SET:
				sprintf(tmpBuf, "OBJECT #0x%02x = WALL_SET xxx\n", inst->wall_set.id);
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_POS_SET:
				sprintf(tmpBuf, "LIGHT_POS_SET %d,%c=%d\n",
					inst->light_pos_set.id,
					'x'+inst->light_pos_set.param-11,
					SDL_SwapLE16(inst->light_pos_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT3_POS_SET:
				sprintf(tmpBuf, "LIGHT3_POS_SET %c=%d\n",
					'x'+inst->light3_pos_set.param,
					SDL_SwapLE16(inst->light3_pos_set.value));
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

					if (inst->item_add.id < 64) {
						sprintf(strBuf, "#\t%s\n", item_name[inst->item_add.id]);
					} else {
						sprintf(strBuf, "#\tUnknown item\n");
					}
				}
				break;
			case INST_LIGHT_COLOR_SET:
				sprintf(tmpBuf, "LIGHT_COLOR_SET %d,r=0x%02x,g=0x%02x,b=0x%02x\n",
					inst->light_color_set.id, inst->light_color_set.r,
					inst->light_color_set.g, inst->light_color_set.b);
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_POS_CAM_SET:
				sprintf(tmpBuf, "LIGHT_POS_CAM_SET camera %d,%d,%c=%d\n",
					inst->light_pos_cam_set.camera,
					inst->light_pos_cam_set.id,
					'x'+inst->light_pos_cam_set.param-11,
					SDL_SwapLE16(inst->light_pos_cam_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT3_POS_CAM_SET:
				sprintf(tmpBuf, "LIGHT3_POS_CAM_SET camera %d,%c=%d\n",
					inst->light3_pos_cam_set.camera,
					'x'+inst->light3_pos_cam_set.param,
					SDL_SwapLE16(inst->light3_pos_cam_set.value));
				strcat(strBuf, tmpBuf);
				break;
			case INST_LIGHT_COLOR_CAM_SET:
				sprintf(tmpBuf, "LIGHT_COLOR_CAM_SET camera %d,%d,r=0x%02x,g=0x%02x,b=0x%02x\n",
					inst->light_color_cam_set.camera,
					inst->light_color_cam_set.id, inst->light_color_cam_set.r,
					inst->light_color_cam_set.g, inst->light_color_cam_set.b);
				strcat(strBuf, tmpBuf);
				break;

			/* 0x80-0x8f */

			case INST_ITEM_ABOVE:
				sprintf(tmpBuf, "ITEM_ABOVE %d\n", inst->item_above.id);
				strcat(strBuf, tmpBuf);
				break;

			default:
				sprintf(tmpBuf, "Unknown opcode 0x%02x\n", inst->opcode);
				strcat(strBuf, tmpBuf);
				break;
		}

		logMsg(1, "0x%08x: %s", offset, strBuf);

		inst_len = scriptDumpGetInstLen(inst->opcode); 
		if (block_ptr) {
			/*logMsg(1, " block 0x%04x inst 0x%04x\n", block_len, inst_len);*/
			int next_len = block_len - inst_len;
			if (inst->opcode == INST_CASE) next_len = block_len;
			if (inst->opcode == INST_BEGIN_LOOP) next_len = block_len - 2;

			scriptDumpBlock(this, (script_inst_t *) block_ptr, offset+inst_len, next_len, indent+1);

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
