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

#include <SDL.h>

#include "../video.h"

#include "../r_common/render_texture_list.h"
#include "../r_common/render_skel_list.h"
#include "../r_common/render.h"

#include "matrix.h"
#include "dither.h"
#include "draw.h"
#include "draw_simple.h"
#include "draw_sbuffer.h"
#include "render_mask.h"
#include "render_mesh.h"
#include "render_texture.h"
#include "render_bitmap.h"

/*--- Defines ---*/

#define MAX_MODELVIEW_MTX 32

/*--- Variables ---*/

static float modelview_mtx[MAX_MODELVIEW_MTX][4][4];	/* 16 4x4 matrices */
static int num_modelview_mtx;	/* current active matrix */

static float camera_mtx[4][4]; /* camera matrix */
static float projection_mtx[4][4];	/* projection matrix */
static float viewport_mtx[4][4]; /* viewport matrix */
static float frustum_mtx[4][4]; /* frustum = viewport*projection*camera */
static float clip_planes[6][4]; /* view frustum clip planes */

static int gouraud;

/*--- Functions prototypes ---*/

static void (*baseShutdown)(void);
static void shutdown(void);

static void render_resize(int w, int h, int bpp);
static void render_startFrame(void);
static void render_flushFrame(void);
static void render_endFrame(void);

static void set_viewport(int x, int y, int w, int h);
static void set_projection(float angle, float aspect,
	float z_near, float z_far);
static void set_ortho(float left, float right, float bottom, float top,
	float p_near, float p_far);
static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up);
static void set_identity(void);
static void scale(float x, float y, float z);
static void translate(float x, float y, float z);
static void rotate(float angle, float x, float y, float z);
static void push_matrix(void);
static void pop_matrix(void);

static void get_proj_matrix(float mtx[4][4]);
static void get_model_matrix(float mtx[4][4]);
static void set_proj_matrix(float mtx[4][4]);
static void set_model_matrix(float mtx[4][4]);

static void recalc_frustum_mtx(void);

static void set_color(Uint32 color);
static void set_render(int num_render);
static void set_dithering(int enable);
static void set_depth(int enable);
static void set_useDirtyRects(int enable);

static Uint32 get_color_from_texture(vertex_t *v1);

static void sortBackToFront(int num_vtx, int *num_idx, vertex_t *vtx);

static void line(vertex_t *v1, vertex_t *v2);
static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void triangle_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void setRenderDepth(int show_depth);

/*--- Functions ---*/

void render_soft_init(render_t *this)
{
	render_init(this);
	baseShutdown = this->shutdown;

	this->shutdown = shutdown;

	this->resize = render_resize;
	this->startFrame = render_startFrame;
	this->flushFrame = render_flushFrame;
	this->endFrame = render_endFrame;

	this->createTexture = render_texture_soft_create;
	this->createMesh = render_mesh_soft_create;
	this->createSkel = render_skel_create;
	this->createMask = render_mask_soft_create;

	this->set_viewport = set_viewport;
	this->set_projection = set_projection;
	this->set_ortho = set_ortho;
	this->set_modelview = set_modelview;
	this->set_identity = set_identity;
	this->scale = scale;
	this->translate = translate;
	this->rotate = rotate;
	this->push_matrix = push_matrix;
	this->pop_matrix = pop_matrix;

	this->get_proj_matrix = get_proj_matrix;
	this->get_model_matrix = get_model_matrix;
	this->set_proj_matrix = set_proj_matrix;
	this->set_model_matrix = set_model_matrix;

	this->set_color = set_color;
	this->set_render = set_render;
	this->set_dithering = set_dithering;
	this->set_depth = set_depth;
	this->set_useDirtyRects = set_useDirtyRects;

	this->sortBackToFront = sortBackToFront;

	render_bitmap_soft_init(&this->bitmap);

	num_modelview_mtx = 0;
	mtx_setIdentity(modelview_mtx[0]);
	mtx_setIdentity(projection_mtx);
	mtx_setIdentity(viewport_mtx);
	mtx_setIdentity(camera_mtx);
	mtx_setIdentity(frustum_mtx);

	this->setRenderDepth = setRenderDepth;

	/*draw_init_simple(&draw);*/
	draw_init_sbuffer(&draw);

	gouraud = 0;
}

