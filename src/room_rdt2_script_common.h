/*
	Room description
	RE2 RDT script instructions

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

#ifndef ROOM_RDT2_SCRIPT_COMMON_H
#define ROOM_RDT2_SCRIPT_COMMON_H 1

/*--- Defines ---*/

/* Instructions */

/* 0x00-0x0f */
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
#define INST_BEGIN_WHILE	0x0f

/* 0x10-0x1f */
#define INST_END_WHILE	0x10
#define INST_DO		0x11
#define INST_WHILE	0x12
#define INST_BEGIN_SWITCH	0x13
#define INST_CASE	0x14
#define INST_DEFAULT	0x15
#define INST_END_SWITCH	0x16
#define INST_GOTO	0x17
#define INST_FUNC	0x18
#define INST_BREAK	0x1a
#define INST_NOP1C	0x1c
#define INST_CHG_SCRIPT	0x1d
#define INST_NOP1E	0x1e
#define INST_NOP1F	0x1f

/* 0x20-0x2f */
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
#define INST_ESPR3D_SET	0x2c
#define INST_TRIGGER_SET	0x2d
#define INST_SET_REG_MEM	0x2e	/* 0x2e 01 00 : reenable control+animation in room1010 */
#define INST_SET_REG_IMM	0x2f

/* 0x30-0x3f */
#define INST_SET_REG_TMP	0x30
#define INST_ADD_REG	0x31
#define INST_EM_SET_POS	0x32
#define INST_SET_REG3	0x33
#define INST_EM_SET_VAR	0x34
#define INST_EM_SET_VAR_VARW	0x35
#define INST_CAM_CHG	0x37
#define INST_DOOR_SET	0x3b
#define INST_STATUS_SET	0x3c
#define INST_EM_GET_VAR_VARW	0x3d
#define INST_CMP_IMM	0x3e

/* 0x40-0x4f */
#define INST_STATUS_SHOW	0x42
#define INST_EM_SET	0x44
#define INST_ACTIVATE_OBJECT	0x47
#define INST_CAMSWITCH_SWAP	0x4b
#define INST_ITEM_SET	0x4e

/* 0x50-0x5f */
#define INST_SND_SET	0x51
#define INST_SND_PLAY	0x59
#define INST_ITEM_HAVE	0x5e

/* 0x60-0x6f */
#define INST_ITEM_REMOVE	0x62
#define INST_NOP63	0x63
#define INST_WALL_SET	0x67
#define INST_LIGHT_POS_SET	0x6a
#define INST_LIGHT_RANGE_SET	0x6b
#define INST_BG_YPOS_SET	0x6d
#define INST_MOVIE_PLAY	0x6f

/* 0x70-0x7f */
#define INST_ITEM_ADD	0x76
#define INST_LIGHT_COLOR_SET	0x7c
#define INST_LIGHT_POS_CAM_SET	0x7d
#define INST_LIGHT_RANGE_CAM_SET	0x7e
#define INST_LIGHT_COLOR_CAM_SET	0x7f

/* 0x80-0x8e */
#define INST_POISON_CHECK	0x86
#define INST_POISON_CLEAR	0x87
#define INST_ITEM_HAVE_AND_REMOVE	0x88
#define INST_NOP8A	0x8a
#define INST_NOP8B	0x8b
#define INST_NOP8C	0x8c

/*--- Types ---*/

/* 0x00-0x0f */

typedef struct {
	Uint8 opcode;
	Uint8 ret_value;
} script_return_t;

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
	Uint8 dummy;
	Uint16 block_length;
} script_if_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
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
	Uint8 dummy;
	Uint16 block_length;
} script_begin_while_t;

/* 0x10-0x1f */

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 block_length;
} script_do_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
} script_while_t;

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
	Uint8 unknown[3];
	Sint16 rel_offset;
} script_goto_t;

typedef struct {
	Uint8 opcode;
	Uint8 num_func;
} script_func_t;

typedef struct {
	Uint8 opcode;
	Uint8 varw;
	Uint8 offset;
	Uint8 flag; /* 0 byte, 1 word */
} script_chg_script_t;

/* 0x20-0x2f */

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
	Uint8 dummy;
	Uint8 varw;
	Uint8 compare;
	Uint16 value;
} script_cmp_varw_t;

typedef struct {
	Uint8 opcode;
	Uint8 varw;
	Uint16 value;
} script_set_varw_t;

