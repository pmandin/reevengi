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

static void resize(video_surface_t *this, int w, int h);

/*--- Functions ---*/

video_surface_t *video_surface_create(int w, int h, int bpp)
{
	int sw = w, sh = h;

	video_surface_t *this = (video_surface_t *) calloc(1, sizeof(video_surface_t));
	if (!this) {
		return NULL;
	}

	/* Align on 16 pixels boundaries */
	if (sw & 15) {
		sw = (sw|15)+1;
	}
	if (sh & 15) {
		sh = (sh|15)+1;
	}

	this->sdl_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, sw,sh,bpp, 0,0,0,0);
	if (!this->sdl_surf) {
		free(this);
		return NULL;
	}

	/* This is the dimensions we work on */
	this->width = w;
	this->height = h;
	this->bpp = this->sdl_surf->format->BitsPerPixel;

	this->dirty_rects = dirty_rects_create(sw, sh);

	this->resize = resize;
	return this;
}

video_surface_t *video_surface_create_pf(int w, int h, SDL_PixelFormat *pixelFormat)
{
	int sw = w, sh = h;

	video_surface_t *this = (video_surface_t *) calloc(1, sizeof(video_surface_t));
	if (!this) {
		return NULL;
	}

	/* Align on 16 pixels boundaries */
	if (sw & 15) {
		sw = (sw|15)+1;
	}
	if (sh & 15) {
		sh = (sh|15)+1;
	}

	this->sdl_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, sw,sh,
		pixelFormat->BitsPerPixel,
		pixelFormat->Rmask, pixelFormat->Gmask,
		pixelFormat->Bmask, pixelFormat->Amask
	);
	if (!this->sdl_surf) {
		free(this);
		return NULL;
	}

	/* This is the dimensions we work on */
	this->width = w;
	this->height = h;
	this->bpp = this->sdl_surf->format->BitsPerPixel;

	this->dirty_rects = dirty_rects_create(sw, sh);

	this->resize = resize;
	return this;
}

video_surface_t *video_surface_create_su(SDL_Surface *surface)
{
	int sw = surface->w, sh = surface->h;

	video_surface_t *this = (video_surface_t *) calloc(1, sizeof(video_surface_t));
	if (!this) {
		return NULL;
	}

	/* Align on 16 pixels boundaries */
	if (sw & 15) {
		sw = (sw|15)+1;
	}
	if (sh & 15) {
		sh = (sh|15)+1;
	}

	this->sdl_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, sw,sh,
		surface->format->BitsPerPixel,
		surface->format->Rmask, surface->format->Gmask,
		surface->format->Bmask, surface->format->Amask
	);
	if (!this->sdl_surf) {
		free(this);
		return NULL;
	}

	/* Copy palette */
	if (surface->format->BitsPerPixel == 8) {
		int i;
		SDL_Color palette[256];

		for (i=0; i<surface->format->palette->ncolors; i++) {
			palette[i].r = surface->format->palette->colors[i].r;
			palette[i].g = surface->format->palette->colors[i].g;
			palette[i].b = surface->format->palette->colors[i].b;
		}
		SDL_SetPalette(this->sdl_surf, SDL_LOGPAL, palette, 0, 256);
	}

	/* Copy pixels */
	SDL_BlitSurface(surface, NULL, this->sdl_surf, NULL);

	/* This is the dimensions we work on */
	this->width = surface->w;
	this->height = surface->h;
	this->bpp = this->sdl_surf->format->BitsPerPixel;

	this->dirty_rects = dirty_rects_create(sw, sh);

	this->resize = resize;
	return this;
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

static void resize(video_surface_t *this, int w, int h)
{
	SDL_bool recreate_surface = SDL_TRUE;
	SDL_bool restore_palette = SDL_FALSE;
	SDL_PixelFormat pixelFormat;
	SDL_Color palette[256];
	SDL_Surface *surface = this->sdl_surf;
	int sw = w, sh = h;

	/* Do not recreate surface if big enough */
	if (surface) {
		if ((surface->w > w) && (surface->h > h)) {
			recreate_surface = SDL_FALSE;
		}
		this->width = w;
		this->height = h;
	}

	if (!recreate_surface) {
		return;
	}

	/* Needed surface dimensions */
	if (sw & 15) {
		sw = (sw|15)+1;
	}
	if (sh & 15) {
		sh = (sh|15)+1;
	}

	/* Create a bigger surface */
	memset(&pixelFormat, 0, sizeof(SDL_PixelFormat));

	if (surface) {
		/* Save pixel format */
		memcpy(&pixelFormat, surface->format, sizeof(SDL_PixelFormat));

		/* Save palette ? */
		if ((surface->format->BitsPerPixel==8) && surface->format->palette) {
			int i;

			for (i=0; i<surface->format->palette->ncolors; i++) {
				palette[i].r = surface->format->palette->colors[i].r;
				palette[i].g = surface->format->palette->colors[i].g;
				palette[i].b = surface->format->palette->colors[i].b;
			}
			restore_palette = SDL_TRUE;
		}

		SDL_FreeSurface(surface);
	} else {
		pixelFormat.BitsPerPixel = 8;
	}

	this->sdl_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
		sw, sh, pixelFormat.BitsPerPixel,
		pixelFormat.Rmask, pixelFormat.Gmask,
		pixelFormat.Bmask, pixelFormat.Amask
	);

	if (restore_palette) {
		SDL_SetPalette(this->sdl_surf, SDL_LOGPAL, palette, 0, 256);
	}

	this->dirty_rects->resize(this->dirty_rects, sw, sh);
}
