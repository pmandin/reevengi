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

#include "render.h"

/*--- Functions prototypes ---*/

static void set_projection(float angle, float aspect,
	float z_near, float z_far);
static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up);
static void scale(float x, float y, float z);
static void translate(float x, float y, float z);
static void push_matrix(void);
static void pop_matrix(void);

static void line(SDL_Surface *surf,
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	Uint32 color);
static void scaled_image(SDL_Surface *surf, SDL_Surface *source,
	int x, int y, int w, int h);

/*--- Functions ---*/

void render_soft_init(render_t *render)
{
	render->set_projection = set_projection;
	render->set_modelview = set_modelview;
	render->scale = scale;
	render->translate = translate;
	render->push_matrix = push_matrix;
	render->pop_matrix = pop_matrix;

	render->line = line;
	render->scaled_image = scaled_image;
}

static void set_projection(float angle, float aspect, float z_near, float z_far)
{
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
}

static void scale(float x, float y, float z)
{
}

static void translate(float x, float y, float z)
{
}

static void push_matrix(void)
{
}

static void pop_matrix(void)
{
}

static void line(SDL_Surface *surf,
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	Uint32 color)
{
}

static void scaled_image(SDL_Surface *surf, SDL_Surface *source,
	int x, int y, int w, int h)
{
}