typedef struct {
	Uint8 opcode;
	Uint8 dst;
	Uint8 src;
} script_copy_varw_t;

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
	Uint8 id;
} script_cam_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 id;
	Uint8 unknown1[3];
} script_print_text_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 type;	/* ptr to this stored in room object list */
	Uint8 unknown0[3];
	Sint16 x,y,w,h;
	Uint16 inst[3];	/* Instructions to execute. 0:examine 1:activate 2:? or 0xff */
} script_espr3d_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown[18];
} script_trigger_set_t;

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

/* 0x30-0x3f */

typedef struct {
	Uint8 opcode;
	Uint8 unknown;
	Sint16 value[3];
} script_setreg3w_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 value;
} script_set_var_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 varw;
} script_em_set_var_varw_t;

/*
[    0.484] 0x0000007e: 0x3d 0x10 0x07
[    0.484] 0x00000088: 0x35 0x07 0x10
*/

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 camera;
	Uint8 unknown1;
} script_cam_chg_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy0;
	Uint8 anim[4];	/* Anim ID, same as inst 4c */
	Uint8 dummy1;
	Uint8 w,h;	/* Width,height of animation */
	Uint8 unknown[7];
} script_inst3a_t;

/* inst 3a,4c
0x0d 0x00 0x00 0x00: other flame, crash
0x0e 0x00 0x00 0x00: big flame
0x04 0x12 0x00 0x00: smoke
*/

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
	Uint8 screen;
} script_status_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 varw;
	Uint8 id;
} script_em_get_var_varw_t;

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 unknown1;
	Uint8 compare;
	Sint16 value;
} script_cmp_imm_t;

/* 0x40-0x4f */

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint8 unknown0[2];
	Uint16 unknown1[2];
} script_inst40_t;

/*
[    3.724] 0x00000420: 0x40 0x00 0x04 0x21 0xf4 0xc3 0x01 0xc9
[    3.724] 0x0000042a: 0x40 0x00 0x09 0x21 0xd3 0xd5 0x92 0xc8
[    3.725] 0x00000464: 0x40 0x00 0x09 0x21 0x07 0xc1 0x3f 0xce
[    3.747] 0x00000a5c: 0x40 0x00 0x09 0x20 0x92 0xa3 0x0b 0xc8
[    3.757] 0x00000d02: 0x40 0x00 0x04 0x21 0xdc 0xa4 0x3c 0xa6
[    3.757] 0x00000d1c: 0x40 0x00 0x04 0x21 0x9c 0xc1 0xaa 0xbc
[    3.757] 0x00000d2a: 0x40 0x00 0x04 0x21 0x6c 0xc2 0xb8 0xc5
[    3.758] 0x00000d46: 0x40 0x00 0x09 0x20 0xdc 0xc1 0xc6 0xbe
[    3.758] 0x00000d50: 0x40 0x00 0x07 0x20 0x97 0xc0 0x3d 0xcf
[    3.759] 0x00000d8a: 0x40 0x00 0x09 0x21 0x00 0x00 0x00 0x00
[    3.760] 0x00000e3e: 0x40 0x00 0x15 0x21 0xb8 0x01 0x40 0x00
*/

typedef struct {
	Uint8 opcode;
	Uint8 unknown0;
	Uint8 id;
	Uint8 model;

	Uint16 pose;
	Uint8 unknown1;
	Uint8 sound_bank;

	Uint8 texture;
	Uint8 killed;
	Sint16 x,y,z,dir;

	Uint16 unknown2[2];
} script_em_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 unknown0[2];
	Uint16 unknown1[3];	/* instructions to execute? 3*2 or 1*6 ? */
} script_inst46_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_set_cur_obj_t;

typedef struct {
	Uint8 opcode;
	Uint8 cam[2];
} script_camswitch_swap_t;

typedef struct {
	Uint8 opcode;
	Uint8 anim[4];
} script_inst4c_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown0[2];	/* ptr to this stored in room object list */
	Sint16 x,y,w,h;
	Uint16 type;
	Uint16 amount;
	Uint16 unknown1[2];
} script_item_set_t;

/* 0x50-0x5f */

typedef struct {
	Uint8 opcode;
	Uint8 unknown[5];
} script_snd_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 value;
} script_snd_play_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_item_have_t;

/* 0x60-0x6f */

