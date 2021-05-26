/*
	Load background from tim file

	Copyright (C) 2007	Patrice Mandin

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

#ifndef BACKGROUND_TIM_H
#define BACKGROUND_TIM_H 1

/*--- Defines ---*/

#define MAGIC_TIM	0x10

#define TIM_TYPE_WITHPAL	(1<<3)
#define TIM_TYPE_BPP4		(0 | TIM_TYPE_WITHPAL)
#define TIM_TYPE_BPP8		(1 | TIM_TYPE_WITHPAL)
#define TIM_TYPE_BPP15		2
#define TIM_TYPE_BPP24		3

/*--- Types ---*/

typedef struct {
	Uint32	magic;
	Uint32	type;
	Uint32	offset;
	Uint16	dummy0;
	Uint16	dummy1;
	Uint16	palette_colors;
	Uint16	nb_palettes;
} tim_header_t;

typedef struct {
	Uint16	width;
	Uint16	height;
} tim_size_t;

/*--- Functions ---*/

SDL_Surface *background_tim_load(SDL_RWops *src, int row_offset);

#endif /* BACKGROUND_TIM_H */
