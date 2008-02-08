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

#include <stdlib.h>

#include "video_surface.h"
#include "video_surface_opengl.h"

/*--- Functions prototypes ---*/

static void resize(video_surface_t *this, int w, int h);

/*--- Functions ---*/

video_surface_t *video_surface_gl_create(int w, int h, int bpp)
{
	video_surface_t *parent = video_surface_create(w,h,bpp);
	if (!parent) {
		return NULL;
	}

	video_surface_gl_t *this = (video_surface_gl_t *) calloc(1, sizeof(video_surface_gl_t));
	if (!this) {
		video_surface_destroy(parent);
		return NULL;
	}

	memcpy(&this->surf_soft, parent, sizeof(video_surface_t));

	return (video_surface_t *) this;
}

video_surface_t *video_surface_gl_create_pf(int w, int h, SDL_PixelFormat *pixelFormat)
{
	video_surface_t *parent = video_surface_create_pf(w,h,pixelFormat);
	if (!parent) {
		return NULL;
	}

	video_surface_gl_t *this = (video_surface_gl_t *) calloc(1, sizeof(video_surface_gl_t));
	if (!this) {
		video_surface_destroy(parent);
		return NULL;
	}

	memcpy(&this->surf_soft, parent, sizeof(video_surface_t));

	return (video_surface_t *) this;
}

video_surface_t *video_surface_gl_create_su(SDL_Surface *surface)
{
	video_surface_t *parent = video_surface_create_su(surface);
	if (!parent) {
		return NULL;
	}

	video_surface_gl_t *this = (video_surface_gl_t *) calloc(1, sizeof(video_surface_gl_t));
	if (!this) {
		video_surface_destroy(parent);
		return NULL;
	}

	memcpy(&this->surf_soft, parent, sizeof(video_surface_t));

	return (video_surface_t *) this;
}

void video_surface_gl_destroy(video_surface_t *this)
{
	if (this) {
		video_surface_destroy(& ((video_surface_gl_t *) this)->surf_soft);

		free(this);
	}
}

/*--- Private functions ---*/

static void resize(video_surface_t *this, int w, int h)
{
}
