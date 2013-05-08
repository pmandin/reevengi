/*
	Room data

	Copyright (C) 2007-2013	Patrice Mandin

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

#ifndef ROOM_H
#define ROOM_H 1

/*--- Defines ---*/

/*--- External types ---*/

typedef struct render_texture_s render_texture_t;
typedef struct render_mask_s render_mask_t;

/*--- Types ---*/

typedef struct room_camera_s room_camera_t;

struct room_camera_s {
	Sint32 from_x;	/* Camera is there */
	Sint32 from_y;
	Sint32 from_z;
	Sint32 to_x;	/* looking at this point */
	Sint32 to_y;
	Sint32 to_z;
};

typedef struct room_camswitch_s room_camswitch_t;

struct room_camswitch_s {
	Uint8 from, to;	/* camera to switch from/to, to=0 for boundary */
	Sint16 x[4];	/* Quad zone to check when player enters/exits it */
	Sint16 y[4];
};

typedef struct room_s room_t;

struct room_s {
	void (*dtor)(room_t *this);

	void (*load)(room_t *this, int stage, int room, int camera);
	void (*load_background)(room_t *this, int stage, int room, int camera);
	void (*load_bgmask)(room_t *this, int stage, int room, int camera);

	/* RDT file */
	void *file;
	Uint32 file_length;

	/* Background image for current camera */
	render_texture_t *background;

	/*--- Camera positions ---*/
	int num_cameras;

	int (*getNumCameras)(room_t *this);
	void (*getCamera)(room_t *this, int num_camera, room_camera_t *room_camera);

	/*--- Camera switches ---*/
	int (*getNumCamSwitches)(room_t *this);
	void (*getCamSwitch)(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

	/*--- Boundaries ---*/
	int (*getNumBoundaries)(room_t *this);
	void (*getBoundary)(room_t *this, int num_boundary, room_camswitch_t *room_boundary);

	/*--- Background masking ---*/
	render_texture_t *bg_mask;
	render_mask_t *rdr_mask;

	void (*initMasks)(room_t *this, int num_camera);
	void (*drawMasks)(room_t *this, int num_camera);
};

/*--- Variables ---*/

/*--- Functions ---*/

room_t *room_ctor(void);

#endif /* ROOM_H */
