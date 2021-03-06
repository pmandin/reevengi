/*
	RE2 RVD
	Camera switches

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

#ifndef RDT2_RVD_H
#define RDT2_RVD_H 1

/*--- Defines ---*/

/*--- External types ---*/

struct room_s;
struct room_camswitch_s;

/*--- Types ---*/

typedef struct {
	Uint16 const0; /* 0xff01 */
	Uint8 from,to;
	Sint16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
} rdt2_rvd_t;

/*--- Functions ---*/

int rdt2_rvd_getNumCamSwitches(struct room_s *this);
void rdt2_rvd_getCamSwitch(struct room_s *this, int num_camswitch, struct room_camswitch_s *room_camswitch);

int rdt2_rvd_getNumBoundaries(struct room_s *this);
void rdt2_rvd_getBoundary(struct room_s *this, int num_boundary, struct room_camswitch_s *room_boundary);

#endif /* RDT2_RVD_H */