static void shutdown(void)
{
	draw.shutdown(&draw);
	baseShutdown();
}

static void render_resize(int w, int h, int bpp)
{
	draw.resize(&draw, w,h, bpp);
}

static void render_startFrame(void)
{
	draw.startFrame(&draw);
}

static void render_flushFrame(void)
{
	draw.flushFrame(&draw);
}

static void render_endFrame(void)
{
	if (render.render_depth) {
		render.copyDepthToColor();
	}

	draw.endFrame(&draw);
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
	mtx_setIdentity(camera_mtx);

	recalc_frustum_mtx();

	set_identity();
}

static void set_ortho(float left, float right, float bottom, float top,
	float p_near, float p_far)
{
	mtx_setOrtho(projection_mtx, left,right, bottom,top, p_near,p_far);
	mtx_setIdentity(camera_mtx);

	recalc_frustum_mtx();

	set_identity();
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

	set_identity();
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

static void get_proj_matrix(float mtx[4][4])
{
	memcpy(mtx, projection_mtx, sizeof(float)*4*4);
}

static void get_model_matrix(float mtx[4][4])
{
	memcpy(mtx, modelview_mtx[num_modelview_mtx], sizeof(float)*4*4);
}

static void set_proj_matrix(float mtx[4][4])
{
	memcpy(projection_mtx, mtx, sizeof(float)*4*4);

	recalc_frustum_mtx();
}

static void set_model_matrix(float mtx[4][4])
{
	memcpy(modelview_mtx[num_modelview_mtx], mtx, sizeof(float)*4*4);
}

static void set_color(Uint32 color)
{
	int r,g,b,a;
	SDL_Surface *surf = video.screen;

	a = (color>>24) & 0xff;
	r = (color>>16) & 0xff;
	g = (color>>8) & 0xff;
	b = color & 0xff;

	if (video.bpp==8) {
		render.color = dither_nearest_index(r,g,b);
	} else {
		render.color = SDL_MapRGBA(surf->format, r,g,b,a);
	}
}

static void set_dithering(int enable)
{
	render.dithering = enable;
}

static void set_depth(int enable)
{
	render.depth_test = enable;
}

static void set_useDirtyRects(int enable)
{
	render.useDirtyRects = enable;
}

static void set_render(int num_render)
{
	render.line = line;
	render.triangle_wf = triangle;
	render.quad_wf = quad;

	render.render_mode = num_render;

	switch(num_render) {
		case RENDER_WIREFRAME:
			render.triangle = triangle;
			render.quad = quad;
			break;
		case RENDER_FILLED:
			gouraud = 0;
			render.triangle = triangle_fill;
			render.quad = quad_fill;
			break;
		case RENDER_GOURAUD:
			gouraud = 1;
			render.triangle = triangle_fill;
			render.quad = quad_fill;
			break;
		case RENDER_TEXTURED:
			render.triangle = triangle_tex;
			render.quad = quad_tex;
			break;
	}
}

static Uint32 get_color_from_texture(vertex_t *v1)
{
	render_texture_t *texture = render.texture;
	Uint32 color = 0xffffffff;
	Uint8 r,g,b,a;
	SDL_PixelFormat *fmt;

	if (!texture) {
		return color;
	}

	fmt = &(texture->format);
	switch(texture->bpp) {
		case 1:
			{
				Uint8 pix = texture->pixels[(texture->pitch * v1->v) + v1->u];
				color = texture->palettes[render.tex_pal][pix];
				fmt = video.screen->format;
			}
			break;
		case 2:
			color = ((Uint16 *) texture->pixels)[((texture->pitch>>1) * v1->v) + v1->u];
			break;
		case 3:
			/* TODO */
			break;
		case 4:
			color = ((Uint32 *) texture->pixels)[((texture->pitch>>2) * v1->v) + v1->u];
			break;
	}

	SDL_GetRGBA(color, fmt, &r,&g,&b,&a);
	return (a<<24)|(r<<16)|(g<<8)|b;
}

/*
	Sort vertex back to front
*/
static void sortBackToFront(int num_vtx, int *num_idx, vertex_t *vtx)
{
	int i,j;
	vertexf_t vtx1[32], vtx2[32];

	if (num_vtx>=32) {
		num_vtx = 32;
	}

	/* Project vertex list in view frustum */
	for (i=0; i<num_vtx; i++) {
		vtx1[i].pos[0] = vtx[i].x;
		vtx1[i].pos[1] = vtx[i].y;
		vtx1[i].pos[2] = vtx[i].z;
		vtx1[i].pos[3] = 1.0f;
	}
	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], num_vtx, vtx1, vtx2);
	mtx_multMtxVtx(frustum_mtx, num_vtx, vtx2, vtx1);

	/* Then sort them */
	for (i=0; i<num_vtx-1; i++) {

		/* Find the farer, swap against the one at pos i */
		for (j=i+1; j<num_vtx; j++) {

			if (vtx1[i].pos[2]<vtx1[j].pos[2]) {
				vertex_t tmp_vertex;
				vertexf_t tmp_vertexf;
				int tmp_idx;
			
				/* Swap vtx at i,j */
				memcpy(&tmp_vertex, &vtx[i], sizeof(vertex_t));
				memcpy(&vtx[i], &vtx[j], sizeof(vertex_t));
				memcpy(&vtx[j], &tmp_vertex, sizeof(vertex_t));

				memcpy(&tmp_vertexf, &vtx1[i], sizeof(vertexf_t));
				memcpy(&vtx1[i], &vtx1[j], sizeof(vertexf_t));
				memcpy(&vtx1[j], &tmp_vertexf, sizeof(vertexf_t));

				tmp_idx = num_idx[i];
				num_idx[i] = num_idx[j];
				num_idx[j] = tmp_idx;
			}
		}
	}
}

