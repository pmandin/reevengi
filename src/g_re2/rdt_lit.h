/*
	RE2 LIT
	Lights

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

#ifndef RDT2_LIT_H
#define RDT2_LIT_H 1

/*--- Types ---*/

typedef struct {
	Uint8 r,g,b;
} rdt_light_color_t;

typedef struct {
	Sint16 x,y,z;
} rdt_light_pos_t;

typedef struct {
	Uint16 type[2];
	rdt_light_color_t col[3];
	rdt_light_color_t ambient;
	rdt_light_pos_t pos[3];
	Uint16 brightness[3];
} rdt_light_t;

#endif /* RDT2_LIT_H */
