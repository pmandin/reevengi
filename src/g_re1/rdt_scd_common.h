/*
	Room description
	RE1 RDT script instructions

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

#ifndef ROOM_RDT_SCRIPT_COMMON_H
#define ROOM_RDT_SCRIPT_COMMON_H 1

/*--- Defines ---*/

/* 0x00-0x0f */
#define INST_NOP	0x00
#define INST_IF		0x01
#define INST_ELSE	0x02
#define INST_END_IF	0x03
#define INST_BIT_TEST	0x04
#define INST_BIT_OP	0x05
#define INST_CMP06	0x06
#define INST_CMP07	0x07
#define INST_STAGEROOMCAM_SET	0x08
#define INST_PRINT_MSG	0x0b
#define INST_DOOR_SET	0x0c
#define INST_ITEM_SET	0x0d
#define INST_NOP0E	0x0e

/* 0x10-0x1f */
#define INST_CMP10	0x10
#define INST_CMP11	0x11
#define INST_ITEM_ATTR_SET	0x12
#define INST_ITEM_ATTR2_SET	0x13
#define INST_ITEM_MODEL_SET	0x18
#define INST_EM_SET	0x1b
#define INST_OM_SET	0x1f

/* 0x20-0x2f */
#define INST_PLAYER_POS_SET	0x20
#define INST_EM_POS_SET		0x21

/* 0x30-0x3f */
#define INST_37		0x37

/* 0x40-0x4f */
/* 0x50 */


/*
dexit
dret
dsleep
next
if8
if16
sfor
efor
dtask_init
dtask_kill
sv_work8
add_work8
sv_work16
add_work16
fade_in
fade_out
model_set
cam_pset
cam_aset
cam_add
cam_add2
pos_pset
pos_aset
pos_add
pos_add2
dir_pset
dir_aset
dir_add
dir_add2
vert_set
vert_add
mess_disp	0x0b?
call_se
attri2_set
color_set
packed_add
demo_sleep
demo_stfoff

0x0b mess_disp ?
0x14 event?
0x17 volume_set, pan_set

room1070 # 0x1b 0x00 0x03 0x91 0x01 0x02 0x00 0x00 0x2d 0x00 0x00 0x00 0x06 0x45 0x00 0x00 0xd2 0x34 0x00 0x00 0x00 0x00
room1080 # 0x1b 0x02 0x80 0x00 0x01 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x30 0x75 0x30 0x75 0x30 0x75 0x00 0x00 0x00 0x00
room1080 # 0x1b 0x02 0x01 0x00 0x01 0x02 0x00 0x00 0x00 0x08 0x00 0x00 0x10 0x27 0x00 0x00 0x10 0x0e 0x00 0x00 0x00 0x00
room1080 # 0x1b 0x02 0x80 0x01 0x01 0x02 0x00 0x00 0xe0 0x07 0x00 0x00 0xf0 0x6e 0x48 0xf4 0x04 0x42 0x01 0x00 0x00 0x00
room1080 # 0x1b 0x02 0x01 0x01 0x01 0x02 0x00 0x00 0x00 0x0c 0x00 0x00 0xc0 0x5d 0x00 0x00 0xe8 0x3a 0x01 0x00 0x00 0x00
room10a0 # 0x1b 0x11 0x00 0x71 0x01 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x50 0x20 0x00 0x00 0x81 0x09 0x00 0x00 0x00 0x00
room10a0 # 0x1b 0x11 0x00 0x72 0x01 0x02 0x00 0x00 0x07 0x04 0x00 0x00 0xcf 0x0b 0x00 0x00 0xd0 0x1a 0x01 0x00 0x00 0x00
*/


/*--- Types ---*/

/* 0x00-0x0f */

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
} script_if_t;

typedef struct {
	Uint8 opcode;
	Uint8 block_length;
} script_else_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
} script_endif_t;

typedef struct {
	Uint8 opcode;
	Uint8 flag;
	Uint8 object;
	Uint8 value;
} script_bit_test_t;

typedef struct {
	Uint8 opcode;
	Uint8 object; /* 0 stage, 1 room, 2 camera */
	Uint16 value;
} script_stageroomcam_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown;
} script_printmsg_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 x,y,w,h;
	Uint8 unknown0[2];
	Uint8 anim;
	Uint8 unknown1[2];
	Uint8 next_stage_and_room;	/* bits 7-5: stage, 4-0: room */
	Sint16 next_x,next_y,next_z;
	Sint16 next_dir;
	Uint16 unknown2;
} script_door_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Sint16 x,y,w,h;
	Uint8 type;	/* 0x10: typewriter */
	Uint8 unknown[7];
} script_item_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
} script_nop0e_t;

/* 0x10-0x1f */

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 unknown0[2];
	Uint16 unknown1[3];
} script_item_attr_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint8 unknown[2];
} script_item_attr2_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown[12];
} script_item_model_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 model;
	Uint8 unknown0;
	Uint8 killed;
	Uint8 unknown1[2];
	Uint16 unknown2;
	Uint16 a;
	Uint16 unknown3;
	Uint16 x;
	Uint16 y;
	Uint16 z;
	Uint8 id;
	Uint8 unknown4[3];
} script_em_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 unknown[13];
} script_om_set_t;

/* 0x20-0x2f */

typedef struct {
	Uint8 opcode;
	Uint8 dummy;
	Uint16 unknown0;
	Uint16 a;
	Uint16 unknown1;
	Uint16 x,y,z;
} script_player_pos_set_t;

typedef struct {
	Uint8 opcode;
	Uint8 id;
	Uint16 dummy;
	Uint16 a;
	Uint16 unknown;
	Uint16 x,y,z;
} script_em_pos_set_t;

/* 0x30-0x3f */

typedef struct {
	Uint8 opcode;	/* 0x37 */
	Uint8 row;	/* row,col in 7*32 byte array */
	Uint8 col;
	Uint8 value;
} script_inst37_t;

/* 0x40-0x4f */
/* 0x50 */

/* All instructions */

typedef union {
	Uint8 opcode;

	/* 0x00-0x0f */
	script_if_t	i_if;
	script_else_t	i_else;
	script_endif_t	i_endif;
	script_bit_test_t	bit_test;
	script_stageroomcam_set_t	stageroomcam_set;
	script_printmsg_t	print_msg;	/* invalid */
	script_door_set_t	door_set;
	script_item_set_t	item_set;
	script_nop0e_t	nop0e;

	/* 0x10-0x1f */
	script_item_attr_set_t	item_attr_set;
	script_item_attr2_set_t	item_attr2_set;
	script_item_model_set_t	item_model_set;
	script_em_set_t	em_set;
	script_om_set_t	om_set;

	/* 0x20-0x2f */
	script_player_pos_set_t	player_pos_set;
	script_em_pos_set_t em_pos_set;

	/* 0x30-0x3f */
	script_inst37_t	inst37;
	
	/* 0x40-0x4f */
	/* 0x50 */

} script_inst_t;

#endif /* ROOM_RDT_SCRIPT_COMMON_H */
