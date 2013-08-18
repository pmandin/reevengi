/*
	Archive room data
	RE3 ARD manager

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

#ifndef RE3_ARD_H
#define RE3_ARD_H 1

/*--- Defines ---*/

#define RE3_ARD_RDT	8

/*--- Types ---*/

typedef struct {
	Uint32 length;
	Uint32 count;
} ard_header_t;

typedef struct {
	Uint32 length;
	Uint32 unknown;
} ard_object_t;

/*--- Functions prototypes ---*/

void *ard_loadRdtFile(const char *filename, int *file_length);

#endif /* RE3_ARD_H */
