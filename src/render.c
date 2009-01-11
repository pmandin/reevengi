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
#include "draw.h"

/*--- Defines ---*/

#define MAX_MODELVIEW_MTX 16

/*--- Variables ---*/

static float modelview_mtx[MAX_MODELVIEW_MTX][4][4];	/* 16 4x4 matrices */
static int num_modelview_mtx;	/* current active matrix */

static float camera_mtx[4][4]; /* camera matrix */
static float projection_mtx[4][4];	/* projection matrix */
static float viewport_mtx[4][4]; /* viewport matrix */
static float frustum_mtx[4][4]; /* frustum = viewport*projection*camera */
static float clip_planes[6][4]; /* view frustum clip planes */

/*--- Functions prototypes ---*/

static void render_soft_shutdown(render_t *render);

static void set_viewport(int x, int y, int w, int h);
static void set_projection(float angle, float aspect,
	float z_near, float z_far);
static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up);
static void set_identity(void);
static void scale(float x, float y, float z);
static void translate(float x, float y, float z);
static void rotate(float angle, float x, float y, float z);
static void push_matrix(void);
static void pop_matrix(void);

static void recalc_frustum_mtx(void);

static void set_color(Uint32 color);
static void set_texture(int num_pal, render_texture_t *render_tex);

static void line(vertex_t *v1, vertex_t *v2);
static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

/*--- Functions ---*/

void render_soft_init(render_t *render)
{
	render->set_viewport = set_viewport;
	render->set_projection = set_projection;
	render->set_modelview = set_modelview;
	render->set_identity = set_identity;
	render->scale = scale;
	render->translate = translate;
	render->rotate = rotate;
	render->push_matrix = push_matrix;
	render->pop_matrix = pop_matrix;

	render->set_color = set_color;
	render->set_texture = set_texture;

	render->line = line;
	render->triangle = triangle;
	render->quad = quad;

	render->triangle_tex = triangle_tex;
	render->quad_tex = quad_tex;

	render->initBackground = render_background_init;
	render->drawBackground = render_background;

	render->shutdown = render_soft_shutdown;

	render->texture = NULL;

	num_modelview_mtx = 0;
	mtx_setIdentity(modelview_mtx[0]);
	mtx_setIdentity(projection_mtx);
	mtx_setIdentity(viewport_mtx);
	mtx_setIdentity(camera_mtx);
	mtx_setIdentity(frustum_mtx);
}

static void render_soft_shutdown(render_t *render)
{
	render_background_shutdown();
}

/* Recalculate frustum matrix = modelview*projection */
static void recalc_frustum_mtx(void)
{
	float projcam_mtx[4][4];

	mtx_mult(projection_mtx, camera_mtx, projcam_mtx);
	mtx_calcFrustumClip(projcam_mtx, clip_planes);

	mtx_mult(viewport_mtx, projcam_mtx, frustum_mtx);
}

static void set_viewport(int x, int y, int w, int h)
{
	viewport_mtx[0][0] = w*0.5f;
	viewport_mtx[3][0] = w*0.5f;
	viewport_mtx[1][1] = -h*0.5f;
	viewport_mtx[3][1] = h*0.5f;
}

static void set_projection(float angle, float aspect, float z_near, float z_far)
{
	mtx_setProjection(projection_mtx, angle, aspect, z_near, z_far);
	recalc_frustum_mtx();
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
	float tm[4][4], r[4][4];

	mtx_setLookAt(camera_mtx,
		x_from, y_from, z_from,
		x_to, y_to, z_to,
		x_up, y_up, z_up);
	/*translate(-x_from, -y_from, -z_from);*/

	/* Translate to cam position */
	mtx_setIdentity(tm);
	tm[3][0] = -x_from;
	tm[3][1] = -y_from;
	tm[3][2] = -z_from;
	mtx_mult(camera_mtx, tm, r);
	memcpy(camera_mtx, r, sizeof(float)*4*4);

	recalc_frustum_mtx();

	/* Reset matrix stack for modelview */
	num_modelview_mtx = 0;
	mtx_setIdentity(modelview_mtx[num_modelview_mtx]);
}

static void set_identity(void)
{
	mtx_setIdentity(modelview_mtx[num_modelview_mtx]);
}

static void scale(float x, float y, float z)
{
	float sm[4][4], r[4][4];

	mtx_setIdentity(sm);
	sm[0][0] = x;
	sm[1][1] = y;
	sm[2][2] = z;
	mtx_mult(modelview_mtx[num_modelview_mtx], sm, r);
	memcpy(modelview_mtx[num_modelview_mtx], r, sizeof(float)*4*4);
}

