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
#include "log.h"

/*--- Defines ---*/

#define ITEM_NULL	0x00
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
	Uint8 unknown;
} rdt_item_null_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1;
} rdt_item_end_t;

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item_start_t;

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
	Uint16 unknown0[2];
	Sint16 x,y,z;
	Sint16 angle;
	Uint16 unknown1[3];
} rdt_item2c_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint16 unknown0[6];
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
	Uint16 unknown0[2];
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
} rdt_item_door_t;

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
	Uint16 unknown2[2];
} rdt_item_enemy_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[6];
} rdt_item46_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[2];
	Sint16 x,y,z;
	Sint16 angle;
	Uint16 unknown2[5];
} rdt_item4e_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint16 unknown0[2];
	Sint16 x,y,z;
	Sint16 angle;
	Sint16 next_x,next_y,next_z;
	Sint16 next_angle;
	Uint16 unknown1[3];
} rdt_item67_t;

typedef union {
	Uint8 type;
	rdt_item_null_t	null;
	rdt_item_end_t	end;
	rdt_item_start_t	start;
	rdt_item06_t	item06;
	rdt_item07_t	item07;
	rdt_item2c_t	item2c;
	rdt_item2d_t	item2d;
	rdt_item3a_t	item3a;
	rdt_item_door_t	door;
	rdt_item_enemy_t	enemy;
	rdt_item46_t	item46;
	rdt_item4e_t	item4e;
	rdt_item67_t	item67;
} rdt_item_t;

/*--- Functions ---*/

void room_rdt2_listItems(room_t *this)
{
	rdt_item_t *item;
	Uint32 *item_offset, offset;
	int end_list=0;
	
	item_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+16*4]);
	offset = SDL_SwapLE32(*item_offset);

	logMsg(2, "Listing items from offset 0x%08x\n", offset);

	while (!end_list) {
		Uint32 item_length = 0;

		/*printf(" offset 0x%08x\n", offset);*/
		item = (rdt_item_t *) &((Uint8 *) this->file)[offset];

		switch(item->type) {
			case ITEM_NULL:
				logMsg(2, " Null item\n");
				item_length = sizeof(rdt_item_null_t);
				break;
			case ITEM_END_LIST:
				logMsg(2, " End of item list\n");
				end_list=1;
				item_length = sizeof(rdt_item_end_t);
				break;
			case ITEM_START_LIST:
				logMsg(2, " Start of item list\n");
				item_length = sizeof(rdt_item_start_t);
				break;
			case ITEM_06:
				logMsg(2, " Item 0x06\n");
				item_length = sizeof(rdt_item06_t);
				break;
			case ITEM_07:
				logMsg(2, " Item 0x07\n");
				item_length = sizeof(rdt_item07_t);
				break;
			case ITEM_2C:
				logMsg(2, " Item 0x2c\n");
				item_length = sizeof(rdt_item2c_t);
				break;
			case ITEM_2D:
				logMsg(2, " Item 0x2d\n");
				item_length = sizeof(rdt_item2d_t);
				break;
			case ITEM_3A:
				logMsg(2, " Item 0x3a\n");
				item_length = sizeof(rdt_item3a_t);
				break;
			case ITEM_DOOR:
				logMsg(2, " Door %d\n", item->door.number);
				item_length = sizeof(rdt_item_door_t);
				break;
			case ITEM_ENEMY:
				logMsg(2, " Enemy %d\n", item->enemy.number);
				item_length = sizeof(rdt_item_enemy_t);
				break;
			case ITEM_46:
				logMsg(2, " Item 0x46\n");
				item_length = sizeof(rdt_item46_t);
				break;
			case ITEM_4E:
				logMsg(2, " Item 0x4e\n");
				item_length = sizeof(rdt_item4e_t);
				break;
			case ITEM_67:
				logMsg(2, " Item 0x67\n");
				item_length = sizeof(rdt_item67_t);
				break;
			default:
				logMsg(2, " Unknown item 0x%02x\n", item->type);
				end_list=1;
				break;
		}

		offset += item_length;
	}
}
