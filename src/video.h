/*
	Video backend

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

#ifndef VIDEO_H
#define VIDEO_H 1

/*--- Types ---*/

typedef struct video_s video_t;

struct video_s {
	int width, height, bpp, flags, numfb;
	int fps, start_tick;
	SDL_Surface *screen;
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_Renderer *renderer;
#endif
	int num_screenshot;

	SDL_Rect viewport;

	void (*shutDown)(void);
	void (*findNearestMode)(int *width, int *height, int bpp);
	void (*setVideoMode)(int width, int height, int bpp);
	void (*swapBuffers)(void);
	void (*screenShot)(void);
	void (*countFps)(void);

	void (*initViewport)(void);
	void (*setPalette)(SDL_Surface *surf);

	/* Rect list of final update */
	int num_list_rects;
	SDL_Rect *list_rects;

	/* OpenGL extensions */
	int has_gl_arb_texture_non_power_of_two;
	int has_gl_arb_texture_rectangle;
	int has_gl_ext_paletted_texture;
	int has_gl_ext_texture_rectangle;
	int has_gl_nv_texture_rectangle;
};

void video_detect_aspect(void);

void video_soft_init(video_t *this);
void video_soft_shutdown(video_t *this);

int video_opengl_loadlib(void);
void video_opengl_init(video_t *this);
void video_opengl_shutdown(video_t *this);

/*--- Variables ---*/

extern video_t video;

#endif /* VIDEO_H */
