/*
	Filesystem, using physfs

	Copyright (C) 2007	Patrice Mandin

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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

/*--- Includes ---*/

#include <SDL.h>

#include <physfs.h>

/*--- Variables ---*/

/*--- Functions prototypes ---*/

int FS_Init(char *argv0);
void FS_AddArchive(const char *filename);
int FS_Shutdown(void);

void *FS_Load(const char *filename, PHYSFS_sint64 *filelength);
int FS_Save(const char *filename, void *buffer, PHYSFS_sint64 length);

SDL_RWops *FS_makeRWops(const char *filename);

#endif /* FILESYSTEM_H */
