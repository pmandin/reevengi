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

#include "video.h"
#include "render.h"
#include "room.h"
#include "room_rdt2.h"
#include "log.h"

/*--- Defines ---*/

#define MAP_COLOR_DOOR		0x0000cccc
#define MAP_COLOR_WALLS		0x00ff00ff

#define ITEM_NULL	0x00
#define ITEM_END_LIST	0x01
#define ITEM_START_LIST	0x02
#define ITEM_04		0x04
#define ITEM_06		0x06
#define ITEM_07		0x07
#define ITEM_08		0x08
#define ITEM_0F		0x0f
#define ITEM_21		0x21
#define ITEM_22		0x22
#define ITEM_29		0x29
#define ITEM_2C		0x2c
#define ITEM_2D		0x2d
#define ITEM_2E		0x2e
#define ITEM_32		0x32
#define ITEM_33		0x33
#define ITEM_37		0x37
#define ITEM_3A		0x3a
#define ITEM_DOOR	0x3b
#define ITEM_3D		0x3d
#define ITEM_ENEMY	0x44
#define ITEM_46		0x46
#define ITEM_4B		0x4b
#define ITEM_4E		0x4e
#define ITEM_51		0x51
#define ITEM_54		0x54
#define ITEM_5D		0x5d
#define ITEM_WALLS	0x67
#define ITEM_68		0x68
#define ITEM_6A		0x6a
#define ITEM_6C		0x6c

/* walls.flag */
#define WALLS_NO_ENTER	0xff
#define WALLS_NO_EXIT	0x00

/*--- Types ---*/

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item_null_t;

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item_end_t;

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item_start_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[2];
} rdt_item04_t;

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
	Uint8 unknown;
} rdt_item08_t;

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item0f_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1;
} rdt_item21_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1;
} rdt_item22_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
} rdt_item29_t;

typedef struct {
	Uint8 type;
	Uint8 number;
} rdt_item2c_header_t;

typedef struct {	/* if item2c.number<>0 */
	Uint8 type;
	Uint8 number;
	Uint16 unknown0[2];
	Sint16 x,y,w,h;
	Uint16 unknown1[3];
} rdt_item2c_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint16 unknown0[6];
	Sint16 x,y,z;
	Uint16 unknown1[9];
} rdt_item2d_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1;
} rdt_item2e_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[4];
} rdt_item32_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[3];
} rdt_item33_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1;
} rdt_item37_t;

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
	Sint16 x,y;
	Uint16 w,h;
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
	Uint16 unknown1[6];
} rdt_item3d_t;

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
	Uint16 unknown1[4];
} rdt_item46_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[12];
} rdt_item4b_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[2];
	Sint16 x,y,z;
	Sint16 angle;
	Uint16 unknown2[4];
} rdt_item4e_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[2];
} rdt_item51_t;

typedef struct {
	Uint8 type;
	Uint8 unknown0;
	Uint16 unknown1[3];
} rdt_item54_t;

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item5d_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint16 unknown0[2];
	Sint16 x1,y1;
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
	Uint16 unknown1[2];
	Uint8 unknown2;
	Uint8 flag;
} rdt_item_walls_t;

typedef struct {
	Uint8 type;
	Uint8 number;
	Uint16 unknown0[19];
} rdt_item68_t;

typedef struct {
	Uint8 type;
	Uint8 unknown;
} rdt_item6c_t;

typedef union {
	Uint8 type;
	rdt_item_null_t	null;
	rdt_item_end_t	end;
	rdt_item_start_t	start;
	rdt_item04_t	item04;
	rdt_item06_t	item06;
	rdt_item07_t	item07;
	rdt_item08_t	item08;
	rdt_item0f_t	item0f;
	rdt_item21_t	item21;
	rdt_item22_t	item22;
	rdt_item29_t	item29;
	rdt_item2c_t	item2c;
	rdt_item2d_t	item2d;
	rdt_item2e_t	item2e;
	rdt_item32_t	item32;
	rdt_item33_t	item33;
	rdt_item37_t	item37;
	rdt_item3a_t	item3a;
	rdt_item_door_t	door;
	rdt_item3d_t	item3d;
	rdt_item_enemy_t	enemy;
	rdt_item46_t	item46;
	rdt_item4b_t	item4b;
	rdt_item4e_t	item4e;
	rdt_item51_t	item51;
	rdt_item54_t	item54;
	rdt_item5d_t	item5d;
	rdt_item_walls_t	walls;
	rdt_item68_t	item68;
	rdt_item6c_t	item6c;
} rdt_item_t;

