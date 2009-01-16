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

/*--- Variables ---*/

static GLuint tex_obj = (GLuint) -1;
static int blending;

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
static void set_render(render_t *this, int num_render);
static void set_texture(int num_pal, render_texture_t *render_tex);
static void set_blending(int enable);

static void line(vertex_t *v1, vertex_t *v2);
static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void triangle_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3);
static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

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
	render->set_render = set_render;
	render->set_texture = set_texture;
	render->set_blending = set_blending;

	render->initBackground = render_background_init_opengl;
	render->drawBackground = render_background_opengl;

	render->texture = NULL;
	render->tex_pal = -1;

	render->shutdown = render_opengl_shutdown;

	set_render(render, RENDER_WIREFRAME);
	blending = 0;
}

static void render_opengl_shutdown(render_t *render)
{
	/* Delete texture obj ? */
	if (tex_obj != (GLuint) -1) {
		gl.DeleteTextures(1, &tex_obj);
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

static void set_render(render_t *this, int num_render)
{
	this->line = line;
	this->triangle_wf = triangle;
	this->quad_wf = quad;

	switch(num_render) {
		case RENDER_WIREFRAME:
			this->triangle = triangle;
			this->quad = quad;
			break;
		case RENDER_FILLED:
			this->triangle = triangle_fill;
			this->quad = quad_fill;
			break;
		case RENDER_TEXTURED:
			this->triangle = triangle_tex;
			this->quad = quad_tex;
			break;
	}
}

/*
	Wireframe triangles/quads
*/

static void line(vertex_t *v1, vertex_t *v2)
{
	gl.Disable(GL_DEPTH_TEST);

	gl.Begin(GL_LINES);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.End();
}

static void triangle(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	gl.Disable(GL_DEPTH_TEST);

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
	gl.Disable(GL_DEPTH_TEST);

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

/*
	Filled triangles/quads
*/

static void triangle_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	render_texture_t *texture = render.texture;
	Uint32 color;

	if (!texture) {
		return;
	}

	if (texture->paletted) {
		Uint8 pix;
		
		pix = texture->pixels[(texture->pitch * v1->v) + v1->u];
		color = texture->palettes[pix][render.tex_pal];
	} else {
		int r,g,b;
		Uint16 pix;
		
		pix = ((Uint16 *) texture->pixels)[((texture->pitch>>1) * v1->v) + v1->u];
		r = (pix>>8) & 0xf8;
		r |= r>>5;
		g = (pix>>3) & 0xfc;
		g |= g>>6;
		b = (pix<<3) & 0xf8;
		b |= b>>5;
		color = (r<<16)|(g<<8)|b;
	}
	set_color(color);

	gl.Enable(GL_DEPTH_TEST);

	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_FRONT);

	gl.Begin(GL_TRIANGLES);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.End();

	gl.Disable(GL_CULL_FACE);
}

static void quad_fill(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	render_texture_t *texture = render.texture;
	Uint32 color, offset;

	if (!texture) {
		return;
	}

	if (texture->paletted) {
		Uint8 pix;

		pix = texture->pixels[(texture->pitch * v1->v) + v1->u];
		color = texture->palettes[pix][render.tex_pal];
	} else {
		int r,g,b;
		Uint16 pix;
		
		pix = ((Uint16 *) texture->pixels)[((texture->pitch>>1) * v1->v) + v1->u];
		r = (pix>>8) & 0xf8;
		r |= r>>5;
		g = (pix>>3) & 0xfc;
		g |= g>>6;
		b = (pix<<3) & 0xf8;
		b |= b>>5;
		color = (r<<16)|(g<<8)|b;
	}
	set_color(color);

	gl.Enable(GL_DEPTH_TEST);

	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_FRONT);

	gl.Begin(GL_QUADS);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.Vertex3s(v4->x, v4->y, v4->z);
	gl.End();

	gl.Disable(GL_CULL_FACE);
}

/*
	Textured triangles/quads
*/

static void set_blending(int enable)
{
	blending = enable;

	if (enable) {
		gl.Enable(GL_BLEND);
		gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		gl.Disable(GL_BLEND);
	}
}

