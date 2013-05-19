/*
	RE2 SCA
	Scene collision array

	Copyright (C) 2013	Patrice Mandin

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

#ifndef RDT_SCA_H
#define RDT_SCA_H 1

/*--- Defines ---*/

/*--- External types ---*/

/*--- Types ---*/

typedef struct {
	Sint16 cx, cz;
	Uint32 count;
	Sint32 ceiling;
	Uint32 dummy;	/* constant, 0xc5c5c5c5 */
} rdt2_sca_header_t;

typedef struct {
	Sint16 x,z;
	Uint16 w,h;
	Uint16 id, type;
	Uint32 floor;
} rdt2_sca_element_t;

/*--- Functions ---*/

void rdt2_sca_init(room_t *this);

#endif /* RDT_SCA_H */