/*--- Functions prototypes ---*/

static void room_rdt2_drawItem2c(rdt_item2c_t *item);
static void room_rdt2_drawDoor(rdt_item_door_t *item);
static void room_rdt2_drawWalls(rdt_item_walls_t *item);

/*--- Functions ---*/

void room_rdt2_items_init(room_t *this)
{
	/*this->checkWalls = room_rdt2_checkWalls;*/
}

void room_rdt2_listItems(room_t *this)
{
	rdt_item_t *item;
	Uint32 *item_offset, offset;
	int end_list=0;

	item_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+16*4]);
	offset = SDL_SwapLE32(*item_offset);

	logMsg(1, "Listing items from offset 0x%08x\n", offset);

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
			case ITEM_04:
				logMsg(2, " Item 0x04\n");
				item_length = sizeof(rdt_item04_t);
				break;
			case ITEM_06:
				logMsg(2, " Item 0x06\n");
				item_length = sizeof(rdt_item06_t);
				break;
			case ITEM_07:
				logMsg(2, " Item 0x07\n");
				item_length = sizeof(rdt_item07_t);
				break;
			case ITEM_08:
				logMsg(2, " Item 0x08\n");
				item_length = sizeof(rdt_item08_t);
				break;
			case ITEM_0F:
				logMsg(2, " Item 0x0f\n");
				item_length = sizeof(rdt_item0f_t);
				break;
			case ITEM_21:
				logMsg(2, " Item 0x21\n");
				item_length = sizeof(rdt_item21_t);
				break;
			case ITEM_22:
				logMsg(2, " Item 0x22\n");
				item_length = sizeof(rdt_item22_t);
				break;
			case ITEM_29:
				logMsg(2, " Item 0x29\n");
				item_length = sizeof(rdt_item29_t);
				break;
			case ITEM_2C:
				{
					rdt_item2c_t *item2c = (rdt_item2c_t *) item;

					logMsg(2, " Item 0x2c\n");
					item_length = sizeof(rdt_item2c_header_t);
					if (item2c->number != 0) {
						item_length = sizeof(rdt_item2c_t);
					}
				}
				break;
			case ITEM_2D:
				logMsg(2, " Item 0x2d\n");
				item_length = sizeof(rdt_item2d_t);
				break;
			case ITEM_2E:
				logMsg(2, " Item 0x2e\n");
				item_length = sizeof(rdt_item2e_t);
				break;
			case ITEM_32:
				logMsg(2, " Item 0x32\n");
				item_length = sizeof(rdt_item32_t);
				break;
			case ITEM_33:
				logMsg(2, " Item 0x33\n");
				item_length = sizeof(rdt_item33_t);
				break;
			case ITEM_37:
				logMsg(2, " Item 0x37\n");
				item_length = sizeof(rdt_item37_t);
				break;
			case ITEM_3A:
				logMsg(2, " Item 0x3a\n");
				item_length = sizeof(rdt_item3a_t);
				break;
			case ITEM_DOOR:
				logMsg(2, " Door %d\n", item->door.number);
				item_length = sizeof(rdt_item_door_t);
				break;
			case ITEM_3D:
				logMsg(2, " Item 0x3d\n");
				item_length = sizeof(rdt_item3d_t);
				break;
			case ITEM_ENEMY:
				logMsg(2, " Enemy %d\n", item->enemy.number);
				item_length = sizeof(rdt_item_enemy_t);
				break;
			case ITEM_46:
				logMsg(2, " Item 0x46\n");
				item_length = sizeof(rdt_item46_t);
				break;
			case ITEM_4B:
				logMsg(2, " Item 0x4b\n");
				item_length = sizeof(rdt_item4b_t);
				break;
			case ITEM_4E:
				logMsg(2, " Item 0x4e\n");
				item_length = sizeof(rdt_item4e_t);
				break;
			case ITEM_51:
				logMsg(2, " Item 0x51\n");
				item_length = sizeof(rdt_item51_t);
				break;
			case ITEM_54:
				logMsg(2, " Item 0x54\n");
				item_length = sizeof(rdt_item54_t);
				break;
			case ITEM_5D:
				logMsg(2, " Item 0x5d\n");
				item_length = sizeof(rdt_item5d_t);
				break;
			case ITEM_WALLS:
				logMsg(2, " Walls %d\n", item->walls.number);
				item_length = sizeof(rdt_item_walls_t);
				break;
			case ITEM_68:
				logMsg(2, " Item 0x68\n");
				item_length = sizeof(rdt_item68_t);
				break;
			case ITEM_6C:
				logMsg(2, " Item 0x6c\n");
				item_length = sizeof(rdt_item6c_t);
				break;
			default:
				logMsg(2, " Unknown item 0x%02x\n", item->type);
				end_list=1;
				break;
		}

		offset += item_length;
	}
}

