/*
	Room description
	RE3 RDT manager

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

#ifndef RDT3_H
#define RDT3_H 1

/*--- Defines ---*/

#define RDT3_OFFSET_COLLISION 6
#define RDT3_OFFSET_CAMERAS	7
#define RDT3_OFFSET_CAM_SWITCHES	8
#define RDT3_OFFSET_TEXT_LANG1	13
#define RDT3_OFFSET_TEXT_LANG2	14
#define RDT3_OFFSET_INIT_SCRIPT	16
#define RDT3_OFFSET_ROOM_SCRIPT	17
#define RDT3_OFFSET_ANIMS	18

/*--- External types ---*/

struct room_s;
struct game_s;

/*--- Types ---*/

typedef struct {
	Uint8	unknown0;
	Uint8	num_cameras;
	Uint8	unknown1[6];
	Uint32	offsets[21];
} rdt3_header_t;

/*--- Functions ---*/

struct room_s *rdt3_room_ctor(struct game_s *this, int num_stage, int num_room);

void rdt3_init(struct room_s *this);

#endif /* RDT3_H */
