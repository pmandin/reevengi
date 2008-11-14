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

#include "video.h"
#include "render.h"
#include "render_background.h"
#include "matrix.h"

/*--- Defines ---*/

#define MAX_PROJMODELMTX 16

/*--- Variables ---*/

static float projmodelmtx[MAX_PROJMODELMTX][4][4];	/* 16 4x4 matrices */
static int num_projmodelmtx;	/* currenct active matrix */

static float cur_projection[4][4];
static float cur_modelview[4][4];
static float cur_viewport[4][4];

/*--- Functions prototypes ---*/

static void set_viewport(int x, int y, int w, int h);
static void set_projection(float angle, float aspect,
	float z_near, float z_far);
static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up);
static void scale(float x, float y, float z);
static void translate(float x, float y, float z);
static void push_matrix(void);
static void pop_matrix(void);

static void set_color(SDL_Surface *surf, Uint32 color);
static void line(SDL_Surface *surf,
	float x1, float y1, float z1,
	float x2, float y2, float z2);

static void render_soft_shutdown(render_t *render);

/*--- Functions ---*/

void render_soft_init(render_t *render)
{
	render->set_viewport = set_viewport;
	render->set_projection = set_projection;
	render->set_modelview = set_modelview;
	render->scale = scale;
	render->translate = translate;
	render->push_matrix = push_matrix;
	render->pop_matrix = pop_matrix;

	render->set_color = set_color;
	render->line = line;

	render->initBackground = render_background_init;
	render->drawBackground = render_background;

	render->shutdown = render_soft_shutdown;

	num_projmodelmtx = 0;
	mtx_setIdentity(projmodelmtx[0]);
	mtx_setIdentity(cur_projection);
	mtx_setIdentity(cur_modelview);
	mtx_setIdentity(cur_viewport);
}

static void render_soft_shutdown(render_t *render)
{
	render_background_shutdown();
}

static void set_viewport(int x, int y, int w, int h)
{
	cur_viewport[0][0] = w;
	cur_viewport[1][1] = h;
	/* recalc viewport*modelview*projection */
}

static void set_projection(float angle, float aspect, float z_near, float z_far)
{
	float m[4][4];

	mtx_setProjection(m, angle, aspect, z_near, z_far);
	/* recalc viewport*modelview*projection */
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
	float m[4][4];

	mtx_setLookAt(m,
		x_from, y_from, z_from,
		x_to, y_to, z_to,
		x_up, y_up, z_up);

	translate(-x_from, -y_from, -z_from);
	/* recalc viewport*modelview*projection */
}

static void scale(float x, float y, float z)
{
	float sm[4][4];

	mtx_setIdentity(sm);
	sm[0][0] = x;
	sm[1][1] = y;
	sm[2][2] = z;
}

static void translate(float x, float y, float z)
{
	float tm[4][4];

	mtx_setIdentity(tm);
	tm[0][3] = x;
	tm[1][3] = y;
	tm[2][3] = z;
}

static void push_matrix(void)
{
	if (num_projmodelmtx==MAX_PROJMODELMTX-1) {
		return;
	}

	/* Copy current matrix in next position */
	memcpy(&projmodelmtx[num_projmodelmtx], &projmodelmtx[num_projmodelmtx+1], sizeof(projmodelmtx[0]));

	++num_projmodelmtx;
}

static void pop_matrix(void)
{
	if (num_projmodelmtx==0) {
		return;
	}

	--num_projmodelmtx;
}

static void set_color(SDL_Surface *surf, Uint32 color)
{
	render.color = SDL_MapRGBA(surf->format,
		(color>>16) & 0xff, (color>>8) & 0xff,
		color & 0xff, (color>>24) & 0xff);
}

static void line(SDL_Surface *surf,
	float x1, float y1, float z1,
	float x2, float y2, float z2)
{
}
