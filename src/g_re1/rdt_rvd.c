/*
	RE1 RVD
	Camera switches

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

#include <assert.h>
#include <SDL.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../g_common/room.h"
#include "../g_common/room_camswitch.h"

#include "rdt.h"
#include "rdt_rvd.h"

/*--- Functions prototypes ---*/

static int rdt1_rvd_getNumRvd(room_t *this, SDL_bool boundary_flag);
static void rdt1_rvd_getRvd(room_t *this, SDL_bool boundary_flag, int num_rvd, room_camswitch_t *room_rvd);

/*--- Functions ---*/

int rdt1_rvd_getNumCamSwitches(room_t *this)
{
	return rdt1_rvd_getNumRvd(this, SDL_FALSE);
}

void rdt1_rvd_getCamSwitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
	rdt1_rvd_getRvd(this, SDL_FALSE, num_camswitch, room_camswitch);
}

int rdt1_rvd_getNumBoundaries(room_t *this)
{
	return rdt1_rvd_getNumRvd(this, SDL_TRUE);
}

void rdt1_rvd_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_boundary)
{
	rdt1_rvd_getRvd(this, SDL_TRUE, num_boundary, room_boundary);
}

static int rdt1_rvd_getNumRvd(room_t *this, SDL_bool boundary_flag)
{
	rdt1_header_t *rdt_header;
	Uint32 offset;
	rdt1_rvd_t *rvd_array;
	int i=0, j=0;

	rdt_header = (rdt1_header_t *) this->file;
	if (!rdt_header) {
		return 0;
	}

	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_CAM_SWITCHES]);
	rvd_array = (rdt1_rvd_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(rvd_array[i].to) != 0xffff) {
		int is_boundary = (SDL_SwapLE16(rvd_array[i].to) == RDT_RVD_BOUNDARY);

		if ((is_boundary && boundary_flag) || (!is_boundary && !boundary_flag)) {
			++j;
		}

		++i;
	}

	return j;
}

static void rdt1_rvd_getRvd(room_t *this, SDL_bool boundary_flag, int num_rvd, room_camswitch_t *room_rvd)
{
	rdt1_header_t *rdt_header;
	Uint32 offset;
	rdt1_rvd_t *rvd_array;
	int i=0, j=0;

	rdt_header = (rdt1_header_t *) this->file;
	assert(rdt_header);
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_CAM_SWITCHES]);
	rvd_array = (rdt1_rvd_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(rvd_array[i].to) != 0xffff) {
		int is_boundary = (SDL_SwapLE16(rvd_array[i].to) == RDT_RVD_BOUNDARY);

		if ((is_boundary && boundary_flag) || (!is_boundary && !boundary_flag)) {
			if (j==num_rvd) {
				break;
			}

			++j;
		}

		++i;
	}

	room_rvd->from = SDL_SwapLE16(rvd_array[i].from);
	room_rvd->to = SDL_SwapLE16(rvd_array[i].to);
	room_rvd->x[0] = SDL_SwapLE16(rvd_array[i].x1);
	room_rvd->y[0] = SDL_SwapLE16(rvd_array[i].y1);
	room_rvd->x[1] = SDL_SwapLE16(rvd_array[i].x2);
	room_rvd->y[1] = SDL_SwapLE16(rvd_array[i].y2);
	room_rvd->x[2] = SDL_SwapLE16(rvd_array[i].x3);
	room_rvd->y[2] = SDL_SwapLE16(rvd_array[i].y3);
	room_rvd->x[3] = SDL_SwapLE16(rvd_array[i].x4);
	room_rvd->y[3] = SDL_SwapLE16(rvd_array[i].y4);
}