void room_rdt2_drawItems(room_t *this)
{
	rdt_item_t *item;
	Uint32 *item_offset, offset;
	int end_list=0;

	if (!this) {
		return;
	}

	item_offset = (Uint32 *) ( &((Uint8 *) this->file)[8+16*4]);
	offset = SDL_SwapLE32(*item_offset);

	while (!end_list) {
		Uint32 item_length = 0;
		item = (rdt_item_t *) &((Uint8 *) this->file)[offset];

		switch(item->type) {
			case ITEM_NULL:
				item_length = sizeof(rdt_item_null_t);
				break;
			case ITEM_END_LIST:
				end_list=1;
				item_length = sizeof(rdt_item_end_t);
				break;
			case ITEM_START_LIST:
				item_length = sizeof(rdt_item_start_t);
				break;
			case ITEM_04:
				item_length = sizeof(rdt_item04_t);
				break;
			case ITEM_06:
				item_length = sizeof(rdt_item06_t);
				break;
			case ITEM_07:
				item_length = sizeof(rdt_item07_t);
				break;
			case ITEM_08:
				item_length = sizeof(rdt_item08_t);
				break;
			case ITEM_0F:
				item_length = sizeof(rdt_item0f_t);
				break;
			case ITEM_21:
				item_length = sizeof(rdt_item21_t);
				break;
			case ITEM_22:
				item_length = sizeof(rdt_item22_t);
				break;
			case ITEM_29:
				item_length = sizeof(rdt_item29_t);
				break;
			case ITEM_2C:
				{
					rdt_item2c_t *item2c = (rdt_item2c_t *) item;

					item_length = sizeof(rdt_item2c_header_t);
					if (item2c->number != 0) {
						/*room_rdt2_drawItem2c((rdt_item2c_t *) item);*/
						item_length = sizeof(rdt_item2c_t);
					}
				}
				break;
			case ITEM_2D:
				item_length = sizeof(rdt_item2d_t);
				break;
			case ITEM_2E:
				item_length = sizeof(rdt_item2e_t);
				break;
			case ITEM_32:
				item_length = sizeof(rdt_item32_t);
				break;
			case ITEM_33:
				item_length = sizeof(rdt_item33_t);
				break;
			case ITEM_37:
				item_length = sizeof(rdt_item37_t);
				break;
			case ITEM_3A:
				item_length = sizeof(rdt_item3a_t);
				break;
			case ITEM_DOOR:
				room_rdt2_drawDoor((rdt_item_door_t *) item);
				item_length = sizeof(rdt_item_door_t);
				break;
			case ITEM_3D:
				item_length = sizeof(rdt_item3d_t);
				break;
			case ITEM_ENEMY:
				item_length = sizeof(rdt_item_enemy_t);
				break;
			case ITEM_46:
				item_length = sizeof(rdt_item46_t);
				break;
			case ITEM_4B:
				item_length = sizeof(rdt_item4b_t);
				break;
			case ITEM_4E:
				item_length = sizeof(rdt_item4e_t);
				break;
			case ITEM_51:
				item_length = sizeof(rdt_item51_t);
				break;
			case ITEM_54:
				item_length = sizeof(rdt_item54_t);
				break;
			case ITEM_5D:
				item_length = sizeof(rdt_item5d_t);
				break;
			case ITEM_WALLS:
				room_rdt2_drawWalls((rdt_item_walls_t *) item);
				item_length = sizeof(rdt_item_walls_t);
				break;
			case ITEM_68:
				item_length = sizeof(rdt_item68_t);
				break;
			case ITEM_6C:
				item_length = sizeof(rdt_item6c_t);
				break;
			default:
				end_list=1;
				break;
		}

		offset += item_length;
	}
}

