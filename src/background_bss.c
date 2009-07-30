/*
	Load background from bss file

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

#include <stdlib.h>

#include <SDL.h>

#include "state.h"
#include "filesystem.h"
#include "background_bss.h"
#include "depack_vlc.h"
#include "depack_mdec.h"
#include "video.h"

/*--- Defines ---*/

#define WIDTH 320
#define HEIGHT 240

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static int background_vlc_load(SDL_RWops *src, int chunk_size);
static int background_mdec_load(SDL_RWops *src);

/*--- Functions ---*/

int background_bss_load(const char *filename, int chunk_size)
{
	SDL_RWops *src;
	/*Uint8 *dstBuffer;
	int dstBufLen;*/
	int retval = 0;
	
	src = FS_makeRWops(filename);
	if (src) {
		retval = background_vlc_load(src, chunk_size);

		SDL_RWclose(src);
	}

	return retval;
}

static int background_vlc_load(SDL_RWops *src, int chunk_size)
{
	Uint8 *dstBuffer;
	int dstBufLen;
	int retval = 0;

	SDL_RWseek(src, game_state.num_camera * chunk_size, RW_SEEK_SET);

	vlc_depack(src, &dstBuffer, &dstBufLen);

	if (dstBuffer && dstBufLen) {
		SDL_RWops *mdec_src;
			
		mdec_src = SDL_RWFromMem(dstBuffer, dstBufLen);
		if (mdec_src) {
			retval = background_mdec_load(mdec_src);

			SDL_FreeRW(mdec_src);
		}

		free(dstBuffer);
	}

	return retval;
}

static int background_mdec_load(SDL_RWops *src)
{
	Uint8 *dstBuffer;
	int dstBufLen;
	int retval = 0;

	mdec_depack(src, &dstBuffer, &dstBufLen, WIDTH, HEIGHT);

	if (dstBuffer && dstBufLen) {
		SDL_Surface *image;

		/*game_state.background_surf = mdec_surface(dstBuffer, WIDTH, HEIGHT);
		if (game_state.background_surf) {
			retval = 1;
		}*/

		image = mdec_surface(dstBuffer, WIDTH, HEIGHT);
		if (image) {
			game_state.back_surf = video.createSurfaceSu(image);
			if (game_state.back_surf) {
				video.convertSurface(game_state.back_surf);
				retval = 1;
			}
			SDL_FreeSurface(image);
		}

		free(dstBuffer);
	}

	return retval;
}
