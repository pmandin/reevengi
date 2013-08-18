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

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "../filesystem.h"
#include "../parameters.h"
#include "../log.h"

#include "ard.h"

/*--- Functions prototypes ---*/

static void *ard_loadFile(const char *filename, int num_object, int *file_length);

/*--- Functions ---*/

void *ard_loadRdtFile(const char *filename, int *file_length)
{
	return ard_loadFile(filename, RE3_ARD_RDT, file_length);
}

static void *ard_loadFile(const char *filename, int num_object, int *file_length)
{
	PHYSFS_sint64 length;
	Uint8 *ard_file;
	ard_object_t *ard_object;
	int i, count;
	Uint32 offset, len;
	void *file;

	ard_file = (Uint8 *) FS_Load(filename, &length);
	if (!ard_file) {
		return NULL;
	}

	count = ((ard_header_t *) ard_file)->count;
	count = SDL_SwapLE32(count);
/*
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		logMsg(1, "ard: object %d at offset 0x%08x\n", i,offset);
		len = SDL_SwapLE32(ard_object->length);

		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}
*/
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		len = SDL_SwapLE32(ard_object->length);
		if (i==num_object) {
			/* Stop on needed embedded file */
			break;
		}
		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}

	file = malloc(len);
	if (!file) {
		free(ard_file);
		return NULL;
	}

	logMsg(3, __FILE__ ": Loading embedded file from offset 0x%08x\n", offset);

	memcpy(file, &ard_file[offset], len);
	free(ard_file);

	*file_length = len;
	return file;
}