static void room_rdt2_drawItem2c(rdt_item2c_t *item)
{
	vertex_t v[4];
	Sint16 x = SDL_SwapLE16(item->x);
	Sint16 y = SDL_SwapLE16(item->y);
	Sint16 w = SDL_SwapLE16(item->w);
	Sint16 h = SDL_SwapLE16(item->h);

	/*printf("item %d: %04x,%04x %04x,%04x\n",item->number,x,y,w,h);*/

	v[0].x = x * 0.5f;
	v[0].y = y * 0.5f;
	v[0].z = 1.0f;

	v[1].x = (x+w) * 0.5f;
	v[1].y = y * 0.5f;
	v[1].z = 1.0f;

	v[2].x = (x+w) * 0.5f;
	v[2].y = (y+h) * 0.5f;
	v[2].z = 1.0f;

	v[3].x = x * 0.5f;
	v[3].y = (y+h) * 0.5f;
	v[3].z = 1.0f;

	render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
}

static void room_rdt2_drawDoor(rdt_item_door_t *item)
{
	vertex_t v[4];
	Sint16 x = SDL_SwapLE16(item->x);
	Sint16 y = SDL_SwapLE16(item->y);
	Sint16 w = SDL_SwapLE16(item->w);
	Sint16 h = SDL_SwapLE16(item->h);

	/*printf("door %d: %d,%d %d,%d\n",item->number,x,y,w,h);*/

	render.set_color(MAP_COLOR_DOOR);

	v[0].x = x * 0.5f;
	v[0].y = y * 0.5f;
	v[0].z = 1;

	v[1].x = (x+w) * 0.5f;
	v[1].y = y * 0.5f;
	v[1].z = 1;

	render.line(&v[0], &v[1]);

	v[0].x = (x+w) * 0.5f;
	v[0].y = (y+h) * 0.5f;
	v[0].z = 1;

	render.line(&v[0], &v[1]);

	v[1].x = x * 0.5f;
	v[1].y = (y+h) * 0.5f;
	v[1].z = 1;

	render.line(&v[0], &v[1]);

	v[0].x = x * 0.5f;
	v[0].y = y * 0.5f;
	v[0].z = 1;

	render.line(&v[0], &v[1]);

#if 0
	v[0].x = x * 0.5f;
	v[0].y = y * 0.5f;
	v[0].z = 1;

	v[1].x = (x+w) * 0.5f;
	v[1].y = y * 0.5f;
	v[1].z = 1;

	v[2].x = (x+w) * 0.5f;
	v[2].y = (y+h) * 0.5f;
	v[2].z = 1;

	v[3].x = x * 0.5f;
	v[3].y = (y+h) * 0.5f;
	v[3].z = 1;

	render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
#endif
}

static void room_rdt2_drawWalls(rdt_item_walls_t *item)
{
	vertex_t v[4];

	render.set_color(MAP_COLOR_WALLS);

	/*printf("walls %d: %d,%d %d,%d %d,%d %d,%d\n",item->number,
		SDL_SwapLE16(item->x1),SDL_SwapLE16(item->y1),
		SDL_SwapLE16(item->x2),SDL_SwapLE16(item->y2),
		SDL_SwapLE16(item->x3),SDL_SwapLE16(item->y3),
		SDL_SwapLE16(item->x4),SDL_SwapLE16(item->y4));*/

	v[0].x = SDL_SwapLE16(item->x1) * 0.5f;
	v[0].y = SDL_SwapLE16(item->y1) * 0.5f;
	v[0].z = 1;

	v[1].x = SDL_SwapLE16(item->x2) * 0.5f;
	v[1].y = SDL_SwapLE16(item->y2) * 0.5f;
	v[1].z = 1;

	v[2].x = SDL_SwapLE16(item->x3) * 0.5f;
	v[2].y = SDL_SwapLE16(item->y3) * 0.5f;
	v[2].z = 1;

	v[3].x = SDL_SwapLE16(item->x4) * 0.5f;
	v[3].y = SDL_SwapLE16(item->y4) * 0.5f;
	v[3].z = 1;

	render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
}
