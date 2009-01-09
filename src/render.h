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
#include "render_texture.h"

/*--- Types ---*/

typedef struct {
	Sint16 x,y,z;	/* Vertex coords */
	Uint16 u,v;	/* Texture coords */
} vertex_t;

typedef struct render_s render_t;

struct render_s {
	void (*set_viewport)(int x, int y, int w, int h);
	void (*set_projection)(float angle, float aspect,
		float z_near, float z_far);
	void (*set_modelview)(float x_from, float y_from, float z_from,
		float x_to, float y_to, float z_to,
		float x_up, float y_up, float z_up);
	void (*set_identity)(void);
	void (*scale)(float x, float y, float z);
	void (*translate)(float x, float y, float z);
	void (*rotate)(float angle, float x, float y, float z);
	void (*push_matrix)(void);
	void (*pop_matrix)(void);

	void (*set_color)(Uint32 color);	/* color in ARGB format */
	void (*set_texture)(int num_pal, render_texture_t *render_tex);

	void (*line)(vertex_t *v1, vertex_t *v2);
	void (*triangle)(vertex_t *v1, vertex_t *v2, vertex_t *v3);
	void (*quad)(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

	void (*triangle_tex)(vertex_t *v1, vertex_t *v2, vertex_t *v3);
	void (*quad_tex)(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

	void (*initBackground)(video_t *this, video_surface_t *source);
	void (*drawBackground)(video_t *this);

	void (*shutdown)(render_t *this);

	render_texture_t *texture;
};

void render_soft_init(render_t *render);

void render_opengl_init(render_t *render);

/*--- Variables ---*/

extern render_t render;

#endif /* RENDER_H */