/*
	Wireframe triangles/quads
*/

static void line(vertex_t *v1, vertex_t *v2)
{
#if 0
	float segment[4][4], result[4][4];
	draw_vertex_t v[2];

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

	v[0].x = segment[0][0]/segment[0][2];
	v[0].y = segment[0][1]/segment[0][2];
	v[1].x = segment[1][0]/segment[1][2];
	v[1].y = segment[1][1]/segment[1][2];

	draw.line(&draw, &v[0], &v[1]);
#else
	/*float segment[4][4], result[4][4];*/
	vertexf_t tri1[2], poly[16], poly2[16];
	int clip_result, /*i,*/ num_vtx;
	Uint32 color = 0xffffffff;

	if (render.texture) {
		color = get_color_from_texture(v1);
		set_color(color);
	} else {
		Uint8 r,g,b,a;

		SDL_GetRGBA(render.color, video.screen->format, &r,&g,&b,&a);
		color = (a<<24)|(r<<16)|(g<<8)|b;
	}

	tri1[0].pos[0] = v1->x;
	tri1[0].pos[1] = v1->y;
	tri1[0].pos[2] = v1->z;
	tri1[0].pos[3] = 1.0f;
	tri1[0].tx[0] = v1->u;
	tri1[0].tx[1] = v1->v;
	tri1[0].col[0] = (color>>16) & 0xff;
	tri1[0].col[1] = (color>>8) & 0xff;
	tri1[0].col[2] = color & 0xff;
	tri1[0].col[3] = (color>>24) & 0xff;

	tri1[1].pos[0] = v2->x;
	tri1[1].pos[1] = v2->y;
	tri1[1].pos[2] = v2->z;
	tri1[1].pos[3] = 1.0f;
	tri1[1].tx[0] = v2->u;
	tri1[1].tx[1] = v2->v;
	tri1[1].col[0] = tri1[0].col[0];
	tri1[1].col[1] = tri1[0].col[1];
	tri1[1].col[2] = tri1[0].col[2];
	tri1[1].col[3] = tri1[0].col[3];

	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], 2, tri1, poly);

	num_vtx = 2;
	clip_result = mtx_clipTriangle(poly, &num_vtx, poly2, clip_planes);
	if (clip_result == CLIPPING_OUTSIDE) {
		return;
	}

	/* Check face visible */
	/*memset(result, 0, sizeof(float)*4*4);
	for (i=0; i<3; i++) {
		result[i][0] = poly[i].pos[0];
		result[i][1] = poly[i].pos[1];
		result[i][2] = poly[i].pos[2];
		result[i][3] = poly[i].pos[3];
	}

	mtx_mult(frustum_mtx, result, segment);
	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}*/

	/* Project poly in frustum */
	mtx_multMtxVtx(frustum_mtx, num_vtx, poly2, poly);

	/* Draw polygon */
	draw.polyLine(&draw, poly, num_vtx);