typedef struct {
	Uint8 opcode;
	Uint8 entity_type; /* 1:em_set, 2:0x89, 3:0x2e, other:script data */
	Uint8 unknown0;	/* for entity_type=3 */
	Uint8 unknown1[3];
	Uint16 unknown2[4];
} script_inst60_t;

/*
[    0.392] 0x00000410: 0x60 0x03 0x00 0xbf 0xbf 0xbf 0x90 0x01 0x58 0x02 0x00 0x00 0x00 0x00
[    0.393] 0x00000458: 0x60 0x03 0x01 0xbf 0xbf 0xbf 0x90 0x01 0x58 0x02 0x00 0x00 0x00 0x00
[    0.393] 0x000004a0: 0x60 0x03 0x02 0xbf 0xbf 0xbf 0x90 0x01 0x58 0x02 0x00 0x00 0x00 0x00
*/

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_item_remove_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown0[2];	/* ptr to this stored in room object list */
	Sint16 xycoords[4*2];
	Uint16 unknown1[3];
} script_wall_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint8 id;
	Uint8 param;
	Sint16 value;
} script_light_pos_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 range;
} script_light_range_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 y;
} script_bg_ypos_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
} script_movie_play_t;

/* 0x70-0x7f */

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 amount;
} script_item_add_t;

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
	Uint8 id;
	Uint16 range;
} script_light_range_cam_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 camera;
	Uint8 id;
	Uint8 r,g,b;
} script_light_color_cam_set_t;

/* 0x80-0x8e */

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 unknown;
} script_item_have_and_remove_t;

/* All instructions */

typedef union {
	Uint8 opcode;

	/* 0x00-0x0f */
	script_return_t		i_return;
	script_reset_t		reset;
	script_evtexec_t	evtexec;
	script_if_t		i_if;
	script_else_t		i_else;
	script_sleep_init_t	sleep_init;
	script_sleep_loop_t	sleep_loop;
	script_loop_t		loop;
	script_begin_while_t	begin_while;

	/* 0x10-0x1f */
	script_do_t		i_do;
	script_while_t		i_while;
	script_switch_t		i_switch;
	script_case_t		i_case;
	script_goto_t		i_goto;
	script_func_t		func;
	script_chg_script_t	chg_script;

	/* 0x20-0x2f */
	script_bittest_t	bittest;
	script_bitchg_t		bitchg;
	script_cmp_varw_t	cmp_varw;
	script_set_varw_t	set_varw;
	script_copy_varw_t	copy_varw;
	script_op_varw_imm_t	op_varw_imm;
	script_op_varw_t	op_varw;
	script_cam_set_t	cam_set;
	script_print_text_t	print_text;
	script_espr3d_set_t	espr3d_set;
	script_trigger_set_t	trigger_set;
	script_setregmem_t	set_reg_mem;
	script_setregimm_t	set_reg_imm;

	/* 0x30-0x3f */
	script_setreg3w_t	set_reg_3w;
	script_set_var_t	set_var;
	script_em_get_var_varw_t	em_get_var_varw;
	script_cam_chg_t	cam_chg;
	script_inst3a_t		inst3a;
	script_door_set_t	door_set;
	script_status_set_t	status_set;
	script_em_set_var_varw_t	em_set_var_varw;
	script_cmp_imm_t	cmp_imm;

	/* 0x40-0x4f */
	script_em_set_t		em_set;
	script_inst46_t		inst46;
	script_set_cur_obj_t	set_cur_obj;
	script_camswitch_swap_t	camswitch_swap;
	script_item_set_t	item_set;

	/* 0x50-0x5f */
	script_snd_set_t	snd_set;
	script_snd_play_t	snd_play;
	script_item_have_t	item_have;

	/* 0x60-0x6f */
	script_item_remove_t	item_remove;
	script_wall_set_t	wall_set;
	script_light_pos_set_t	light_pos_set;
	script_light_range_set_t	light_range_set;
	script_bg_ypos_set_t	bg_ypos_set;
	script_movie_play_t	movie_play;

	/* 0x70-0x7f */
	script_item_add_t	item_add;
	script_light_color_set_t	light_color_set;
	script_light_pos_cam_set_t	light_pos_cam_set;
	script_light_range_cam_set_t	light_range_cam_set;
	script_light_color_cam_set_t	light_color_cam_set;

	/* 0x80-0x8e */
	script_item_have_and_remove_t	item_have_and_remove;

} script_inst_t;

#endif /* ROOM_RDT2_SCRIPT_COMMON_H */
