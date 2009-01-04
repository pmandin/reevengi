/*
	Render engine
	OpenGL

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_OPENGL

#include <stdio.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"

#include "video.h"
#include "render.h"
#include "render_background_opengl.h"
#include "matrix.h"

/*--- Functions prototypes ---*/

static void set_viewport(int x, int y, int w, int h);
static void set_projection(float angle, float aspect, float z_near, float z_far);
static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up);
static void set_identity(void);
static void scale(float x, float y, float z);
static void translate(float x, float y, float z);
static void rotate(float angle, float x, float y, float z);
static void push_matrix(void);
static void pop_matrix(void);

static void set_color(Uint32 color);
static void line(vertex_t *v1, vertex_t *v2);
static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void render_opengl_shutdown(render_t *render);

/*--- Functions ---*/

void render_opengl_init(render_t *render)
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
	render->line = line;
	render->triangle = triangle;
	render->quad = quad;

	render->initBackground = render_background_init_opengl;
	render->drawBackground = render_background_opengl;

	render->shutdown = render_opengl_shutdown;
}

static void render_opengl_shutdown(render_t *render)
{
}

static void set_viewport(int x, int y, int w, int h)
{
	gl.Viewport(x,y, w,h);
}

static void set_projection(float angle, float aspect, float z_near, float z_far)
{
	GLfloat m[4][4];

	mtx_setProjection(m, angle, aspect, z_near, z_far);

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.MultMatrixf(&m[0][0]);

	gl.MatrixMode(GL_MODELVIEW);
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
	GLfloat m[4][4];

	mtx_setLookAt(m,
		x_from, y_from, z_from,
		x_to, y_to, z_to,
		x_up, y_up, z_up);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.MultMatrixf(&m[0][0]);

	translate(-x_from, -y_from, -z_from);
}

static void set_identity(void)
{
	gl.LoadIdentity();
}

static void scale(float x, float y, float z)
{
	gl.Scalef(x,y,z);
}

static void translate(float x, float y, float z)
{
	gl.Translatef(x,y,z);
}

static void rotate(float angle, float x, float y, float z)
{
	gl.Rotatef(angle, x,y,z);
}

static void push_matrix(void)
{
	gl.PushMatrix();
}

static void pop_matrix(void)
{
	gl.PopMatrix();
}

static void set_color(Uint32 color)
{
	gl.Color4ub((color>>16) & 0xff, (color>>8) & 0xff,
		color & 0xff, (color>>24) & 0xff);
}

static void line(vertex_t *v1, vertex_t *v2)
{
	gl.Begin(GL_LINES);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.End();
}

static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_FRONT);

	gl.Begin(GL_TRIANGLES);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.End();

	gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	gl.Disable(GL_CULL_FACE);
}

static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_FRONT);

	gl.Begin(GL_QUADS);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.Vertex3s(v4->x, v4->y, v4->z);
	gl.End();

	gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	gl.Disable(GL_CULL_FACE);
}

#else

#include "video.h"
#include "render.h"

void render_opengl_init(render_t *render)
{
}

#endif /* ENABLE_OPENGL */