#endif
}

static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
#if 0
	float segment[4][4], result[4][4];
	draw_vertex_t v[3];

	if (render.texture) {
		set_color(get_color_from_texture(v1));
	}

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

	v[0].x = segment[0][0]/segment[0][2];
	v[0].y = segment[0][1]/segment[0][2];
	v[1].x = segment[1][0]/segment[1][2];
	v[1].y = segment[1][1]/segment[1][2];
	v[2].x = segment[2][0]/segment[2][2];
	v[2].y = segment[2][1]/segment[2][2];

	draw.triangle(&draw, v);
#else
	float segment[4][4], result[4][4];
	vertexf_t tri1[3], poly[16], poly2[16];
	int clip_result, i, num_vtx;
	Uint32 color = 0xffffffff;

	if (render.texture) {
		color = get_color_from_texture(v1);
		set_color(color);
	} else {
		Uint8 r,g,b,a;

		SDL_GetRGBA(render.color, video.screen->format, &r,&g,&b,&a);
		color = (a<<24)|(r<<16)|(g<<8)|b;
	}

	/*color = get_color_from_texture(v1);
	set_color(color);*/

	tri1[0].pos[0] = v1->x;
	tri1[0].pos[1] = v1->y;
	tri1[0].pos[2] = v1->z;
	tri1[0].pos[3] = 1.0f;
	tri1[0].tx[0] = v1->u;
	tri1[0].tx[1] = v1->v;
	tri1[0].col[0] = (color>>16) & 0xff;
	tri1[0].col[1] = (color>>8) & 0xff;
	tri1[0].col[2] = color & 0xff;
	tri1[0].col[3] = (color>>24) & 0xff;

	tri1[1].pos[0] = v2->x;
	tri1[1].pos[1] = v2->y;
	tri1[1].pos[2] = v2->z;
	tri1[1].pos[3] = 1.0f;
	tri1[1].tx[0] = v2->u;
	tri1[1].tx[1] = v2->v;
	tri1[1].col[0] = tri1[0].col[0];
	tri1[1].col[1] = tri1[0].col[1];
	tri1[1].col[2] = tri1[0].col[2];
	tri1[1].col[3] = tri1[0].col[3];

	tri1[2].pos[0] = v3->x;
	tri1[2].pos[1] = v3->y;
	tri1[2].pos[2] = v3->z;
	tri1[2].pos[3] = 1.0f;
	tri1[2].tx[0] = v3->u;
	tri1[2].tx[1] = v3->v;
	tri1[2].col[0] = tri1[0].col[0];
	tri1[2].col[1] = tri1[0].col[1];
	tri1[2].col[2] = tri1[0].col[2];
	tri1[2].col[3] = tri1[0].col[3];

	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], 3, tri1, poly);

	num_vtx = 3;
	clip_result = mtx_clipTriangle(poly, &num_vtx, poly2, clip_planes);
	if (clip_result == CLIPPING_OUTSIDE) {
		return;
	}

	/* Check face visible */
	memset(result, 0, sizeof(float)*4*4);
	for (i=0; i<3; i++) {
		result[i][0] = poly[i].pos[0];
		result[i][1] = poly[i].pos[1];
		result[i][2] = poly[i].pos[2];
		result[i][3] = poly[i].pos[3];
	}

	mtx_mult(frustum_mtx, result, segment);
	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	/* Project poly in frustum */
	mtx_multMtxVtx(frustum_mtx, num_vtx, poly2, poly);

	/* Draw polygon */
	draw.polyLine(&draw, poly, num_vtx);
