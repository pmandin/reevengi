/*
	Load EMD model

	Copyright (C) 2008	Patrice Mandin

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

#include <SDL.h>

#include "filesystem.h"
#include "video.h"

/*--- Variables ---*/

static void *emd_file = NULL;
static void *tim_file = NULL;
static int firsttime = 1;

/*--- Types ---*/

typedef struct {
	Uint32 offset;
	Uint32 length;
} emd_t;

/*--- Functions prototypes ---*/

/*--- Functions ---*/

int model_emd_load(const char *filename)
{
	PHYSFS_sint64 length;
	int retval = 0;
	char *tim_filename;
	
	emd_file = FS_Load(filename, &length);
	if (!emd_file) {
		fprintf(stderr, "emd: Can not load %s\n", filename);
		return 0;
	}

	tim_filename = calloc(1, strlen(filename)+1);
	if (!tim_filename) {
		fprintf(stderr, "emd: Can not allocate memory for filename\n");
		return 0;
	}
	strncpy(tim_filename, filename, strlen(filename)-4);
	strcat(tim_filename, ".tim");

	tim_file = FS_Load(tim_filename, &length);
	if (!tim_file) {
		fprintf(stderr, "emd: Can not load %s\n", tim_filename);
	} else {
		retval = 1;
	}

	free(tim_filename);
	return retval;
}

void model_emd_close(void)
{
	if (emd_file) {
		free(emd_file);
		emd_file = NULL;
	}

	if (tim_file) {
		free(tim_file);
		tim_file = NULL;
	}
}

void model_emd_draw(video_t *video)
{
	emd_t *emd_hdr = (emd_t *) emd_file;
	Uint32 *hdr_offsets;
	int i;

	if (!emd_file) {
		return;
	}

	if (!firsttime) {
		return;
	}
	firsttime = 0;

	/*printf("emd: header 0x%08x, length %d\n",
		SDL_SwapLE32(emd_hdr->offset), SDL_SwapLE32(emd_hdr->length)
	);*/
	hdr_offsets = (Uint32 *) (&((char *) emd_file)[SDL_SwapLE32(emd_hdr->offset)]);
	/*for (i=0; i<SDL_SwapLE32(emd_hdr->length); i++) {
		printf("emd:  object %d: offset 0x%08x\n", i, hdr_offsets[i]);
	}*/
}
