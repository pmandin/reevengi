/*
	2D drawing functions
	SBuffer renderer, 32 bits mode

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <assert.h>

#include "../video.h"
#include "../parameters.h"

#include "../r_common/render.h"
#include "../r_common/r_misc.h"

#include "dither.h"
#include "draw.h"
#include "draw_sbuffer.h"

/*--- Defines ---*/

#define CONCAT2(x,y)	x ## y
#define FNDEF2(name,bpp)	CONCAT2(name,bpp)
#define CONCAT3(x,y,z)	x ## y ## z
#define FNDEF3(name,bpp,perscorr)	CONCAT3(name,bpp,perscorr)

#define BPP 32
#define PIXEL_TYPE	Uint32
#define WRITE_PIXEL(output, color) \
		*output = color;
#define WRITE_PIXEL_GONEXT(output, color) \
		*output++ = color;
#define PIXEL_GONEXT(output) \
	output++;
#define PIXEL_FROM_RGB(color, r,g,b) \
	color = SDL_MapRGB(surf->format, r,g,b);


/*#define FORCE_OPAQUE 1*/

/*--- Functions ---*/

#include "span_fill.inc.c"

#include "span_gouraud.inc.c"

#include "span_textured.inc.c"
