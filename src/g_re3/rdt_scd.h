/*
	Room description
	RE3 RDT script

	Copyright (C) 2009	Patrice Mandin

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

#ifndef RDT_SCD_H
#define RDT_SCD_H 1

Uint8 *rdt3_scd_scriptInit(room_t *this, int num_script);
int rdt3_scd_scriptGetInstLen(room_t *this, Uint8 *curInstPtr);
void rdt3_scd_scriptExecInst(room_t *this);

void rdt3_scd_scriptExec(room_t *this, int num_script);

#endif /* RDT_SCD_H */
