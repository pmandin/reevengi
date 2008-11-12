/*
	Render engine

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

#ifndef RENDER_H
#define RENDER_H 1

#include <SDL.h>

#include "video_surface.h"

typedef struct render_s render_t;

struct render_s {
	Uint32 color;

	void (*set_projection)(float angle, float aspect,
		float z_near, float z_far);
	void (*set_modelview)(float x_from, float y_from, float z_from,
		float x_to, float y_to, float z_to,
		float x_up, float y_up, float z_up);
	void (*scale)(float x, float y, float z);
	void (*translate)(float x, float y, float z);
	void (*push_matrix)(void);
	void (*pop_matrix)(void);

	void (*set_color)(SDL_Surface *surf, Uint32 color);	/* color in ARGB format */
	void (*line)(SDL_Surface *surf,
		float x1, float y1, float z1,
		float x2, float y2, float z2);

	void (*initBackground)(video_t *this, video_surface_t *source);
	void (*drawBackground)(video_t *this);

	void (*shutdown)(render_t *this);
};

void render_soft_init(render_t *render);

void render_opengl_init(render_t *render);

/*--- Variables ---*/

extern render_t render;

#endif /* RENDER_H */
