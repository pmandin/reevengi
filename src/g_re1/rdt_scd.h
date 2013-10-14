/*
	RE1 SCD
	Game scripts

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

#ifndef RDT1_SCD_H
#define RDT1_SCD_H 1

/*--- External types ---*/

struct room_s;

/*--- Functions ---*/

Uint8 *rdt1_scd_scriptInit(struct room_s *this, int num_script);
int rdt1_scd_scriptGetInstLen(struct room_s *this, Uint8 *curInstPtr);
void rdt1_scd_scriptExecInst(struct room_s *this);

void rdt1_scd_scriptExec(struct room_s *this, int num_script);

#endif /* RDT1_SCD_H */
