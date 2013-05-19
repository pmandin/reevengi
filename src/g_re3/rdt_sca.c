/*
	RE3 SCA
	Scene collision array

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

#include <SDL.h>

#include "../render.h"
#include "../log.h"

#include "../g_common/room.h"

#include "rdt.h"
#include "rdt_sca.h"

/*--- Functions ---*/

void rdt3_sca_init(room_t *this)
{
	rdt3_header_t *rdt_header;
	rdt3_sca_header_t *rdt_sca_hdr;
	rdt3_sca_element_t *rdt_sca_elt;
	Uint32 offset;
	int i;

	rdt_header = (rdt3_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT3_OFFSET_COLLISION]);
	if (offset==0) {
		return;
	}

	logMsg(1, "sca: offset 0x%08x\n", offset);
	rdt_sca_hdr = (rdt3_sca_header_t *) &((Uint8 *) this->file)[offset];
	offset += sizeof(rdt3_sca_header_t);

	rdt_sca_elt = (rdt3_sca_element_t *) &((Uint8 *) this->file)[offset];

	/* Display SCA data */
	logMsg(1, "sca: count=%d,ceiling=%d\n",
		SDL_SwapLE32(rdt_sca_hdr->count)-1
	);

	for (i=0; i<SDL_SwapLE32(rdt_sca_hdr->count)-1; i++) {
		logMsg(1, "sca: %d: x=%d,z=%d,w=%d,h=%d, type=0x%04x,floor=0x%04x\n", i,
			SDL_SwapLE16(rdt_sca_elt[i].x),
			SDL_SwapLE16(rdt_sca_elt[i].z),
			SDL_SwapLE16(rdt_sca_elt[i].w),
			SDL_SwapLE16(rdt_sca_elt[i].h),
			SDL_SwapLE16(rdt_sca_elt[i].type),
			SDL_SwapLE16(rdt_sca_elt[i].floor)
		);
	}
}