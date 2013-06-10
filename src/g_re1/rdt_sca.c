/*
	RE1 SCA
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

/*--- Defines ---*/

#define RDT_SCA_RECT	1
#define RDT_SCA_CIRC	3

/*--- Types ---*/

typedef struct {
	Sint16 cx, cz;
	Uint32 counts[5];
} rdt1_sca_header_t;

typedef struct {
	Sint16 x1,z1;
	Sint16 x2,z2;
	Uint16 type;
	Uint16 floor;
} rdt1_sca_element_t;

/*--- Functions ---*/

void rdt1_sca_init(room_t *this)
{
	rdt1_header_t *rdt_header;
	rdt1_sca_header_t *rdt_sca_hdr;
	rdt1_sca_element_t *rdt_sca_elt;
	Uint32 offset;
	int i,j=0;

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_COLLISION]);
	if (offset==0) {
		return;
	}

	rdt_sca_hdr = (rdt1_sca_header_t *) &((Uint8 *) this->file)[offset];
	for (i=0; i<5; i++) {
		j += SDL_SwapLE32(rdt_sca_hdr->counts[i]);
	}

	offset += sizeof(rdt1_sca_header_t);

	rdt_sca_elt = (rdt1_sca_element_t *) &((Uint8 *) this->file)[offset];

	/* Display SCA data */

	logMsg(1, "sca: offset 0x%08x, cx=%d,cz=%d,count=%d\n", offset,
		SDL_SwapLE16(rdt_sca_hdr->cx),
		SDL_SwapLE16(rdt_sca_hdr->cz),
		j);

	for (i=0; i<j; i++) {
		logMsg(1, "sca: %d: p1=%d,%d p2=%d,%d type=0x%04x,floor=0x%04x\n", i,
			SDL_SwapLE16(rdt_sca_elt[i].x1),
			SDL_SwapLE16(rdt_sca_elt[i].z1),
			SDL_SwapLE16(rdt_sca_elt[i].x2),
			SDL_SwapLE16(rdt_sca_elt[i].z2),
			SDL_SwapLE16(rdt_sca_elt[i].type),
			SDL_SwapLE16(rdt_sca_elt[i].floor)
		);
	}
}

int rdt1_sca_getNumCollisions(room_t *this)
{
	rdt1_header_t *rdt_header;
	rdt1_sca_header_t *rdt_sca_hdr;
	Uint32 offset;
	int i,j=0;

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_COLLISION]);
	if (offset==0) {
		return 0;
	}

	rdt_sca_hdr = (rdt1_sca_header_t *) &((Uint8 *) this->file)[offset];
	for (i=0; i<5; i++) {
		j += SDL_SwapLE32(rdt_sca_hdr->counts[i]);
	}

	return j;
}

void rdt1_sca_drawMapCollision(room_t *this, int num_collision)
{
	rdt1_header_t *rdt_header;
	rdt1_sca_header_t *rdt_sca_hdr;
	rdt1_sca_element_t *rdt_sca_elt;
	Uint32 offset;
	vertex_t v[4];

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_COLLISION]);
	if (offset==0) {
		return;
	}

	rdt_sca_hdr = (rdt1_sca_header_t *) &((Uint8 *) this->file)[offset];
	if (num_collision >= rdt1_sca_getNumCollisions(this)) {
		return;
	}
	offset += sizeof(rdt1_sca_header_t);

	rdt_sca_elt = (rdt1_sca_element_t *) &((Uint8 *) this->file)[offset];

	v[0].x = SDL_SwapLE16(rdt_sca_elt[num_collision].x1);
	v[0].y = 0.0f;
	v[0].z = SDL_SwapLE16(rdt_sca_elt[num_collision].z1);

	v[1].x = SDL_SwapLE16(rdt_sca_elt[num_collision].x2);
	v[1].y = 0.0f;
	v[1].z = v[0].z;

	v[2].x = v[1].x;
	v[2].y = 0.0f;
	v[2].z = SDL_SwapLE16(rdt_sca_elt[num_collision].z2);

	v[3].x = v[0].x;
	v[3].y = 0.0f;
	v[3].z = v[2].z;

	render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
}
