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

#ifndef RDT_RVD_H
#define RDT_RVD_H 1

/*--- Defines ---*/

#define RDT_RVD_BOUNDARY 9

/*--- External types ---*/

typedef struct room_s room_t;
typedef struct room_camswitch_s room_camswitch_t;

/*--- Types ---*/

typedef struct {
	Uint16 to, from;	/* to = RDT_RVD_BOUNDARY if boundary, not camera switch */
	Sint16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
} rdt_rvd_t;

/*--- Functions ---*/

int rdt1_rvd_getNumCamSwitches(room_t *this);
void rdt1_rvd_getCamSwitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

int rdt1_rvd_getNumBoundaries(room_t *this);
void rdt1_rvd_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_boundary);

#endif /* RDT_RVD_H */
