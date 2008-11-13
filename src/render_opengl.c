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

#include <math.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "dyngl.h"

#include "video.h"
#include "render.h"
#include "render_background_opengl.h"

/*--- Functions prototypes ---*/

static void set_viewport(int x, int y, int w, int h);
static void set_projection(float angle, float aspect, float z_near, float z_far);
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

static void render_opengl_shutdown(render_t *render);

/*--- Functions ---*/

void render_opengl_init(render_t *render)
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
	float sine, cotangent, deltaZ;
	float radians = angle / 2 * M_PI / 180;

	deltaZ = z_far - z_near;
	sine = sin(radians);
	if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
		return;
	}

	cotangent = cos(radians) / sine;

	memset(m, 0, sizeof(GLfloat)*4*4);
	m[0][0] = cotangent / aspect;
	m[1][1] = cotangent;
	m[2][2] = -(z_far + z_near) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * z_near * z_far / deltaZ;
	m[3][3] = 0;

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.MultMatrixf(&m[0][0]);
	/*gluPerspective(angle, aspect, z_near, z_far);*/
	gl.MatrixMode(GL_MODELVIEW);
/*
	f = cotangent(fovy/2) = 1 / tan(fovy/2)

	   f
	------	0	0	0
	aspect

	0	f	0	0

	0	0	zf+zn	2*zf*zn
			-----   -------
			zn-zf	zn-zf

	0	0	-1	0
*/
}

static void normalize(float v[3])
{
    float r;

    r = sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
    if (r == 0.0) return;

    v[0] /= r;
    v[1] /= r;
    v[2] /= r;
}

static void cross(float v1[3], float v2[3], float result[3])
{
    result[0] = v1[1]*v2[2] - v1[2]*v2[1];
    result[1] = v1[2]*v2[0] - v1[0]*v2[2];
    result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

static void set_modelview(float x_from, float y_from, float z_from,
	float x_to, float y_to, float z_to,
	float x_up, float y_up, float z_up)
{
	float forward[3], side[3], up[3];
	GLfloat m[4][4];

	forward[0] = x_to - x_from;
	forward[1] = y_to - y_from;
	forward[2] = z_to - z_from;

	up[0] = x_up;
	up[1] = y_up;
	up[2] = z_up;

	normalize(forward);

	/* Side = forward x up */
	cross(forward, up, side);
	normalize(side);

	/* Recompute up as: up = side x forward */
	cross(side, forward, up);

	memset(m, 0, sizeof(GLfloat)*4*4);
	m[0][0] = side[0];
	m[1][0] = side[1];
	m[2][0] = side[2];

	m[0][1] = up[0];
	m[1][1] = up[1];
	m[2][1] = up[2];

	m[0][2] = -forward[0];
	m[1][2] = -forward[1];
	m[2][2] = -forward[2];

	m[3][3] = 1.0;

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.MultMatrixf(&m[0][0]);
	gl.Translatef(-x_from, -y_from, -z_from);
/*    
    	gluLookAt(x_from, y_from, z_from,
		x_to, y_to, z_to,
		x_up, y_up, z_up);

    Let E be the 3d column vector (eyeX, eyeY, eyeZ).
    Let C be the 3d column vector (centerX, centerY, centerZ).
    Let U be the 3d column vector (upX, upY, upZ).
    Compute L = C - E.
    Normalize L.
    Compute S = L x U.
    Normalize S.
    Compute U' = S x L.

M is the matrix whose columns are, in order:

    (S, 0), (U', 0), (-L, 0), (-E, 1)  (all column vectors)
*/
}

static void scale(float x, float y, float z)
{
	gl.Scalef(x,y,z);
}

static void translate(float x, float y, float z)
{
	gl.Translatef(x,y,z);
}

static void push_matrix(void)
{
	gl.PushMatrix();
}

static void pop_matrix(void)
{
	gl.PopMatrix();
}

static void set_color(SDL_Surface *surf, Uint32 color)
{
	gl.Color4ub((color>>16) & 0xff, (color>>8) & 0xff,
		color & 0xff, (color>>24) & 0xff);
}

static void line(SDL_Surface *surf,
	float x1, float y1, float z1,
	float x2, float y2, float z2)
{
	gl.Begin(GL_LINES);
	gl.Vertex3f(x1,y1,z1);
	gl.Vertex3f(x2,y2,z2);
	gl.End();
}

#else

void render_opengl_init(render_t *render)
{
}

#endif /* ENABLE_OPENGL */