static void translate(float x, float y, float z)
{
	float tm[4][4], r[4][4];

	mtx_setIdentity(tm);
	tm[3][0] = x;
	tm[3][1] = y;
	tm[3][2] = z;
	mtx_mult(modelview_mtx[num_modelview_mtx], tm, r);
	memcpy(modelview_mtx[num_modelview_mtx], r, sizeof(float)*4*4);
}

static void rotate(float angle, float x, float y, float z)
{
	float rm[4][4], r[4][4];

	mtx_setRotation(rm, angle, x,y,z);

	mtx_mult(modelview_mtx[num_modelview_mtx], rm, r);
	memcpy(modelview_mtx[num_modelview_mtx], r, sizeof(float)*4*4);
}

static void push_matrix(void)
{
	if (num_modelview_mtx==MAX_MODELVIEW_MTX-1) {
		fprintf(stderr, "Matrix stack full\n");
		return;
	}

	/* Copy current matrix in next position */
	memcpy(modelview_mtx[num_modelview_mtx+1], modelview_mtx[num_modelview_mtx], sizeof(float)*4*4);
	++num_modelview_mtx;
}

static void pop_matrix(void)
{
	if (num_modelview_mtx==0) {
		fprintf(stderr, "Matrix stack empty\n");
		return;
	}

	--num_modelview_mtx;
}

static void set_color(Uint32 color)
{
	draw_setColor(color);
}

static void line(vertex_t *v1, vertex_t *v2)
{
	float segment[4][4], result[4][4];
	/*int clip_result;*/

	memset(segment, 0, sizeof(float)*4*4);
	segment[0][0] = v1->x;
	segment[0][1] = v1->y;
	segment[0][2] = v1->z;
	segment[0][3] = 1.0f;
	segment[1][0] = v2->x;
	segment[1][1] = v2->y;
	segment[1][2] = v2->z;
	segment[1][3] = 1.0f;

	/* Project in current modelview */
	mtx_mult(modelview_mtx[num_modelview_mtx], segment, result);

	/* Clip segment to viewport */
	if (mtx_clipSegment(result, clip_planes) == CLIPPING_OUTSIDE) {
		return;
	}

	/* Project against view frustum */
	mtx_mult(frustum_mtx, result, segment);

	draw_line(
		(int) (segment[0][0]/segment[0][2]),
		(int) (segment[0][1]/segment[0][2]),
		(int) (segment[1][0]/segment[1][2]),
		(int) (segment[1][1]/segment[1][2])
	);
}

static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	float segment[4][4], result[4][4];

	memset(segment, 0, sizeof(float)*4*4);
	segment[0][0] = v1->x;
	segment[0][1] = v1->y;
	segment[0][2] = v1->z;
	segment[0][3] = 1.0f;
	segment[1][0] = v2->x;
	segment[1][1] = v2->y;
	segment[1][2] = v2->z;
	segment[1][3] = 1.0f;
	segment[2][0] = v3->x;
	segment[2][1] = v3->y;
	segment[2][2] = v3->z;
	segment[2][3] = 1.0f;

	mtx_mult(modelview_mtx[num_modelview_mtx], segment, result);
	if (mtx_clipCheck(result, 3 , clip_planes) == CLIPPING_OUTSIDE) {
		return;
	}
	mtx_mult(frustum_mtx, result, segment);

	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	line(v1,v2);
	line(v2,v3);
	line(v3,v1);
}

static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	float segment[4][4], result[4][4];

	memset(segment, 0, sizeof(float)*4*4);
	segment[0][0] = v1->x;
	segment[0][1] = v1->y;
	segment[0][2] = v1->z;
	segment[0][3] = 1.0f;
	segment[1][0] = v2->x;
	segment[1][1] = v2->y;
	segment[1][2] = v2->z;
	segment[1][3] = 1.0f;
	segment[2][0] = v3->x;
	segment[2][1] = v3->y;
	segment[2][2] = v3->z;
	segment[2][3] = 1.0f;
	segment[3][0] = v4->x;
	segment[3][1] = v4->y;
	segment[3][2] = v4->z;
	segment[3][3] = 1.0f;

	mtx_mult(modelview_mtx[num_modelview_mtx], segment, result);
	if (mtx_clipCheck(result, 4 , clip_planes) == CLIPPING_OUTSIDE) {
		return;
	}
	mtx_mult(frustum_mtx, result, segment);

	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	line(v1,v2);
	line(v2,v3);
	line(v3,v4);
	line(v4,v1);
}

/*
	Draw textured triangle / quad (2 triangles)

	Generate polygon if triangle clipped
	Draw triangles, given parts of poly
*/

static void set_texture(int num_pal, render_texture_t *render_tex)
{
	render.texture = render_tex;
}

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	if (!render.texture) {
		return;
	}

	triangle(v1,v2,v3);
}

static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	if (!render.texture) {
		return;
	}

	triangle_tex(v1,v2,v3);
	triangle_tex(v3,v4,v1);
}
