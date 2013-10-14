/*
	RE1 RID
	Camera positions

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

#ifndef RDT2_RID_H
#define RDT2_RID_H 1

/*--- Defines ---*/

/*--- External types ---*/

struct room_s;
struct room_camera_s;

/*--- Types ---*/

typedef struct {
	Uint16 unk0;
	Uint16 const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	Sint32 camera_from_x;
	Sint32 camera_from_y;
	Sint32 camera_from_z;
	Sint32 camera_to_x;
	Sint32 camera_to_y;
	Sint32 camera_to_z;
	Uint32 masks_offset;
} rdt2_rid_t;

/*--- Functions ---*/

int rdt2_rid_getNumCameras(struct room_s *this);
void rdt2_rid_getCamera(struct room_s *this, int num_camera, struct room_camera_s *room_camera);

#endif /* RDT2_RID_H */
