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

#include <stdio.h>

#include <SDL.h>

#ifdef ENABLE_OPENGL

#include <SDL_opengl.h>

#include "dyngl.h"

#include "video.h"
#include "render.h"
#include "render_texture_opengl.h"
#include "render_mesh_opengl.h"
#include "render_skel_opengl.h"
#include "matrix.h"
#include "render_texture_list.h"
#include "render_skel_list.h"

/*--- Variables ---*/

static int blending;
static int gouraud;

/*--- Functions prototypes ---*/

static void render_opengl_shutdown(render_t *render);

static void render_resize(render_t *this, int w, int h);
static void render_startFrame(render_t *this);
static void render_endFrame(render_t *this);

static void set_viewport(int x, int y, int w, int h);
static void set_projection(float angle, float aspect, float z_near, float z_far);
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

static void set_color(Uint32 color);
static void set_render(render_t *this, int num_render);
static void set_texture(int num_pal, render_texture_t *render_tex);
static void set_blending(int enable);
static void set_dithering(int enable);
static void set_useDirtyRects(int enable);
static void set_color_from_texture(vertex_t *v1);

static void sortBackToFront(int num_vtx, int *num_idx, vertex_t *vtx);

static void line(vertex_t *v1, vertex_t *v2);
static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void triangle_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void setRenderDepth(render_t *this, int show_depth);
static void copyDepthToColor(void);

/*--- Functions ---*/

void render_opengl_init(render_t *render)
{
	render->shutdown = render_opengl_shutdown;

	render->resize = render_resize;
	render->startFrame = render_startFrame;
	render->endFrame = render_endFrame;

	render->createTexture = render_texture_gl_create;
	render->createMesh = render_mesh_gl_create;
	render->createSkel = render_skel_gl_create;

	render->set_viewport = set_viewport;
	render->set_projection = set_projection;
	render->set_ortho = set_ortho;
	render->set_modelview = set_modelview;
	render->set_identity = set_identity;
	render->scale = scale;
	render->translate = translate;
	render->rotate = rotate;
	render->push_matrix = push_matrix;
	render->pop_matrix = pop_matrix;

	render->set_color = set_color;
	render->set_render = set_render;
	render->set_texture = set_texture;
	render->set_blending = set_blending;
	render->set_dithering = set_dithering;
	render->set_useDirtyRects = set_useDirtyRects;

	render->sortBackToFront = sortBackToFront;

	render_bitmap_opengl_init(&(render->bitmap));

	render->texture = NULL;
	render->tex_pal = 0;

	set_render(render, RENDER_WIREFRAME);
	blending = 0;
	gouraud = 0;
	render->useDirtyRects = 0;

	render->render_depth = 0;
	render->setRenderDepth = setRenderDepth;
	render->copyDepthToColor = copyDepthToColor;
}

static void render_opengl_shutdown(render_t *render)
{
	list_render_texture_shutdown();
	list_render_skel_shutdown();
}

static void render_resize(render_t *this, int w, int h)
{
}

static void render_startFrame(render_t *this)
{
	gl.ClearColor(0.0,0.0,0.0,0.0);
	gl.ClearDepth(1.0f);
	gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	gl.Enable(GL_DEPTH_TEST);

	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_FRONT);
}

static void render_endFrame(render_t *this)
{
	if (this->render_depth) {
		this->copyDepthToColor();
	}

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
	gl.LoadIdentity();
}

