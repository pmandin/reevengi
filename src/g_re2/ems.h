/*
	RE2 PS1 EMS files

	Copyright (C) 2007-2013	Patrice Mandin

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

#ifndef RE2_EMS_H
#define RE2_EMS_H 1

/*--- Types ---*/

typedef struct {
	Uint32	offset;
	Uint32	flag;	/* 0: ignore, 1: tim or emd */
} re2ps1_ems_t;

/*--- External types ---*/

struct game_s;

/*--- Functions prototypes ---*/

/* filename is allocated, must be freed by caller */
void ems_getModel(struct game_s *this, int num_model, char **filename,
	const re2ps1_ems_t **ems_array, int *emd_pos, int *tim_pos);

#endif /* RE2_EMS_H */
