/*
	Draw background image mask

	Copyright (C) 2010	Patrice Mandin

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

#ifndef RENDER_MASK_OPENGL_H
#define RENDER_MASK_OPENGL_H 1

#include <SDL.h>

#include "video.h"
#include "render_texture.h"
#include "render_mask.h"

/*--- Types ---*/

typedef struct {
	Uint16 srcx, srcy;
	Uint16 width, height;
	Uint16 dstx, dsty;
	float depth;
} render_mask_gl_zone_t;

typedef struct render_mask_gl_s render_mask_gl_t;

struct render_mask_gl_s {
	render_mask_t	render_mask;

	int num_zones;
	render_mask_gl_zone_t	*zones;
};

render_mask_t *render_mask_opengl_create(render_texture_t *texture);

#endif /* RENDER_MASK_OPENGL_H */
