/*
	RE1 RVD
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

#ifndef RDT1_RVD_H
#define RDT1_RVD_H 1

/*--- Defines ---*/

#define RDT_RVD_BOUNDARY 9

/*--- External types ---*/

struct room_s;
struct room_camswitch_s;

/*--- Types ---*/

typedef struct {
	Uint16 to, from;	/* to = RDT_RVD_BOUNDARY if boundary, not camera switch */
	Sint16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
} rdt1_rvd_t;

/*--- Functions ---*/

int rdt1_rvd_getNumCamSwitches(struct room_s *this);
void rdt1_rvd_getCamSwitch(struct room_s *this, int num_camswitch, struct room_camswitch_s *room_camswitch);

int rdt1_rvd_getNumBoundaries(struct room_s *this);
void rdt1_rvd_getBoundary(struct room_s *this, int num_boundary, struct room_camswitch_s *room_boundary);

#endif /* RDT_RVD_H */
