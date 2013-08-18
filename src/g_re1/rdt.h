/*
	Room description
	RE1 RDT manager

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

#ifndef RDT_H
#define RDT_H 1

/*--- Defines ---*/

#define RDT1_OFFSET_CAM_SWITCHES	0
#define RDT1_OFFSET_COLLISION		1
#define RDT1_OFFSET_INIT_SCRIPT		6
#define RDT1_OFFSET_ROOM_SCRIPT		7
#define RDT1_OFFSET_EVENTS		8
#define RDT1_OFFSET_TEXT		11

/*--- External types ---*/

typedef struct room_s room_t;
typedef struct game_s game_t;

/*--- Types ---*/

typedef struct {
	Sint32	x,y,z;
	Uint32	unknown[2];
} rdt1_header_part_t;

typedef struct {
	Uint8	unknown0;
	Uint8	num_cameras;
	Uint8	unknown1[4];
	Uint16	unknown2[3];
	rdt1_header_part_t	unknown3[3];
	Uint32	offsets[19];
} rdt1_header_t;

/*--- Functions ---*/

room_t *rdt1_room_ctor(game_t *this, int num_stage, int num_room);

void rdt1_init(room_t *this);

#endif /* RDT_H */
