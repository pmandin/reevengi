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

static float projection_mtx[4][4];	/* projection matrix */
static float frustum_mtx[4][4];	/* modelview * projection matrix */
static float viewport_mtx[4][4]; /* viewport matrix */
static float clip_planes[6][4]; /* view frustum clip planes */

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

static void recalc_frustum_mtx(void);

static void set_color(Uint32 color);
static void line(
	float x1, float y1, float z1,
	float x2, float y2, float z2);

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

/* Recalculate frustum matrix = modelview*projection */
static void recalc_frustum_mtx(void)
{
	mtx_mult(projection_mtx, modelview_mtx[num_modelview_mtx], frustum_mtx);
	mtx_calcFrustumClip(frustum_mtx, clip_planes);

	/*printf("projection_mtx\n");
	mtx_print(projection_mtx);
	printf("modelview_mtx[%d]\n", num_modelview_mtx);
	mtx_print(modelview_mtx[num_modelview_mtx]);
	printf("frustum_mtx\n");
	mtx_print(frustum_mtx);*/
}

static void set_viewport(int x, int y, int w, int h)
{
	/*printf("set_viewport\n");*/

	viewport_mtx[0][0] = w/2;
	viewport_mtx[3][0] = w/2;
	viewport_mtx[1][1] = -h/2;
	viewport_mtx[3][1] = h/2;

	/*printf("viewport_mtx\n");
	mtx_print(viewport_mtx);*/
}

static void set_projection(float angle, float aspect, float z_near, float z_far)
{
	/*printf("set_projection\n");*/

	mtx_setProjection(projection_mtx, angle, aspect, z_near, z_far);

	recalc_frustum_mtx();
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
	/*printf("set_camera\n");
	printf("from %.3f,%.3f,%.3f\n", x_from, y_from, z_from);
	printf("to %.3f,%.3f,%.3f\n", x_to, y_to, z_to);
	printf("up %.3f,%.3f,%.3f\n", x_up, y_up, z_up);*/

	mtx_setLookAt(modelview_mtx[num_modelview_mtx],
		x_from, y_from, z_from,
		x_to, y_to, z_to,
		x_up, y_up, z_up);

	translate(-x_from, -y_from, -z_from);

	recalc_frustum_mtx();
}

static void scale(float x, float y, float z)
{
	float sm[4][4], r[4][4];

	/*printf("scale %.3f,%.3f,%.3f\n",x,y,z);*/

	mtx_setIdentity(sm);
	sm[0][0] = x;
	sm[1][1] = y;
	sm[2][2] = z;
	mtx_mult(modelview_mtx[num_modelview_mtx], sm, r);
	memcpy(modelview_mtx[num_modelview_mtx], r, sizeof(float)*4*4);

	/*printf("modelview_mtx[%d]\n", num_modelview_mtx);
	mtx_print(modelview_mtx[num_modelview_mtx]);*/
}

static void translate(float x, float y, float z)
{
	float tm[4][4], r[4][4];

	/*printf("translate %.3f,%.3f,%.3f\n",x,y,z);*/

	mtx_setIdentity(tm);
	tm[3][0] = x;
	tm[3][1] = y;
	tm[3][2] = z;
	mtx_mult(modelview_mtx[num_modelview_mtx], tm, r);
	memcpy(modelview_mtx[num_modelview_mtx], r, sizeof(float)*4*4);

	/*printf("modelview_mtx[%d]\n", num_modelview_mtx);
	mtx_print(modelview_mtx[num_modelview_mtx]);*/
}

static void push_matrix(void)
{
	if (num_modelview_mtx==MAX_MODELVIEW_MTX-1) {
		return;
	}

	/* Copy current matrix in next position */
	/*printf("push_matrix\n");*/
	memcpy(modelview_mtx[num_modelview_mtx+1], modelview_mtx[num_modelview_mtx], sizeof(float)*4*4);

	++num_modelview_mtx;
}

static void pop_matrix(void)
{
	if (num_modelview_mtx==0) {
		return;
	}

	/*printf("pop_matrix\n");*/
	--num_modelview_mtx;

	/*printf("modelview_mtx[%d]\n", num_modelview_mtx);
	mtx_print(modelview_mtx[num_modelview_mtx]);*/
}

static void set_color(Uint32 color)
{
	draw_setColor(color);
}

