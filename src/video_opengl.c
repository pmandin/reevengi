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
# include <SDL_opengl.h>
# include "dyngl.h"

# include "video_surface_opengl.h"
#endif

#include "video.h"

#include "state.h"

#ifdef ENABLE_OPENGL

/*--- Local variables ---*/

static video_surface_t *background_surf = NULL;

/*--- Function prototypes ---*/

static void setVideoMode(video_t *this, int width, int height, int bpp);
static void swapBuffers(video_t *this);
static void screenShot(video_t *this);
static void initScreen(video_t *this);
static void refreshBackground(video_t *this);
static void drawBackground(video_t *this, video_surface_t *surf);

static void drawGrid(void);

#endif /* ENABLE_OPENGL */

/*--- Functions ---*/

int video_opengl_loadlib(void)
{
#ifdef ENABLE_OPENGL
	if (dyngl_load(NULL)) {
		return 1;
	}

	fprintf(stderr, "Can not load OpenGL library: using software rendering mode\n");
#endif /* ENABLE_OPENGL */
	return 0;
}

void video_opengl_init(video_t *this)
{
#ifdef ENABLE_OPENGL
	this->width = 640;
	this->height = 480;
	this->bpp = 0;
	this->flags = SDL_OPENGL|SDL_RESIZABLE;

	this->screen = NULL;
	this->num_screenshot = 0;

	this->setVideoMode = setVideoMode;
	this->swapBuffers = swapBuffers;
	this->screenShot = screenShot;

	this->initScreen = initScreen;
	this->refreshBackground = refreshBackground;
	this->drawBackground = drawBackground;

	this->createSurface = video_surface_gl_create;
	this->createSurfacePf = video_surface_gl_create_pf;
	this->createSurfaceSu = video_surface_gl_create_su;
	this->destroySurface = video_surface_gl_destroy;
#endif /* ENABLE_OPENGL */
}

#ifdef ENABLE_OPENGL

static void setVideoMode(video_t *this, int width, int height, int bpp)
{
	const int gl_bpp[4]={0,16,24,32};
	int i;
	SDL_Surface *screen;

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

	printf("opengl: %dx%dx%d video mode\n", this->width, this->height, this->bpp);
}

static void swapBuffers(video_t *this)
{
	SDL_GL_SwapBuffers();
}

static void screenShot(video_t *this)
{
	fprintf(stderr, "Screenshot not available in OpenGL mode\n");
}

static void initScreen(video_t *this)
{
	gl.ClearColor(0.6,0.4,0.2,0.0);
	gl.Clear(GL_COLOR_BUFFER_BIT);

	gl.Viewport(0, 0, this->width, this->height);
}

static void refreshBackground(video_t *this)
{
	background_surf = NULL;
}

static void drawBackground(video_t *this, video_surface_t *surf)
{
	video_surface_gl_t *gl_surf = (video_surface_gl_t *) surf;
	GLenum textureTarget, textureObject;
	SDL_Surface *sdl_surf;

	if (!this->screen) {
		return;
	}

	/*if (background_surf == surf) {
		return;
	}*/
	background_surf = surf;

	textureTarget = gl_surf->textureTarget;
	textureObject = gl_surf->textureObject;
	sdl_surf = surf->getSurface(surf);
	
	gl.Enable(textureTarget);
	gl.BindTexture(textureTarget, textureObject);

 	gl.TexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
 	gl.TexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);

 	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	gl.Enable(GL_DITHER);

	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE, GL_ZERO);

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0, this->width, this->height, 0.0, -1.0, 1.0);

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.Scalef(background_surf->width,background_surf->height,1.0);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	//gl.Translatef(0.375, 0.375, 0.0);
	gl.Scalef(this->width, this->height, 1.0);

	gl.Begin(GL_QUADS);
		gl.TexCoord2f(0.0, 0.0);
		gl.Vertex2f(0.0, 0.0);

		gl.TexCoord2f(1.0, 0.0);
		gl.Vertex2f(1.0, 0.0);

		gl.TexCoord2f(1.0, 1.0);
		gl.Vertex2f(1.0, 1.0);

		gl.TexCoord2f(0.0, 1.0);
		gl.Vertex2f(0.0, 1.0);
	gl.End();

	gl.Disable(GL_DITHER);
	gl.Disable(GL_BLEND);
	gl.Disable(GL_TEXTURE_RECTANGLE_ARB);

	drawGrid();
}

static void drawGrid(void)
{
	long cam_pos[6];
	int i;

	if (!game_state.room_file) {
		return;
	}

	switch(game_state.version) {
		case GAME_RE1_PS1_DEMO:
		case GAME_RE1_PS1_GAME:
			re1ps1_get_camera(cam_pos);
			break;
		case GAME_RE1_PC_GAME:
			re1pcgame_get_camera(cam_pos);
			break;
		case GAME_RE2_PS1_DEMO:
		case GAME_RE2_PS1_GAME_LEON:
		case GAME_RE2_PS1_GAME_CLAIRE:
			re2ps1_get_camera(cam_pos);
			break;
		case GAME_RE2_PC_DEMO:
			re2pcdemo_get_camera(cam_pos);
			break;
		case GAME_RE3_PS1_GAME:
			re3ps1game_get_camera(cam_pos);
			break;
		case GAME_RE3_PC_DEMO:
		case GAME_RE3_PC_GAME:
			re3pc_get_camera(cam_pos);
			break;
		default:
			return;
	}

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gluPerspective(60.0, 4.0/3.0, 0.1, 1000.0);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();

	gluLookAt(
		cam_pos[0]/256.0, cam_pos[1]/256.0, cam_pos[2]/256.0,
		cam_pos[3]/256.0, cam_pos[4]/256.0, cam_pos[5]/256.0,
		0.0, -1.0, 0.0
	);

	gl.Begin(GL_LINES);
		/* Origin */
		gl.Color3f(1.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(10.0,0.0,0.0);

		gl.Color3f(0.0,1.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,10.0,0.0);

		gl.Color3f(0.0,0.0,1.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,10.0);
	gl.End();

	gl.Translatef(cam_pos[3]/256.0, cam_pos[4]/256.0, cam_pos[5]/256.0);

	gl.Begin(GL_LINES);
		/* Camera target */
		gl.Color3f(1.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(10.0,0.0,0.0);

		gl.Color3f(0.0,1.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,10.0,0.0);

		gl.Color3f(0.0,0.0,1.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,10.0);

		/* Ground */
		gl.Color3f(1.0,1.0,1.0);
		for (i=-50; i<=50; i+=10) {
			gl.Vertex3f(-50.0,20.0,i);
			gl.Vertex3f(50.0,20.0,i);
			gl.Vertex3f(i,20.0,-50);
			gl.Vertex3f(i,20.0,50);
		}
	gl.End();
}

#endif /* ENABLE_OPENGL */
