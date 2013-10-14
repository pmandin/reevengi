/*
	2D drawing functions
	SBuffer renderer

	Copyright (C) 2008	Patrice Mandin

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

#ifndef DRAW_SBUFFER_H
#define DRAW_SBUFFER_H 1

/*--- External types ---*/

struct draw_s;
struct render_texture_s;

/*--- Types ---*/

typedef struct sbuffer_point_s sbuffer_point_t;

struct sbuffer_point_s {
	int x;		/* x on screen*/
	float r,g,b;	/* color */
	float u,v;	/* u,v coords */
	float w;	/* w=1/z */
};

typedef struct sbuffer_segment_s sbuffer_segment_t;

struct sbuffer_segment_s {
	Uint8	dummy;
	Uint8	render_mode;
	Uint8	tex_num_pal;
	Uint8	masking;
	render_texture_t *texture;
	sbuffer_point_t start, end;
};

/*--- Functions prototypes ---*/

void draw_init_sbuffer(struct draw_s *draw);

#endif /* DRAW_SBUFFER_H */