#endif
}

static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
#if 0
	float segment[4][4], result[4][4];
	draw_vertex_t v[4];

	if (render.texture) {
		set_color(get_color_from_texture(v1));
	}

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

	v[0].x = segment[0][0]/segment[0][2];
	v[0].y = segment[0][1]/segment[0][2];
	v[1].x = segment[1][0]/segment[1][2];
	v[1].y = segment[1][1]/segment[1][2];
	v[2].x = segment[2][0]/segment[2][2];
	v[2].y = segment[2][1]/segment[2][2];
	v[3].x = segment[3][0]/segment[3][2];
	v[3].y = segment[3][1]/segment[3][2];

	draw.quad(&draw, v);
#else
	float segment[4][4], result[4][4];
	vertexf_t tri1[4], poly[16], poly2[16];
	int clip_result, i, num_vtx;
	Uint32 color = 0xffffffff;

	if (render.texture) {
		color = get_color_from_texture(v1);
		set_color(color);
	} else {
		Uint8 r,g,b,a;

		SDL_GetRGBA(render.color, video.screen->format, &r,&g,&b,&a);
		color = (a<<24)|(r<<16)|(g<<8)|b;
	}

	tri1[0].pos[0] = v1->x;
	tri1[0].pos[1] = v1->y;
	tri1[0].pos[2] = v1->z;
	tri1[0].pos[3] = 1.0f;
	tri1[0].tx[0] = v1->u;
	tri1[0].tx[1] = v1->v;
	tri1[0].col[0] = (color>>16) & 0xff;
	tri1[0].col[1] = (color>>8) & 0xff;
	tri1[0].col[2] = color & 0xff;
	tri1[0].col[3] = (color>>24) & 0xff;

	tri1[1].pos[0] = v2->x;
	tri1[1].pos[1] = v2->y;
	tri1[1].pos[2] = v2->z;
	tri1[1].pos[3] = 1.0f;
	tri1[1].tx[0] = v2->u;
	tri1[1].tx[1] = v2->v;
	tri1[1].col[0] = tri1[0].col[0];
	tri1[1].col[1] = tri1[0].col[1];
	tri1[1].col[2] = tri1[0].col[2];
	tri1[1].col[3] = tri1[0].col[3];

	tri1[2].pos[0] = v3->x;
	tri1[2].pos[1] = v3->y;
	tri1[2].pos[2] = v3->z;
	tri1[2].pos[3] = 1.0f;
	tri1[2].tx[0] = v3->u;
	tri1[2].tx[1] = v3->v;
	tri1[2].col[0] = tri1[0].col[0];
	tri1[2].col[1] = tri1[0].col[1];
	tri1[2].col[2] = tri1[0].col[2];
	tri1[2].col[3] = tri1[0].col[3];

	tri1[3].pos[0] = v4->x;
	tri1[3].pos[1] = v4->y;
	tri1[3].pos[2] = v4->z;
	tri1[3].pos[3] = 1.0f;
	tri1[3].tx[0] = v4->u;
	tri1[3].tx[1] = v4->v;
	tri1[3].col[0] = tri1[0].col[0];
	tri1[3].col[1] = tri1[0].col[1];
	tri1[3].col[2] = tri1[0].col[2];
	tri1[3].col[3] = tri1[0].col[3];

	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], 4, tri1, poly);

	num_vtx = 4;
	clip_result = mtx_clipTriangle(poly, &num_vtx, poly2, clip_planes);
	if (clip_result == CLIPPING_OUTSIDE) {
		return;
	}

	/* Check face visible */
	memset(result, 0, sizeof(float)*4*4);
	for (i=0; i<3; i++) {
		result[i][0] = poly[i].pos[0];
		result[i][1] = poly[i].pos[1];
		result[i][2] = poly[i].pos[2];
		result[i][3] = poly[i].pos[3];
	}

	mtx_mult(frustum_mtx, result, segment);
	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	/* Project poly in frustum */
	mtx_multMtxVtx(frustum_mtx, num_vtx, poly2, poly);

	/* Draw polygon */
	draw.polyLine(&draw, poly, num_vtx);
