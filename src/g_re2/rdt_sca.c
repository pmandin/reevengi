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
#include <math.h>

#include "../render.h"
#include "../log.h"

#include "../g_common/room.h"

#include "rdt.h"
#include "rdt_sca.h"

/*--- Defines ---*/

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
	Sint16 type, floor; 
	Uint32 flags;
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
		logMsg(1, "sca: %d: x=%d,z=%d,w=%d,h=%d, type=0x%04x,floor=0x%04x,flags=0x%08x\n", i,
			SDL_SwapLE16(rdt_sca_elt[i].x),
			SDL_SwapLE16(rdt_sca_elt[i].z),
			SDL_SwapLE16(rdt_sca_elt[i].w),
			SDL_SwapLE16(rdt_sca_elt[i].h),
			SDL_SwapLE16(rdt_sca_elt[i].type) & 0xffffUL,
			SDL_SwapLE16(rdt_sca_elt[i].floor)  & 0xffffUL,
			SDL_SwapLE32(rdt_sca_elt[i].flags)
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

	if (SDL_SwapLE16(rdt_sca_elt[num_collision].flags)>3) {
		return;
	}

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

	switch(SDL_SwapLE16(rdt_sca_elt[num_collision].type) & 7) {
		case 0:
		case 7:
			/* rectangle */
			render.quad_wf(&v[3], &v[2], &v[1], &v[0]);
			break;
		case 1:
			/* triangle */
			{
				render.line(&v[1], &v[2]);
				render.line(&v[2], &v[3]);
				render.line(&v[3], &v[1]);
			}
			break;
		case 2:
			/* triangle */
			{
				render.line(&v[0], &v[2]);
				render.line(&v[2], &v[3]);
				render.line(&v[3], &v[0]);
			}
			break;
		case 3:
			/* triangle */
			{
				render.line(&v[0], &v[1]);
				render.line(&v[1], &v[2]);
				render.line(&v[2], &v[0]);
			}
			break;
		case 4:
			/* triangle */
			{
				render.line(&v[0], &v[1]);
				render.line(&v[1], &v[3]);
				render.line(&v[3], &v[0]);
			}
			break;
		case 5:
			/* ellipse w*h */
			{
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

				render.line(&v[0], &v[2]);
				render.line(&v[2], &v[3]);
				render.line(&v[3], &v[0]);
			}
			break;
		case 6:
			/* circle */
			{
				int rx, rz, cx, cz, i;

				rx = SDL_SwapLE16(rdt_sca_elt[num_collision].w)/2;
				rz = SDL_SwapLE16(rdt_sca_elt[num_collision].h)/2;

				cx = SDL_SwapLE16(rdt_sca_elt[num_collision].x) + rx;
				cz = SDL_SwapLE16(rdt_sca_elt[num_collision].z) + rz;

				v[0].x = cx + rx;
				v[0].y = v[1].y = 0;
				v[0].z = cz;

				for (i=0; i<16+1; i++) {
					float angle = ( ((float) i) *M_PI)/8.0f;

					v[1].x = cx + rx * cos(angle);
					v[1].z = cz + rz * sin(angle);

					render.line(&v[0], &v[1]);

					v[0].x = v[1].x;
					v[0].z = v[1].z;
				}
			}
			break;
		default:
			break;
	}

}

int rdt2_sca_checkCollision(room_t *this, int num_collision, float x, float y)
{
	rdt2_header_t *rdt_header;
	rdt2_sca_header_t *rdt_sca_hdr;
	rdt2_sca_element_t *rdt_sca_elt;
	Uint32 offset;
	int i, is_inside;
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

	if (SDL_SwapLE16(rdt_sca_elt[num_collision].flags)>3) {
		return 0;
	}

	x1 = SDL_SwapLE16(rdt_sca_elt[num_collision].x);
	z1 = SDL_SwapLE16(rdt_sca_elt[num_collision].z);
	x2 = SDL_SwapLE16(rdt_sca_elt[num_collision].w) + x1;
	z2 = SDL_SwapLE16(rdt_sca_elt[num_collision].h) + z1;

	is_inside= ((x1<=x) && (x<=x2) && (z1<=y) && (y<=z2));
#if 0
	if (is_inside) {
		logMsg(1, "rdt2: sca: inside %d\n" /*" x:%.0f<%.0f<%.0f z:%.0f<%.0f<%.0f\n"*/,
			num_collision /*, x1,x,x2, z1,y,z2*/);
	}
#endif

	return is_inside;
}

/*
 type: bit 0-3:
	0 rectangle
	1,2,3,4 triangle
	5 ellipse
	6 rounded box
	7 rounded box

room1000 triangle                            offset 0xf80
[    0.330] sca: 9: x=-25205,z=1744,w=8170,h=9119, type=0xfe81,floor=0x9002,flags=0x00000001
long rect
[    0.304] sca: 6: x=-15105,z=-7673,w=36140,h=4390, type=0xfe87,floor=0x900c,flags=0x00000001
rect						offset 0xfa0
[    0.334] sca: 11: x=-15140,z=537,w=11950,h=10400, type=0xfe87,floor=0x9002,flags=0x00000001
circle
[    0.494] sca: 12: x=18689,z=4063,w=4480,h=4480, type=0xfe86,floor=0x9001,flags=0x00000001
circle
[    0.276] sca: 17: x=-4035,z=3012,w=1060,h=1060, type=0xde06,floor=0x9002,flags=0x00000001

room2000
[   10.662] sca: 2: x=-32000,z=-32000,w=35506,h=5200, id=-384,type=-28020,floor=2047
[   10.663] sca: 11: x=-11383,z=-21180,w=10760,h=1020, id=-384,type=-28604,floor=3
[   10.663] sca: 22: x=-11383,z=-31680,w=13079,h=11080, id=-384,type=-28668,floor=1

[   10.664] sca: 43: x=-7754,z=-11050,w=4770,h=6810, id=-8693,type=-28571,floor=3
*/
