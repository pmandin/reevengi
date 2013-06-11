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

/*--- Types ---*/

typedef struct {
	Sint16 cx, cz;
	Uint32 count;
	Sint32 ceiling;
	Uint32 dummy;	/* constant, 0xc5c5c5c5 */
} rdt2_sca_header_t;

typedef struct {
	Sint16 x,z;
	Uint16 w,h;
	Sint16 id, type;
	Uint32 floor;
} rdt2_sca_element_t;

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

	logMsg(1, "sca: offset 0x%08x\n", offset);
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
		logMsg(1, "sca: %d: x=%d,z=%d,w=%d,h=%d, id=%d,type=%d,floor=%d\n", i,
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

int rdt2_sca_getNumCollisions(room_t *this)
{
	rdt2_header_t *rdt_header;
	rdt2_sca_header_t *rdt_sca_hdr;
	Uint32 offset;

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_COLLISION]);
	if (offset==0) {
		return 0;
	}

	rdt_sca_hdr = (rdt2_sca_header_t *) &((Uint8 *) this->file)[offset];
	offset += sizeof(rdt2_sca_header_t);

	return SDL_SwapLE32(rdt_sca_hdr->count)-1;
}

void rdt2_sca_drawMapCollision(room_t *this, int num_collision)
{
	rdt2_header_t *rdt_header;
	rdt2_sca_header_t *rdt_sca_hdr;
	rdt2_sca_element_t *rdt_sca_elt;
	Uint32 offset;
	vertex_t v[4];

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_COLLISION]);
	if (offset==0) {
		return;
	}

	rdt_sca_hdr = (rdt2_sca_header_t *) &((Uint8 *) this->file)[offset];
	if (num_collision >= SDL_SwapLE32(rdt_sca_hdr->count)-1) {
		return;
	}
	offset += sizeof(rdt2_sca_header_t);

	rdt_sca_elt = (rdt2_sca_element_t *) &((Uint8 *) this->file)[offset];

	v[0].x = SDL_SwapLE16(rdt_sca_elt[num_collision].x);
	v[0].y = 0.0f;
	v[0].z = SDL_SwapLE16(rdt_sca_elt[num_collision].z);

	v[1].x = v[0].x + SDL_SwapLE16(rdt_sca_elt[num_collision].w);
	v[1].y = 0.0f;
	v[1].z = v[0].z;

	v[2].x = v[1].x;
	v[2].y = 0.0f;
	v[2].z = v[1].z + SDL_SwapLE16(rdt_sca_elt[num_collision].h);

	v[3].x = v[0].x;
	v[3].y = 0.0f;
	v[3].z = v[2].z;

	render.quad_wf(&v[3], &v[2], &v[1], &v[0]);
}

int rdt2_sca_checkCollision(room_t *this, int num_collision, float x, float y)
{
	rdt2_header_t *rdt_header;
	rdt2_sca_header_t *rdt_sca_hdr;
	rdt2_sca_element_t *rdt_sca_elt;
	Uint32 offset;
	int i, is_inside=1;
	float x1,z1,x2,z2;

	rdt_header = (rdt2_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT2_OFFSET_COLLISION]);
	if (offset==0) {
		return 0;
	}

	rdt_sca_hdr = (rdt2_sca_header_t *) &((Uint8 *) this->file)[offset];
	if (num_collision >= SDL_SwapLE32(rdt_sca_hdr->count)-1) {
		return 0;
	}
	offset += sizeof(rdt2_sca_header_t);

	rdt_sca_elt = (rdt2_sca_element_t *) &((Uint8 *) this->file)[offset];

	x1 = SDL_SwapLE16(rdt_sca_elt[num_collision].x);
	z1 = SDL_SwapLE16(rdt_sca_elt[num_collision].z);
	x2 = SDL_SwapLE16(rdt_sca_elt[num_collision].w) + x1;
	z2 = SDL_SwapLE16(rdt_sca_elt[num_collision].h) + z2;

	return ((x1<=x) && (x<=x2) && (z1<=y) && (y<=z2));
}