#endif
}

/*
	Filled triangles/quads
*/

static void triangle_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	float segment[4][4], result[4][4];
	vertexf_t tri1[3], poly[16], poly2[16];
	int clip_result, i, num_vtx;
	Uint32 color;

	color = get_color_from_texture(v1);
	if (!gouraud) {
		set_color(color);
	}

	tri1[0].pos[0] = v1->x;
	tri1[0].pos[1] = v1->y;
	tri1[0].pos[2] = v1->z;
	tri1[0].pos[3] = 1.0f;
	tri1[0].tx[0] = v1->u;
	tri1[0].tx[1] = v1->v;
	tri1[0].col[0] = (color>>16) & 0xff;
	tri1[0].col[1] = (color>>8) & 0xff;
	tri1[0].col[2] = color & 0xff;
	tri1[0].col[3] = (color>>24) & 0xff;

	if (gouraud) {
		color = get_color_from_texture(v2);
	}
	tri1[1].pos[0] = v2->x;
	tri1[1].pos[1] = v2->y;
	tri1[1].pos[2] = v2->z;
	tri1[1].pos[3] = 1.0f;
	tri1[1].tx[0] = v2->u;
	tri1[1].tx[1] = v2->v;
	tri1[1].col[0] = (color>>16) & 0xff;
	tri1[1].col[1] = (color>>8) & 0xff;
	tri1[1].col[2] = color & 0xff;
	tri1[1].col[3] = (color>>24) & 0xff;

	if (gouraud) {
		color = get_color_from_texture(v3);
	}
	tri1[2].pos[0] = v3->x;
	tri1[2].pos[1] = v3->y;
	tri1[2].pos[2] = v3->z;
	tri1[2].pos[3] = 1.0f;
	tri1[2].tx[0] = v3->u;
	tri1[2].tx[1] = v3->v;
	tri1[2].col[0] = (color>>16) & 0xff;
	tri1[2].col[1] = (color>>8) & 0xff;
	tri1[2].col[2] = color & 0xff;
	tri1[2].col[3] = (color>>24) & 0xff;

	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], 3, tri1, poly);

	num_vtx = 3;
	clip_result = mtx_clipTriangle(poly, &num_vtx, poly2, clip_planes);
	if (clip_result == CLIPPING_OUTSIDE) {
		return;
	}

	/* Check face visible */
	memset(result, 0, sizeof(float)*4*4);
	for (i=0; i<3; i++) {
		result[i][0] = poly[i].pos[0];
		result[i][1] = poly[i].pos[1];
		result[i][2] = poly[i].pos[2];
		result[i][3] = poly[i].pos[3];
	}

	mtx_mult(frustum_mtx, result, segment);
	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	/* Project poly in frustum */
	mtx_multMtxVtx(frustum_mtx, num_vtx, poly2, poly);

	/* Draw polygon */
	if (gouraud) {
		draw.polyGouraud(&draw, poly, num_vtx);
	} else {
		draw.polyFill(&draw, poly, num_vtx);
	}
}

