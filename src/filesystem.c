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

/*--- Includes ---*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <SDL.h>
#include <physfs.h>

#include "parameters.h"
#include "physfsrwops.h"
#include "log.h"

/*--- Defines ---*/

/*--- Global variables ---*/

/*---- Variables ---*/

/*--- Functions prototypes ---*/

/*--- Functions ---*/

int FS_Init(char *argv0)
{
	const char *userdir;
	char *pathname;
	int pathlen;

	if (!PHYSFS_init(argv0)) {
		fprintf(stderr,"fs: PHYSFS_init() failed.\n  reason: %s.\n", PHYSFS_getLastError());
		return 0;
	}

#if 0
	/* Set write directory */
	userdir = PHYSFS_getUserDir();

	pathlen = strlen(userdir)+strlen(PACKAGE_NAME)+2;
	pathname = (char *) malloc(pathlen);
	if (pathname) {
		sprintf(pathname, "%s.%s", userdir, PACKAGE_NAME);
		if (!PHYSFS_setWriteDir(pathname)) {
			/* Create if not already exists */
			sprintf(pathname, ".%s", PACKAGE_NAME);

			if (PHYSFS_setWriteDir(userdir) && PHYSFS_mkdir(pathname)) {
				sprintf(pathname, "%s.%s", userdir, PACKAGE_NAME);
				if (PHYSFS_setWriteDir(pathname)) {
					logMsg(1, "fs: Set write directory to %s\n", pathname);
				}
			}
		} else {
			logMsg(1, "fs: Set write directory to %s\n", pathname);
		}

		free(pathname);
	}
#endif

	return 1;
}

int FS_AddArchive(const char *filename)
{
	int result = 1;
	if (PHYSFS_addToSearchPath(filename, 1)) {
		logMsg(1,"fs: Added %s\n", filename);
		result = 0;
	} else {
		fprintf(stderr, "fs: Error adding %s\n", filename);
	}
	return result;
}

int FS_Shutdown(void)
{
	if (!PHYSFS_deinit()) {
		fprintf(stderr,"fs: PHYSFS_deinit() failed!\n  reason: %s.\n", PHYSFS_getLastError());
		return 0;
	}

	return 1;
}

void *FS_Load(const char *filename, PHYSFS_sint64 *filelength)
{
	PHYSFS_file	*curfile;
	PHYSFS_sint64	curlength;
	void	*buffer;

	curfile=PHYSFS_openRead(filename);
	if (curfile==NULL) {
		/* Try in upper case */

		char *up_filename = calloc(1,strlen(filename)+1);
		if (up_filename) {
			int i;

			for (i=0; i<strlen(filename); i++) {
				up_filename[i] = toupper(filename[i]);
			}

			curfile = PHYSFS_openRead(up_filename);

			free(up_filename);
		}
	}
	if (curfile==NULL) {
		/* Try in lower upcase */

		char *lo_filename = calloc(1,strlen(filename)+1);
		if (lo_filename) {
			int i;

			for (i=0; i<strlen(filename); i++) {
				lo_filename[i] = tolower(filename[i]);
			}

			curfile = PHYSFS_openRead(lo_filename);

			free(lo_filename);
		}
	}
	if (curfile==NULL) {
/*		fprintf(stderr, "fs: can not open %s\n", filename);*/
		return NULL;
	}
	
	curlength = PHYSFS_fileLength(curfile);
	if (filelength!=NULL) {
		*filelength = curlength;
	}

	buffer = malloc(curlength);
	if (buffer == NULL) {
/*		fprintf(stderr,"fs: not enough memory for %s\n",filename);*/
		PHYSFS_close(curfile);
		return NULL;
	}

	PHYSFS_read(curfile, buffer, curlength, 1);
	PHYSFS_close(curfile);

	return(buffer);
}

void *FS_LoadRW(SDL_RWops *src, int *filelength)
{
	void *buffer;

	*filelength = SDL_RWseek(src, 0, RW_SEEK_END);
	SDL_RWseek(src, 0, RW_SEEK_SET);
	
	buffer = malloc(*filelength);
	if (!buffer) {
		fprintf(stderr, "fs: not enough memory to load %d bytes file\n", *filelength);
		return NULL;
	}

	SDL_RWread(src, buffer, *filelength, 1);
	return buffer;
}

int FS_Save(const char *filename, void *buffer, PHYSFS_sint64 length)
{
	PHYSFS_file	*curfile;

	curfile = PHYSFS_openWrite(filename);
	if (!curfile) {
		/*fprintf(stderr, "Unable to open %s\n", filename);*/
		return 0;
	}

	if (PHYSFS_write(curfile, buffer, length, 1) != 1) {
		/*fprintf(stderr, "Unable to write %d bytes\n", length);*/
		return 0;
	}

	PHYSFS_close(curfile);

	return 1;
}

SDL_RWops *FS_makeRWops(const char *filename)
{
	PHYSFS_file	*curfile;

	curfile=PHYSFS_openRead(filename);
	if (curfile==NULL) {
		return NULL;
	}

	return PHYSFSRWOPS_makeRWops(curfile);
}
