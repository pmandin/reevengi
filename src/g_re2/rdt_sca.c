/*
	RE2 SCA
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

void rdt2_sca_init(room_t *this)
{
	rdt2_header_t *rdt_header;
	rdt2_sca_header_t *rdt_sca_hdr;
	rdt2_sca_element_t *rdt_sca_elt;
	Uint32 offset;
	int i;

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_COLLISION]);
	if (offset==0) {
		return;
	}

	rdt_sca_hdr = (rdt2_sca_header_t *) &((Uint8 *) this->file)[offset];
	offset += sizeof(rdt2_sca_header_t);

	rdt_sca_elt = (rdt2_sca_element_t *) &((Uint8 *) this->file)[offset];

	/* Display SCA data */
	logMsg(1, "sca: cx=%d,cz=%d,count=%d,ceiling=%d\n",
		SDL_SwapLE16(rdt_sca_hdr->cx),
		SDL_SwapLE16(rdt_sca_hdr->cz),
		SDL_SwapLE32(rdt_sca_hdr->count)-1,
		SDL_SwapLE32(rdt_sca_hdr->ceiling));

	for (i=0; i<SDL_SwapLE32(rdt_sca_hdr->count)-1; i++) {
		logMsg(1, "sca: %d: x=%d,z=%d,w=%d,h=%d, id=0x%04x,type=0x%04x,floor=%d\n", i,
			SDL_SwapLE16(rdt_sca_elt[i].x),
			SDL_SwapLE16(rdt_sca_elt[i].z),
			SDL_SwapLE16(rdt_sca_elt[i].w),
			SDL_SwapLE16(rdt_sca_elt[i].h),
			SDL_SwapLE16(rdt_sca_elt[i].id),
			SDL_SwapLE16(rdt_sca_elt[i].type),
			SDL_SwapLE32(rdt_sca_elt[i].floor)
		);
	}
}
