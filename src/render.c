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

#define MAX_MODELVIEW_MTX 16

/*--- Variables ---*/

static float modelview_mtx[MAX_MODELVIEW_MTX][4][4];	/* 16 4x4 matrices */
static int num_modelview_mtx;	/* current active matrix */

static float projection_mtx[4][4];	/* projection matrix */
static float frustum_mtx[4][4];	/* modelview * projection matrix */
static float viewport_mtx[4][4]; /* viewport matrix */
static float clip_planes[6][4]; /* view frustum clip planes */
static SDL_Rect viewport;

/*--- Functions prototypes ---*/

static void render_soft_shutdown(render_t *render);

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

static void draw_line(SDL_Surface *surf, int x1, int y1, int x2, int y2);

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

	num_modelview_mtx = 0;
	mtx_setIdentity(modelview_mtx[0]);
	mtx_setIdentity(projection_mtx);
	mtx_setIdentity(frustum_mtx);
	mtx_setIdentity(viewport_mtx);
}

static void render_soft_shutdown(render_t *render)
{
	render_background_shutdown();
}

static void refresh_render_matrix(void)
{
	/* Recalculate frustum matrix = modelview*projection */
	mtx_mult(modelview_mtx[num_modelview_mtx], projection_mtx, frustum_mtx);
	mtx_calcFrustumClip(frustum_mtx, clip_planes);
}

static void set_viewport(int x, int y, int w, int h)
{
	viewport.x = x;
	viewport.y = y;
	viewport.w = w;
	viewport.h = h;

	viewport_mtx[0][0] = w;
	viewport_mtx[1][1] = h;
}

static void set_projection(float angle, float aspect, float z_near, float z_far)
{
	mtx_setProjection(projection_mtx, angle, aspect, z_near, z_far);
	refresh_render_matrix();
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
	mtx_setLookAt(modelview_mtx[num_modelview_mtx],
		x_from, y_from, z_from,
		x_to, y_to, z_to,
		x_up, y_up, z_up);

	translate(-x_from, -y_from, -z_from);
	refresh_render_matrix();
}

static void scale(float x, float y, float z)
{
	float sm[4][4], r[4][4];

	mtx_setIdentity(sm);
	sm[0][0] = x;
	sm[1][1] = y;
	sm[2][2] = z;
	mtx_mult(sm, modelview_mtx[num_modelview_mtx], r);
	memcpy(modelview_mtx[num_modelview_mtx], r, sizeof(float)*4*4);
	refresh_render_matrix();
}

static void translate(float x, float y, float z)
{
	float tm[4][4], r[4][4];

	mtx_setIdentity(tm);
	tm[0][3] = x;
	tm[1][3] = y;
	tm[2][3] = z;
	mtx_mult(tm, modelview_mtx[num_modelview_mtx], r);
	memcpy(modelview_mtx[num_modelview_mtx], r, sizeof(float)*4*4);
	refresh_render_matrix();
}

static void push_matrix(void)
{
	if (num_modelview_mtx==MAX_MODELVIEW_MTX-1) {
		return;
	}

	/* Copy current matrix in next position */
	memcpy(modelview_mtx[num_modelview_mtx], modelview_mtx[num_modelview_mtx+1], sizeof(float)*4*4);

	++num_modelview_mtx;
}

static void pop_matrix(void)
{
	if (num_modelview_mtx==0) {
		return;
	}

	--num_modelview_mtx;
	refresh_render_matrix();
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
	float segment[4][4], result[4][4];

	memset(segment, 0, sizeof(float)*4*4);
	segment[0][0] = x1;
	segment[0][1] = y1;
	segment[0][2] = z1;
	segment[1][0] = x2;
	segment[1][1] = y2;
	segment[1][2] = z2;

	/* Project segment in frustum */
	mtx_mult(frustum_mtx, segment, result);

	/* Check segment is partly in frustum */
	if (!mtx_checkClip(result, 2, clip_planes)) {
		return;
	}

	/* Project segment to viewport */
	memcpy(segment, result, sizeof(float)*4*4);
	mtx_mult(viewport_mtx, segment, result);

	draw_line(surf, result[0][0], result[0][1], result[1][0], result[1][1]);
}

/*--- Draw operations ---*/

static void draw_line(SDL_Surface *surf, int x1, int y1, int x2, int y2)
{
	/* Clip line to viewport */
}