static void quad_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	float segment[4][4], result[4][4];
	vertexf_t tri1[4], poly[16], poly2[16];
	int clip_result, i, num_vtx;
	Uint32 color;

	color = get_color_from_texture(v1);
	if (!gouraud) {
		set_color(color);
	}

	tri1[0].pos[0] = v1->x;
	tri1[0].pos[1] = v1->y;
	tri1[0].pos[2] = v1->z;
	tri1[0].pos[3] = 1.0f;
	tri1[0].tx[0] = v1->u;
	tri1[0].tx[1] = v1->v;
	tri1[0].col[0] = (color>>16) & 0xff;
	tri1[0].col[1] = (color>>8) & 0xff;
	tri1[0].col[2] = color & 0xff;
	tri1[0].col[3] = (color>>24) & 0xff;

	if (gouraud) {
		color = get_color_from_texture(v2);
	}
	tri1[1].pos[0] = v2->x;
	tri1[1].pos[1] = v2->y;
	tri1[1].pos[2] = v2->z;
	tri1[1].pos[3] = 1.0f;
	tri1[1].tx[0] = v2->u;
	tri1[1].tx[1] = v2->v;
	tri1[1].col[0] = (color>>16) & 0xff;
	tri1[1].col[1] = (color>>8) & 0xff;
	tri1[1].col[2] = color & 0xff;
	tri1[1].col[3] = (color>>24) & 0xff;

	if (gouraud) {
		color = get_color_from_texture(v3);
	}
	tri1[2].pos[0] = v3->x;
	tri1[2].pos[1] = v3->y;
	tri1[2].pos[2] = v3->z;
	tri1[2].pos[3] = 1.0f;
	tri1[2].tx[0] = v3->u;
	tri1[2].tx[1] = v3->v;
	tri1[2].col[0] = (color>>16) & 0xff;
	tri1[2].col[1] = (color>>8) & 0xff;
	tri1[2].col[2] = color & 0xff;
	tri1[2].col[3] = (color>>24) & 0xff;

	if (gouraud) {
		color = get_color_from_texture(v4);
	}
	tri1[3].pos[0] = v4->x;
	tri1[3].pos[1] = v4->y;
	tri1[3].pos[2] = v4->z;
	tri1[3].pos[3] = 1.0f;
	tri1[3].tx[0] = v4->u;
	tri1[3].tx[1] = v4->v;
	tri1[3].col[0] = (color>>16) & 0xff;
	tri1[3].col[1] = (color>>8) & 0xff;
	tri1[3].col[2] = color & 0xff;
	tri1[3].col[3] = (color>>24) & 0xff;

	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], 4, tri1, poly);

	num_vtx = 4;
	clip_result = mtx_clipTriangle(poly, &num_vtx, poly2, clip_planes);
	if (clip_result == CLIPPING_OUTSIDE) {
		return;
	}

	/* Check face visible */
	memset(result, 0, sizeof(float)*4*4);
	for (i=0; i<3; i++) {
		result[i][0] = poly[i].pos[0];
		result[i][1] = poly[i].pos[1];
		result[i][2] = poly[i].pos[2];
		result[i][3] = poly[i].pos[3];
	}

	mtx_mult(frustum_mtx, result, segment);
	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	/* Project poly in frustum */
	mtx_multMtxVtx(frustum_mtx, num_vtx, poly2, poly);

	/* Draw polygon */
	if (gouraud) {
		draw.polyGouraud(&draw, poly, num_vtx);
	} else {
		draw.polyFill(&draw, poly, num_vtx);
	}
}