static void set_ortho(float left, float right, float bottom, float top,
	float p_near, float p_far)
{
	GLfloat m[4][4];

	mtx_setOrtho(m, left,right, bottom,top, p_near,p_far);

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.MultMatrixf(&m[0][0]);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
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

static void set_blending(int enable)
{
	blending = enable;

	if (enable) {
		gl.Enable(GL_BLEND);
		gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		gl.Enable(GL_ALPHA_TEST);
		gl.AlphaFunc(GL_GREATER, 0.5f);
	} else {
		gl.Disable(GL_BLEND);
		gl.Disable(GL_ALPHA_TEST);
	}
}

static void set_dithering(int enable)
{
	if (enable) {
		gl.Enable(GL_DITHER);
	} else {
		gl.Disable(GL_DITHER);
	}
}

static void set_useDirtyRects(int enable)
{
}

static void set_render(render_t *this, int num_render)
{
	this->line = line;
	this->triangle_wf = triangle;
	this->quad_wf = quad;

	this->render_mode = num_render;

	switch(num_render) {
		case RENDER_WIREFRAME:
			this->triangle = triangle;
			this->quad = quad;
			break;
		case RENDER_FILLED:
			gouraud = 0;
			this->triangle = triangle_fill;
			this->quad = quad_fill;
			break;
		case RENDER_GOURAUD:
			gouraud = 1;
			this->triangle = triangle_fill;
			this->quad = quad_fill;
			break;
		case RENDER_TEXTURED:
			this->triangle = triangle_tex;
			this->quad = quad_tex;
			break;
	}
}

static void set_color_from_texture(vertex_t *v1)
{
	render_texture_t *texture = render.texture;
	Uint32 color = 0xffffffff;

	if (!texture) {
		return;
	}

	if (texture->paletted) {
		Uint8 pix = texture->pixels[(texture->pitch * v1->v) + v1->u];
		
		color = texture->palettes[render.tex_pal][pix];
	} else {
		int r,g,b;
		Uint16 pix = ((Uint16 *) texture->pixels)[((texture->pitch>>1) * v1->v) + v1->u];

		r = (pix>>8) & 0xf8;
		r |= r>>5;
		g = (pix>>3) & 0xfc;
		g |= g>>6;
		b = (pix<<3) & 0xf8;
		b |= b>>5;
		color = (r<<16)|(g<<8)|b;
	}
	set_color(color);
}

/*
	Sort vertex back to front
*/
static void sortBackToFront(int num_vtx, int *num_idx, vertex_t *vtx)
{
}

/*
	Wireframe triangles/quads
*/

static void line(vertex_t *v1, vertex_t *v2)
{
	gl.Begin(GL_LINES);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.End();
}

static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	set_color_from_texture(v1);

	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	gl.Begin(GL_TRIANGLES);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.End();

	gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	set_color_from_texture(v1);

	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	gl.Begin(GL_QUADS);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.Vertex3s(v4->x, v4->y, v4->z);
	gl.End();

	gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/*
	Filled triangles/quads
*/

static void triangle_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	if (!gouraud) {
		set_color_from_texture(v1);
	} else {
		gl.ShadeModel(GL_SMOOTH);
	}

	gl.Begin(GL_TRIANGLES);
	if (gouraud) {
		set_color_from_texture(v1);
	}
	gl.Vertex3s(v1->x, v1->y, v1->z);
	if (gouraud) {
		set_color_from_texture(v2);
	}
	gl.Vertex3s(v2->x, v2->y, v2->z);
	if (gouraud) {
		set_color_from_texture(v3);
	}
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.End();

	gl.ShadeModel(GL_FLAT);
}

static void quad_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	if (!gouraud) {
		set_color_from_texture(v1);
	} else {
		gl.ShadeModel(GL_SMOOTH);
	}

	gl.Begin(GL_QUADS);
	if (gouraud) {
		set_color_from_texture(v1);
	}
	gl.Vertex3s(v1->x, v1->y, v1->z);
	if (gouraud) {
		set_color_from_texture(v2);
	}
	gl.Vertex3s(v2->x, v2->y, v2->z);
	if (gouraud) {
		set_color_from_texture(v3);
	}
	gl.Vertex3s(v3->x, v3->y, v3->z);
	if (gouraud) {
		set_color_from_texture(v4);
	}
	gl.Vertex3s(v4->x, v4->y, v4->z);
	gl.End();

	gl.ShadeModel(GL_FLAT);
}

/*
	Textured triangles/quads
*/

