/*
	RE3 SCA
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

#ifndef RDT3_SCA_H
#define RDT3_SCA_H 1

/*--- Functions ---*/

void rdt3_sca_init(room_t *this);

int rdt3_sca_getNumCollisions(room_t *this);
void rdt3_sca_drawMapCollision(room_t *this, int num_collision);

#endif /* RDT3_SCA_H */