/*
	Textured triangles/quads
*/

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	float segment[4][4], result[4][4];
	vertexf_t tri1[3], poly[16], poly2[16];
	int clip_result, i, num_vtx;

	tri1[0].pos[0] = v1->x;
	tri1[0].pos[1] = v1->y;
	tri1[0].pos[2] = v1->z;
	tri1[0].pos[3] = 1.0f;
	tri1[0].tx[0] = v1->u;
	tri1[0].tx[1] = v1->v;

	tri1[1].pos[0] = v2->x;
	tri1[1].pos[1] = v2->y;
	tri1[1].pos[2] = v2->z;
	tri1[1].pos[3] = 1.0f;
	tri1[1].tx[0] = v2->u;
	tri1[1].tx[1] = v2->v;

	tri1[2].pos[0] = v3->x;
	tri1[2].pos[1] = v3->y;
	tri1[2].pos[2] = v3->z;
	tri1[2].pos[3] = 1.0f;
	tri1[2].tx[0] = v3->u;
	tri1[2].tx[1] = v3->v;

	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], 3, tri1, poly);

	num_vtx = 3;
	clip_result = mtx_clipTriangle(poly, &num_vtx, poly2, clip_planes);
	if (clip_result == CLIPPING_OUTSIDE) {
		return;
	}

	/* Check face visible */
	memset(result, 0, sizeof(float)*4*4);
	for (i=0; i<3; i++) {
		result[i][0] = poly[i].pos[0];
		result[i][1] = poly[i].pos[1];
		result[i][2] = poly[i].pos[2];
		result[i][3] = poly[i].pos[3];
	}

	mtx_mult(frustum_mtx, result, segment);
	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	/* Project poly in frustum */
	mtx_multMtxVtx(frustum_mtx, num_vtx, poly2, poly);

	/* Draw polygon */
	draw.polyTexture(&draw, poly, num_vtx);
}

static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	float segment[4][4], result[4][4];
	vertexf_t tri1[4], poly[16], poly2[16];
	int clip_result, i, num_vtx;

	tri1[0].pos[0] = v1->x;
	tri1[0].pos[1] = v1->y;
	tri1[0].pos[2] = v1->z;
	tri1[0].pos[3] = 1.0f;
	tri1[0].tx[0] = v1->u;
	tri1[0].tx[1] = v1->v;

	tri1[1].pos[0] = v2->x;
	tri1[1].pos[1] = v2->y;
	tri1[1].pos[2] = v2->z;
	tri1[1].pos[3] = 1.0f;
	tri1[1].tx[0] = v2->u;
	tri1[1].tx[1] = v2->v;

	tri1[2].pos[0] = v3->x;
	tri1[2].pos[1] = v3->y;
	tri1[2].pos[2] = v3->z;
	tri1[2].pos[3] = 1.0f;
	tri1[2].tx[0] = v3->u;
	tri1[2].tx[1] = v3->v;

	tri1[3].pos[0] = v4->x;
	tri1[3].pos[1] = v4->y;
	tri1[3].pos[2] = v4->z;
	tri1[3].pos[3] = 1.0f;
	tri1[3].tx[0] = v4->u;
	tri1[3].tx[1] = v4->v;

	mtx_multMtxVtx(modelview_mtx[num_modelview_mtx], 4, tri1, poly);

	num_vtx = 4;
	clip_result = mtx_clipTriangle(poly, &num_vtx, poly2, clip_planes);
	if (clip_result == CLIPPING_OUTSIDE) {
		return;
	}

	/* Check face visible */
	memset(result, 0, sizeof(float)*4*4);
	for (i=0; i<3; i++) {
		result[i][0] = poly[i].pos[0];
		result[i][1] = poly[i].pos[1];
		result[i][2] = poly[i].pos[2];
		result[i][3] = poly[i].pos[3];
	}

	mtx_mult(frustum_mtx, result, segment);
	if (mtx_faceVisible(segment)<0.0f) {
		return;
	}

	/* Project poly in frustum */
	mtx_multMtxVtx(frustum_mtx, num_vtx, poly2, poly);

	/* Draw polygon */
	draw.polyTexture(&draw, poly, num_vtx);
}

static void setRenderDepth(int show_depth)
{
	render.render_depth = show_depth;
}