static void set_texture(int num_pal, render_texture_t *render_tex)
{
	render_texture_gl_t *gl_tex;

	render.tex_pal = num_pal;
	render.texture = render_tex;

	if (render_tex==NULL) {
		return;
	}
	gl_tex = (render_texture_gl_t *) render_tex;

	render_tex->upload(render_tex, num_pal);

 	gl.TexParameteri(gl_tex->textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 	gl.TexParameteri(gl_tex->textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 	gl.TexParameteri(gl_tex->textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 	gl.TexParameteri(gl_tex->textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	render_texture_t *texture = render.texture;
	render_texture_gl_t *gl_tex;	

	if (!texture) {
		return;
	}
	gl_tex = (render_texture_gl_t *) texture;

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.MatrixMode(GL_MODELVIEW);

	gl.Enable(gl_tex->textureTarget);

	gl.Begin(GL_TRIANGLES);
	if (gl_tex->textureTarget == GL_TEXTURE_2D) {
		gl.TexCoord2f((float) v1->u / texture->pitchw, (float) v1->v / texture->pitchh);
		gl.Vertex3s(v1->x, v1->y, v1->z);
		gl.TexCoord2f((float) v2->u / texture->pitchw, (float) v2->v / texture->pitchh);
		gl.Vertex3s(v2->x, v2->y, v2->z);
		gl.TexCoord2f((float) v3->u / texture->pitchw, (float) v3->v / texture->pitchh);
		gl.Vertex3s(v3->x, v3->y, v3->z);
	} else {
		gl.TexCoord2s(v1->u, v1->v);
		gl.Vertex3s(v1->x, v1->y, v1->z);
		gl.TexCoord2s(v2->u, v2->v);
		gl.Vertex3s(v2->x, v2->y, v2->z);
		gl.TexCoord2s(v3->u, v3->v);
		gl.Vertex3s(v3->x, v3->y, v3->z);
	}
	gl.End();

	gl.Disable(gl_tex->textureTarget);
}

static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	render_texture_t *texture = render.texture;
	render_texture_gl_t *gl_tex;	

	if (!texture) {
		return;
	}
	gl_tex = (render_texture_gl_t *) texture;

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.MatrixMode(GL_MODELVIEW);

	gl.Enable(gl_tex->textureTarget);

	gl.Begin(GL_QUADS);
	if (gl_tex->textureTarget == GL_TEXTURE_2D) {
		gl.TexCoord2f((float) v1->u / texture->pitchw, (float) v1->v / texture->pitchh);
		gl.Vertex3s(v1->x, v1->y, v1->z);
		gl.TexCoord2f((float) v2->u / texture->pitchw, (float) v2->v / texture->pitchh);
		gl.Vertex3s(v2->x, v2->y, v2->z);
		gl.TexCoord2f((float) v3->u / texture->pitchw, (float) v3->v / texture->pitchh);
		gl.Vertex3s(v3->x, v3->y, v3->z);
		gl.TexCoord2f((float) v4->u / texture->pitchw, (float) v4->v / texture->pitchh);
		gl.Vertex3s(v4->x, v4->y, v4->z);
	} else {
		gl.TexCoord2s(v1->u, v1->v);
		gl.Vertex3s(v1->x, v1->y, v1->z);
		gl.TexCoord2s(v2->u, v2->v);
		gl.Vertex3s(v2->x, v2->y, v2->z);
		gl.TexCoord2s(v3->u, v3->v);
		gl.Vertex3s(v3->x, v3->y, v3->z);
		gl.TexCoord2s(v4->u, v4->v);
		gl.Vertex3s(v4->x, v4->y, v4->z);
	}
	gl.End();

	gl.Disable(gl_tex->textureTarget);
}

static void setRenderDepth(render_t *this, int show_depth)
{
	this->render_depth = show_depth;
}

static void copyDepthToColor(void)
{
	GLsizei winWidth = video.viewport.w;
	GLsizei winHeight = video.viewport.h;
        GLfloat zBlack = 1.0f;
	GLfloat zWhite = 0.0f;

	gl.Disable(GL_CULL_FACE);
#if 0
   GLfloat *depthValues;

   /*assert(zBlack >= 0.0);
   assert(zBlack <= 1.0);
   assert(zWhite >= 0.0);
   assert(zWhite <= 1.0);
   assert(zBlack != zWhite);*/

   gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
   gl.PixelStorei(GL_PACK_ALIGNMENT, 1);

   /* Read depth values */
   depthValues = (GLfloat *) malloc(winWidth * winHeight * sizeof(GLfloat));
   /*assert(depthValues);*/
   gl.ReadPixels(0, 0, winWidth, winHeight, GL_DEPTH_COMPONENT,
                GL_FLOAT, depthValues);

   /* Map Z values from [zBlack, zWhite] to gray levels in [0, 1] */
   /* Not using glPixelTransfer() because it's broke on some systems! */
   if (zBlack != 0.0 || zWhite != 1.0) {
      GLfloat scale = 1.0 / (zWhite - zBlack);
      GLfloat bias = -zBlack * scale;
      int n = winWidth * winHeight;
      int i;
      for (i = 0; i < n; i++)
         depthValues[i] = depthValues[i] * scale + bias;
   }

   /* save GL state */
   gl.PushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT |
                GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

   /* setup raster pos for glDrawPixels */
   gl.MatrixMode(GL_PROJECTION);
   gl.PushMatrix();
   gl.LoadIdentity();

   gl.Ortho(0.0, (GLdouble) winWidth, 0.0, (GLdouble) winHeight, -1.0, 1.0);
   gl.MatrixMode(GL_MODELVIEW);
   gl.PushMatrix();
   gl.LoadIdentity();

   gl.Disable(GL_STENCIL_TEST);
   gl.Disable(GL_DEPTH_TEST);
   gl.RasterPos2f(0, 0);

   gl.DrawPixels(winWidth, winHeight, GL_LUMINANCE, GL_FLOAT, depthValues);

   gl.PopMatrix();
   gl.MatrixMode(GL_PROJECTION);
   gl.PopMatrix();
   free(depthValues);

   gl.PopAttrib();
#else
	GLint ShadowTexWidth = 1024 /*winWidth*/;
	GLint ShadowTexHeight = 512 /*winHeight*/;
	GLuint tid;

	/*gl.GenTextures(1, &tid);
	gl.BindTexture(GL_TEXTURE_2D, tid);*/

	GLuint *depth = (GLuint *)
		malloc(ShadowTexWidth * ShadowTexHeight * sizeof(GLuint));
	/*assert(depth);*/
	gl.ReadPixels(0, 0, ShadowTexWidth, ShadowTexHeight,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, depth);
	gl.TexImage2D(GL_TEXTURE_2D, 0, /*GL_LUMINANCE*/ GL_RGBA,
		ShadowTexWidth, ShadowTexHeight, 0,
		/*GL_LUMINANCE*/ GL_RGBA, GL_UNSIGNED_BYTE /*GL_UNSIGNED_INT*/, depth);
	free(depth);

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0f, video.viewport.w, 0.0f, video.viewport.h, 0.0f, 1.0f);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	gl.Translatef((float) -video.viewport.x,
		(float) -video.viewport.y,
		0.0f);
	gl.Scalef((float) ShadowTexWidth,
		(float) ShadowTexHeight,
		1.0f);

	gl.Disable(GL_DEPTH_TEST);
	gl.Enable(GL_TEXTURE_2D);

	gl.Disable(GL_TEXTURE_GEN_S);
	gl.Disable(GL_TEXTURE_GEN_T);

	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	gl.Begin(GL_POLYGON);
		gl.TexCoord2f(0.0f, 0.0f);  gl.Vertex2f(0.0f, 0.0f);
		gl.TexCoord2f(1.0f, 0.0f);  gl.Vertex2f(1.0f, 0.0f);
		gl.TexCoord2f(1.0f, 1.0f);  gl.Vertex2f(1.0f, 1.0f);
		gl.TexCoord2f(0.0f, 1.0f);  gl.Vertex2f(0.0f, 1.0f);
	gl.End();

	gl.Disable(GL_TEXTURE_2D);
	gl.Enable(GL_DEPTH_TEST);

	/*gl.DeleteTextures(1, &tid);*/
#endif
}

#else

#include "video.h"
#include "render.h"

void render_opengl_init(render_t *render)
{
}

#endif /* ENABLE_OPENGL */
