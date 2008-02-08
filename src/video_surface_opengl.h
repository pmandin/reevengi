/*
	Video surface
	OpenGL backend

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

#ifndef VIDEO_SURFACE_OPENGL_H
#define VIDEO_SURFACE_OPENGL_H 1

#include <SDL.h>
#include <SDL_opengl.h>

#include "video_surface.h"

typedef struct video_surface_gl_s video_surface_gl_t;

struct video_surface_gl_s {
	video_surface_t surf_soft;

	GLenum textureTarget;
	GLenum textureFormat;
	GLuint textureObject;

	int can_palette;
	int use_palette;
	int first_upload;
};

video_surface_t *video_surface_gl_create(int w, int h, int bpp);
video_surface_t *video_surface_gl_create_pf(int w, int h, SDL_PixelFormat *pixelFormat);
video_surface_t *video_surface_gl_create_su(SDL_Surface *surface);
void video_surface_gl_destroy(video_surface_t *this);

#endif /* VIDEO_SURFACE_OPENGL_H */
