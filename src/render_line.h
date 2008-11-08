/*
	Render a line in 3D

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
 
#ifndef RENDER_LINE_H
#define RENDER_LINE_H 1

#include <SDL.h>

void render_line_soft(SDL_Surface *surf,
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	Uint32 color);

#endif /* RENDER_LINE_H */
