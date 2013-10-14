/*
	RE1 SCA
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

#ifndef RDT1_SCA_H
#define RDT1_SCA_H 1

/*--- External types ---*/

struct room_s;

/*--- Functions ---*/

void rdt1_sca_init(struct room_s *this);

int rdt1_sca_getNumCollisions(struct room_s *this);
void rdt1_sca_drawMapCollision(struct room_s *this, int num_collision);

#endif /* RDT1_SCA_H */
