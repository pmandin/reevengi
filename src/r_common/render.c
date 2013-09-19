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

#include "render.h"
#include "render_texture_list.h"
#include "render_skel_list.h"
#include "render_bitmap.h"
#include "render_texture.h"
#include "render_mesh.h"
#include "render_skel.h"
#include "render_mask.h"

/*--- Functions prototypes ---*/

static void shutdown(void);

static void resize(int w, int h, int bpp);
static void startFrame(void);
static void endFrame(void);

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

static void set_color(Uint32 color);
static void set_render(int num_render);
static void set_texture(int num_pal, render_texture_t *render_tex);
static void set_blending(int enable);
static void set_dithering(int enable);
static void set_depth(int enable);
static void set_useDirtyRects(int enable);

static void sortBackToFront(int num_vtx, int *num_idx, vertex_t *vtx);

static void line(vertex_t *v1, vertex_t *v2);
static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void setRenderDepth(int show_depth);
static void copyDepthToColor(void);

/*--- Functions ---*/

void render_init(render_t *this)
{
	memset(this, 0, sizeof(render_t));

	this->shutdown = shutdown;

	this->resize = resize;
	this->startFrame = startFrame;
	this->endFrame = endFrame;

	this->createTexture = render_texture_create;
	this->createMesh = render_mesh_create;
	this->createSkel = render_skel_create;
	this->createMask = render_mask_create;

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
	this->set_texture = set_texture;
	this->set_blending = set_blending;
	this->set_dithering = set_dithering;
	this->set_depth = set_depth;
	this->set_useDirtyRects = set_useDirtyRects;

	this->depth_test = 1;

	this->sortBackToFront = sortBackToFront;

	render_bitmap_init(&this->bitmap);

	this->tex_pal = -1;

	set_render(RENDER_WIREFRAME);

	this->setRenderDepth = setRenderDepth;
	this->copyDepthToColor = copyDepthToColor;

	this->line = line;
	this->triangle_wf = this->triangle = triangle;
	this->quad_wf = this->quad = quad;
}

static void shutdown(void)
{
	render.bitmap.shutdown(&render.bitmap);
	list_render_texture_shutdown();
	list_render_skel_shutdown();
}

static void resize(int w, int h, int bpp)
{
}

static void startFrame(void)
{
}

static void endFrame(void)
{
}

static void set_viewport(int x, int y, int w, int h)
{
}

static void set_projection(float angle, float aspect, float z_near, float z_far)
{
}

static void set_ortho(float left, float right, float bottom, float top,
	float p_near, float p_far)
{
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
}

static void set_identity(void)
{
}

static void scale(float x, float y, float z)
{
}

static void translate(float x, float y, float z)
{
}

static void rotate(float angle, float x, float y, float z)
{
}

static void push_matrix(void)
{
}

static void pop_matrix(void)
{
}

static void get_proj_matrix(float mtx[4][4])
{
}

static void get_model_matrix(float mtx[4][4])
{
}

static void set_proj_matrix(float mtx[4][4])
{
}

static void set_model_matrix(float mtx[4][4])
{
}

static void set_color(Uint32 color)
{
}

static void set_blending(int enable)
{
}

static void set_dithering(int enable)
{
}

static void set_depth(int enable)
{
}

static void set_useDirtyRects(int enable)
{
}

static void set_render(int num_render)
{
}

static void sortBackToFront(int num_vtx, int *num_idx, vertex_t *vtx)
{
}

static void line(vertex_t *v1, vertex_t *v2)
{
}

static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
}

static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
}

static void set_texture(int num_pal, render_texture_t *render_tex)
{
	render.tex_pal = num_pal;
	render.texture = render_tex;
}

static void setRenderDepth(int show_depth)
{
}

static void copyDepthToColor(void)
{
}