static void set_texture(int num_pal, render_texture_t *render_tex)
{
	int reupload_tex = 0, i;
	GLenum internalFormat = GL_RGBA;
	GLenum pixelType = GL_UNSIGNED_BYTE;
	GLenum surfaceFormat = GL_RGBA;

	if (num_pal!=render.tex_pal) {
		render.tex_pal = num_pal;
		reupload_tex = 1;
	}
	if (render_tex!=render.texture) {
		render.texture = render_tex;
		reupload_tex = 1;
	}
	if (tex_obj == (GLuint) -1) {
		gl.GenTextures(1, &tex_obj);
		reupload_tex = 1;
	}

	if (!reupload_tex || !render.texture) {
		return;
	}

	gl.BindTexture(GL_TEXTURE_2D, tex_obj);

 	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
 	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
		blending ? GL_DECAL : GL_REPLACE);

	/* Upload new palette */
	if (render_tex->paletted) {
		surfaceFormat = GL_COLOR_INDEX;

#ifdef GL_EXT_paletted_texture
		/* FIXME: check GL_EXT_paletted_texture presence */
		if (1) {
			Uint8 mapP[256*4];
			Uint8 *pMap = mapP;

			internalFormat = GL_COLOR_INDEX8_EXT;
			for (i=0; i<256; i++) {
				Uint32 color = render_tex->palettes[i][num_pal];

				*pMap++ = (color>>16) & 0xff;
				*pMap++ = (color>>8) & 0xff;
				*pMap++ = color & 0xff;
				*pMap++ = (color>>24) & 0xff;
			}
			gl.ColorTableEXT(GL_TEXTURE_2D, GL_RGBA, 256, 
				GL_RGBA, GL_UNSIGNED_BYTE, mapP);
		} else
#endif
		{
			GLfloat mapR[256], mapG[256], mapB[256], mapA[256];

			memset(mapR, 0, sizeof(mapR));
			memset(mapG, 0, sizeof(mapG));
			memset(mapB, 0, sizeof(mapB));
			memset(mapA, 0, sizeof(mapA));
			for (i=0; i<256; i++) {
				Uint32 color = render_tex->palettes[i][num_pal];

				mapR[i] = ((color>>16) & 0xff) / 255.0;
				mapG[i] = ((color>>8) & 0xff) / 255.0;
				mapB[i] = (color & 0xff) / 255.0;
				mapA[i] = ((color>>24) & 0xff) / 255.0;
			}
			gl.PixelTransferi(GL_MAP_COLOR, GL_TRUE);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, mapR);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, mapG);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, mapB);
			gl.PixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, mapA);
		}
	} else {
		pixelType = GL_UNSIGNED_SHORT_5_5_5_1;
	}

	gl.TexImage2D(GL_TEXTURE_2D,0, internalFormat,
		render_tex->pitchw, render_tex->pitchh, 0,
		surfaceFormat, pixelType, render_tex->pixels
	);

	/*gl.PixelTransferi(GL_MAP_COLOR, GL_FALSE);*/
}

static void triangle_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3)
{
	render_texture_t *texture = render.texture;

	if (!texture) {
		return;
	}

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.MatrixMode(GL_MODELVIEW);

	gl.Enable(GL_DEPTH_TEST);

	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_FRONT);

	gl.Enable(GL_TEXTURE_2D);

	gl.BindTexture(GL_TEXTURE_2D, tex_obj);

	gl.Begin(GL_TRIANGLES);
	gl.TexCoord2f((float) v1->u / texture->pitchw, (float) v1->v / texture->pitchh);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.TexCoord2f((float) v2->u / texture->pitchw, (float) v2->v / texture->pitchh);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.TexCoord2f((float) v3->u / texture->pitchw, (float) v3->v / texture->pitchh);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.End();

	gl.Disable(GL_TEXTURE_2D);
	gl.Disable(GL_CULL_FACE);
}

static void quad_tex(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4)
{
	render_texture_t *texture = render.texture;

	if (!texture) {
		return;
	}

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.MatrixMode(GL_MODELVIEW);

	gl.Enable(GL_DEPTH_TEST);

	gl.Enable(GL_CULL_FACE);
	gl.CullFace(GL_FRONT);

	gl.Enable(GL_TEXTURE_2D);

	gl.BindTexture(GL_TEXTURE_2D, tex_obj);

	gl.Begin(GL_QUADS);
	gl.TexCoord2f((float) v1->u / texture->pitchw, (float) v1->v / texture->pitchh);
	gl.Vertex3s(v1->x, v1->y, v1->z);
	gl.TexCoord2f((float) v2->u / texture->pitchw, (float) v2->v / texture->pitchh);
	gl.Vertex3s(v2->x, v2->y, v2->z);
	gl.TexCoord2f((float) v3->u / texture->pitchw, (float) v3->v / texture->pitchh);
	gl.Vertex3s(v3->x, v3->y, v3->z);
	gl.TexCoord2f((float) v4->u / texture->pitchw, (float) v4->v / texture->pitchh);
	gl.Vertex3s(v4->x, v4->y, v4->z);
	gl.End();

	gl.Disable(GL_TEXTURE_2D);
	gl.Disable(GL_CULL_FACE);
}

#else

#include "video.h"
#include "render.h"

void render_opengl_init(render_t *render)
{
}

#endif /* ENABLE_OPENGL */
