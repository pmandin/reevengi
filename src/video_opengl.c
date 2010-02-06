/*
	Video backend
	OpenGL

	Copyright (C) 2007	Patrice Mandin

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

#include <SDL.h>

#ifdef ENABLE_OPENGL

#include <SDL_opengl.h>

#include "dyngl.h"

#include "parameters.h"
#include "video.h"
#include "render.h"
#include "state.h"
#include "log.h"

/*--- Variables ---*/

static int first_time = 1;	/* To display OpenGL driver properties */

/*--- Function prototypes ---*/

static void setVideoMode(video_t *this, int width, int height, int bpp);
static void swapBuffers(video_t *this);
static void screenShot(video_t *this);

static void
ShowDepthBuffer( GLsizei winWidth, GLsizei winHeight,
                 GLfloat zBlack, GLfloat zWhite );

extern int render_masks;

/*--- Functions ---*/

int video_opengl_loadlib(void)
{
	if (dyngl_load(NULL)) {
		return 1;
	}

	fprintf(stderr, "Can not load OpenGL library: using software rendering mode\n");
	return 0;
}

void video_opengl_init(video_t *this)
{
	video_soft_init(this);

	this->width = (params.width ? params.width : 640);
	this->height = (params.height ? params.height : 480);
	this->bpp = 0;
	this->flags = SDL_OPENGL|SDL_RESIZABLE;

	this->setVideoMode = setVideoMode;
	this->swapBuffers = swapBuffers;
	this->screenShot = screenShot;

	/*if (!aspect_user) {
		video_detect_aspect();
	}*/

	this->dirty_rects[this->numfb]->resize(this->dirty_rects[this->numfb], this->width, this->height);
}

static void setVideoMode(video_t *this, int width, int height, int bpp)
{
	const int gl_bpp[4]={0,16,24,32};
	int i;
	SDL_Surface *screen;
	const char *extensions;

	if (this->flags & SDL_FULLSCREEN) {
		this->findNearestMode(this, &width, &height, bpp);
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,15);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	/* Try with default bpp */
	for (i=0;i<4;i++) {
		screen = SDL_SetVideoMode(width, height, gl_bpp[i], this->flags);
		if (screen) {
			break;
		}
	}
	if (screen==NULL) {
		/* Try with default resolution */
		for (i=0;i<4;i++) {
			screen = SDL_SetVideoMode(0, 0, gl_bpp[i], this->flags);
			if (screen) {
				break;
			}
		}
	}
	this->screen = screen;
	if (screen==NULL) {
		fprintf(stderr, "Can not set %dx%dx%d mode\n", width, height, bpp);
		return;
	}

	this->width = this->screen->w;
	this->height = this->screen->h;
	this->bpp = this->screen->format->BitsPerPixel;
	this->flags = this->screen->flags;

	this->dirty_rects[this->numfb]->resize(this->dirty_rects[this->numfb], this->width, this->height);
	logMsg(1, "video_ogl: switched to %dx%d\n", video.width, video.height);

	dyngl_initfuncs();

	/* Read infos about OpenGL driver */
	if (first_time) {
		extensions = (const char *) gl.GetString(GL_VENDOR);
		logMsg(2, "GL_VENDOR: %s\n", extensions);

		extensions = (const char *) gl.GetString(GL_RENDERER);
		logMsg(2, "GL_RENDERER: %s\n", extensions);

		extensions = (const char *) gl.GetString(GL_VERSION);
		logMsg(2, "GL_VERSION: %s\n", extensions);

		/* Check OpenGL extensions */
		extensions = (char *) gl.GetString(GL_EXTENSIONS);

		this->has_gl_arb_texture_non_power_of_two = 0 /*(strstr(extensions, "GL_ARB_texture_non_power_of_two") != NULL)*/;
		logMsg(2, "GL_ARB_texture_non_power_of_two: %d\n", this->has_gl_arb_texture_non_power_of_two);

		this->has_gl_arb_texture_rectangle = (strstr(extensions, "GL_ARB_texture_rectangle") != NULL);
		logMsg(2, "GL_ARB_texture_rectangle: %d\n", this->has_gl_arb_texture_rectangle);

		this->has_gl_ext_paletted_texture = 0 /*(strstr(extensions, "GL_EXT_paletted_texture") != NULL)*/;
		logMsg(2, "GL_EXT_paletted_texture: %d\n", this->has_gl_ext_paletted_texture);

		this->has_gl_ext_texture_rectangle = (strstr(extensions, "GL_EXT_texture_rectangle") != NULL);
		logMsg(2, "GL_EXT_texture_rectangle: %d\n", this->has_gl_ext_texture_rectangle);

		this->has_gl_nv_texture_rectangle = (strstr(extensions, "GL_NV_texture_rectangle") != NULL);
		logMsg(2, "GL_NV_texture_rectangle: %d\n", this->has_gl_nv_texture_rectangle);

		first_time = 0;
	}

	video.initViewport(&video);
}

static void swapBuffers(video_t *this)
{
	render.endFrame(&render);

#if 0
	{
		GLint viewport[4];
		GLdouble modelview[16];
		GLdouble projection[16];
		GLfloat curx,cury,curz;
		GLdouble posx,posy,posz;
		int mx,my;
		static int pmx=-1,pmy=-1;

		SDL_GetMouseState(&mx, &my);
		my = video.viewport.h - my;

		gl.MatrixMode(GL_PROJECTION);
		gl.LoadIdentity();
		gluPerspective(60.0f, 4.0f/3.0f, RENDER_Z_NEAR, RENDER_Z_FAR);

		gl.MatrixMode(GL_MODELVIEW);
		gl.LoadIdentity();

		gl.GetIntegerv(GL_VIEWPORT, viewport);
		gl.GetDoublev(GL_MODELVIEW_MATRIX, modelview);
		gl.GetDoublev(GL_PROJECTION_MATRIX, projection);

		curx = (float) mx;
		cury = (float) my;
		gl.ReadPixels(mx,my, 1,1, GL_DEPTH_COMPONENT, GL_FLOAT, &curz);

		if (gluUnProject(
			curx,cury,curz,
			modelview, projection, viewport,
			&posx,&posy,&posz) == GL_TRUE)
		{
			if ((pmx!=mx) || (pmy!=my)) {
				logMsg(1, "m:%d,%d,%f p:%f,%f,%f\n",
					mx,my,curz,
					posx,posy,posz);
				pmx = mx;
				pmy = my;
			}
		}
	}
#endif

	GLenum errCode = gl.GetError();
	if (errCode != GL_NO_ERROR) {
		logMsg(1, "OpenGL error %d\n", errCode);
	}

	this->countFps(this);

	SDL_GL_SwapBuffers();
}

static void screenShot(video_t *this)
{
	fprintf(stderr, "Screenshot not available in OpenGL mode\n");
}

#else /* ENABLE_OPENGL */

#include "video.h"

int video_opengl_loadlib(void)
{
	return 0;
}

void video_opengl_init(video_t *this)
{
}

#endif /* ENABLE_OPENGL */
