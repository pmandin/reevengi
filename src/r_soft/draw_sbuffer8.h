/*
	2D drawing functions
	SBuffer renderer, 8 bits mode

	Copyright (C) 2008-2013	Patrice Mandin

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

#ifndef DRAW_SBUFFER8_H
#define DRAW_SBUFFER8_H 1

/*--- External types ---*/

struct sbuffer_segment_s;

/*--- Functions ---*/

void draw_render_fill8(SDL_Surface *surf, Uint8 *dst_line, struct sbuffer_segment_s *segment, int x1,int x2);

void draw_render_gouraud8(SDL_Surface *surf, Uint8 *dst_line, struct sbuffer_segment_s *segment, int x1,int x2);
void draw_render_gouraud8_pc0(SDL_Surface *surf, Uint8 *dst_line, struct sbuffer_segment_s *segment, int x1,int x2);
void draw_render_gouraud8_pc1(SDL_Surface *surf, Uint8 *dst_line, struct sbuffer_segment_s *segment, int x1,int x2);
void draw_render_gouraud8_pc3(SDL_Surface *surf, Uint8 *dst_line, struct sbuffer_segment_s *segment, int x1,int x2);

void draw_render_textured8(SDL_Surface *surf, Uint8 *dst_line, struct sbuffer_segment_s *segment, int x1,int x2);

#endif /* DRAW_SBUFFER8_H */
