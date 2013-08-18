/*
	Room description
	RE2 RDT manager

	Copyright (C) 2009-2013	Patrice Mandin

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

#ifndef RDT2_H
#define RDT2_H 1

/*--- Defines ---*/

#define RDT2_OFFSET_COLLISION 6
#define RDT2_OFFSET_CAMERAS	7
#define RDT2_OFFSET_CAM_SWITCHES	8
#define RDT2_OFFSET_TEXT_LANG1	13
#define RDT2_OFFSET_TEXT_LANG2	14
#define RDT2_OFFSET_INIT_SCRIPT	16
#define RDT2_OFFSET_ROOM_SCRIPT	17
#define RDT2_OFFSET_ANIMS	18

/*--- External types ---*/

typedef struct room_s room_t;
typedef struct game_s game_t;

/*--- Types ---*/

typedef struct {
	Uint8	unknown0;
	Uint8	num_cameras;
	Uint8	unknown1[6];
	Uint32	offsets[21];
} rdt2_header_t;

/*--- Functions ---*/

room_t *rdt2_room_ctor(game_t *this, int num_stage, int num_room);

void rdt2_init(room_t *this);

#endif /* RDT2_H */
