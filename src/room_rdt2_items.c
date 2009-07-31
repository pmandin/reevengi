/*
	Room description
	RE2 RDT items

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

#define ITEM_END_LIST	0x01
#define ITEM_START_LIST	0x02
#define ITEM_06		0x06
#define ITEM_07		0x07
#define ITEM_2C		0x2c
#define ITEM_2D		0x2d
#define ITEM_3A		0x3a
#define ITEM_DOOR	0x3b
#define ITEM_ENEMY	0x44
#define ITEM_46		0x46
#define ITEM_4E		0x4e
#define ITEM_67		0x67

/*--- Types ---*/

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1;
} rdt_item01_t;

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item02_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[3];
} rdt_item06_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1;
} rdt_item07_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint32 unknown0;
	Sint16 x,y,z;
	Sint16 angle;
	Uint16 unknown1[3];
} rdt_item2c_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint32 unknown0[3];
	Sint16 x,y,z;
	Uint32 unknown1[4];
	Uint16 unknown2;
} rdt_item2d_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[3];
	Sint16 x,y,z;
	Uint16 unknown2;
} rdt_item3a_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint32 unknown0;
	Sint16 x,y,z;
	Sint16 angle;
	Sint16 next_x,next_y,next_z;
	Sint16 next_angle;
	Uint8 stage;
	Uint8 room;
	Uint8 camera;
	Uint8 unknown1;
	Uint8 door_type;
	Uint8 door_lock;
	Uint8 unknown2;
	Uint8 door_locked;
	Uint8 door_key;
	Uint8 unknown3;
} rdt_item3b_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint8 number;
	Uint8 emd_id;
	Uint8 state;
	Uint8 unknown1[2];
	Uint8 sounds;
	Uint8 tim_id;
	Uint8 flag;
	Sint16 x,y,z;
	Sint16 angle;
	Uint32 unknown2;
} rdt_item44_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint32 unknown1[3];
} rdt_item46_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint32 unknown1;
	Sint16 x,y,z;
	Sint16 angle;
	Uint16 unknown2[5];
} rdt_item4e_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint32 unknown0;
	Sint16 x,y,z;
	Sint16 angle;
	Sint16 next_x,next_y,next_z;
	Sint16 next_angle;
	Uint16 unknown1[3];
} rdt_item67_t;

typedef union {
	Uint8 type;
	rdt_item01_t	end;
	rdt_item02_t	start;
	rdt_item06_t	item06;
	rdt_item07_t	item07;
	rdt_item2c_t	item2c;
	rdt_item2d_t	item2d;
	rdt_item3a_t	item3a;
	rdt_item3b_t	door;
	rdt_item44_t	enemy;
	rdt_item46_t	item46;
	rdt_item4e_t	item4e;
	rdt_item67_t	item67;
} rdt_item_t;

/*--- Functions ---*/

void room_rdt2_listItems(room_t *this)
{
}