static void line(
	float x1, float y1, float z1,
	float x2, float y2, float z2)
{
	float segment[4][4], result[4][4];
	int clip_result;

	printf("segment %.3f,%.3f,%.3f -> %.3f,%.3f,%.3f\n",x1,y1,z1,x2,y2,z2);

	memset(segment, 0, sizeof(float)*4*4);
	segment[0][0] = x1;
	segment[0][1] = y1;
	segment[0][2] = z1;
	segment[0][3] = 1.0;
	segment[1][0] = x2;
	segment[1][1] = y2;
	segment[1][2] = z2;
	segment[1][3] = 1.0;
	/*mtx_print(segment);*/

	/* Project against current modelview */
	mtx_mult(modelview_mtx[num_modelview_mtx], segment, result);
	memcpy(segment, result, sizeof(float)*4*4);
	/*printf("segment -> modelview\n");
	mtx_print(segment);*/

	/* Project segment in frustum */
	mtx_mult(projection_mtx, segment, result);
	/*printf("segment -> frustum\n");
	mtx_print(result);*/

	/* Homogenous -> Normalize segment */
	result[0][0] /= result[0][3];
	result[0][1] /= result[0][3];
	result[0][2] /= result[0][3];
	result[0][3] = 1.0;
	result[1][0] /= result[1][3];
	result[1][1] /= result[1][3];
	result[1][2] /= result[1][3];
	result[1][3] = 1.0;

	printf("projected %.3f,%.3f,%.3f -> %.3f,%.3f,%.3f\n",
		result[0][0],result[0][1],result[0][2],
		result[1][0],result[1][1],result[1][2]);

	/* Check segment is partly in frustum */
	clip_result = mtx_clipCheck(result, 2, clip_planes);
	switch(clip_result) {
		case CLIPPING_OUTSIDE:
			printf("outside\n");
			break;
		case CLIPPING_NEEDED:
			/*printf("must clip\n");*/
			mtx_clipSegment(result, 2, clip_planes);
			break;
		default:
			/*printf("inside\n");*/
			break;
	}

	printf("clipped %.3f,%.3f,%.3f -> %.3f,%.3f,%.3f\n",
		result[0][0],result[0][1],result[0][2],
		result[1][0],result[1][1],result[1][2]);
	/*mtx_print(result);*/

	/* Project segment to viewport */
	memcpy(segment, result, sizeof(float)*4*4);
	mtx_mult(viewport_mtx, segment, result);

	/*printf("viewport %.3f,%.3f,%.3f -> %.3f,%.3f,%.3f\n",
		result[0][0],result[0][1],result[0][2],
		result[1][0],result[1][1],result[1][2]);*/

	draw_line(
		(int) (result[0][0]/result[0][2]),
		(int) (result[0][1]/result[0][2]),
		(int) (result[1][0]/result[1][2]),
		(int) (result[1][1]/result[1][2])
	);
}

/*
	viewport 320x240

	projection 60 deg, 4/3, 1.0, 100000.0

	from 11268.000,-2664.000,-3600.000
	to 22356.000,-2466.000,522.000
	up 0.000,-1.000,0.000

	cam switch
	push
		scale	1.0,100.0,1.0
		segment 10800.000,20.000,-7400.000 -> 12600.000,20.000,7800.000
		outside
		segment 12600.000,20.000,7800.000 -> 18100.000,20.000,7600.000
		clipped
		segment 18100.000,20.000,7600.000 -> 15999.000,20.000,-7400.000
		outside
		segment 15999.000,20.000,-7400.000 -> 10800.000,20.000,-7400.000
		outside
	pop

	origin
	push
		translate 22356.000,-2466.000,522.000
		scale 3000.000,3000.000,3000.000
		segment 0.000,0.000,0.000 -> 1.000,0.000,0.000
		inside
		segment 0.000,0.000,0.000 -> 0.000,1.000,0.000
		inside
		segment 0.000,0.000,0.000 -> 0.000,0.000,1.000
		inside
	pop	

View
	320	0	0	160	*	vx	= vx*320+160
	0	-240	0	120		vy	= vy*-240+120
	0	0	1	0		vz	= vz
	0	0	0	1		vw	= vw
Proj
	1.299	0	0	0
	0	1.732	0	0
	0	0	-1	-2
	0	0	-1	0	
Cam
	0.348	0	-0.937	-7300.752
	0.016	-1	0.006	-2819.391
	-0.937	-0.017	-0.348	9261.467
	0	0	0	1

projected -0.000,0.000,1.000 -> -0.000,-4.213,1.000
Clipping against plane 2 0.052,0.998,0.019,0.225
clipped -0.000,0.000,1.000 -> -0.000,-0.245,1.000

projected -0.000,0.000,1.000 -> -0.000,-2.847,1.000
Clipping against plane 2 0.078,0.997,0.029,0.337
clipped -0.000,0.000,1.000 -> -0.000,-0.367,1.000

projected -0.000,0.000,1.000 -> -0.000,-5.542,1.000
Clipping against plane 2 0.039,0.999,0.014,0.169
clipped -0.000,0.000,1.000 -> -0.000,-0.184,1.000
*/
