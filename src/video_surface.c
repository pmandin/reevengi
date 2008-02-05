/*
	Video surface

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

/*--- Functions prototypes ---*/

/*--- Functions ---*/

video_surface_t *video_surface_create(int w, int h, int bpp)
{
	video_surface_t *this = (video_surface_t *) calloc(1, sizeof(video_surface_t));
	if (!this) {
		return NULL;
	}

	this->sdl_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w,h,bpp, 0,0,0,0);
	if (!this->sdl_surf) {
		free(this);
		return NULL;
	}

	this->width = this->sdl_surf->w;
	this->height = this->sdl_surf->h;
	this->bpp = this->sdl_surf->format->BitsPerPixel;

	this->dirty_rects = dirty_rects_create(this->width, this->height);
}

void video_surface_destroy(video_surface_t *this)
{
	if (this) {
		if (this->sdl_surf) {
			SDL_FreeSurface(this->sdl_surf);
			this->sdl_surf = NULL;
		}
		this->width = 0;
		this->height = 0;
		this->bpp = 0;

		dirty_rects_destroy(this->dirty_rects);
	}
}

/*--- Private functions ---*/
