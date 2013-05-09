/*
	RE2 RVD
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

#include "rdt.h"
#include "rdt_rvd.h"

/*--- Functions ---*/

int rdt2_rvd_getNumCamSwitches(room_t *this)
{
	rdt2_header_t *rdt_header;
	Uint32 offset;
	rdt2_rvd_t *camswitch_array;
	int i=0, num_switches = 0, prev_from = -1;

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt2_rvd_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary=0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			/* boundary, not a switch */
		} else {
			num_switches++;
		}

		++i;
	}

	return num_switches;
}

void rdt2_rvd_getCamSwitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
	rdt2_header_t *rdt_header;
	Uint32 offset;
	rdt2_rvd_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt2_rvd_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary = 0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			/* boundary, not a switch */
		} else {
			if (j==num_camswitch) {
				break;
			}

			++j;
		}

		++i;
	}

	room_camswitch->from = camswitch_array[i].from;
	room_camswitch->to = camswitch_array[i].to;
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}

int rdt2_rvd_getNumBoundaries(room_t *this)
{
	rdt2_header_t *rdt_header;
	Uint32 offset;
	rdt2_rvd_t *camswitch_array;
	int i=0, num_boundaries = 0, prev_from = -1;

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt2_rvd_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary=0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			++num_boundaries;
		} else {
			/* switch, not a boundary */
		}

		++i;
	}

	return num_boundaries;
}

void rdt2_rvd_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_boundary)
{
	rdt2_header_t *rdt_header;
	Uint32 offset;
	rdt2_rvd_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt2_rvd_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary = 0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			if (j==num_boundary) {
				break;
			}

			++j;
		} else {
			/* switch, not a boundary */
		}

		++i;
	}

	room_boundary->from = camswitch_array[i].from;
	room_boundary->to = camswitch_array[i].to;
	room_boundary->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_boundary->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_boundary->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_boundary->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_boundary->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_boundary->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_boundary->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_boundary->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}
