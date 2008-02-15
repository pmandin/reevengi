/*
	Video backend
	Software

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

/*--- Includes ---*/

#include <SDL.h>

#include "parameters.h"
#include "video.h"

/*--- Local variables ---*/

static video_surface_t *background_surf = NULL;

/*--- Function prototypes ---*/

static void setVideoMode(video_t *this, int width, int height, int bpp);
static void swapBuffers(video_t *this);
static void screenShot(video_t *this);
static void initScreen(video_t *this);
static void refreshBackground(video_t *this);
static void drawBackground(video_t *this, video_surface_t *surf);

/*--- Functions ---*/

void video_soft_init(video_t *this)
{
	this->width = 320;
	this->height = 240;
	this->bpp = 16;
	this->flags = SDL_DOUBLEBUF|SDL_RESIZABLE;

	this->screen = NULL;
	this->num_screenshot = 0;

	this->setVideoMode = setVideoMode;
	this->swapBuffers = swapBuffers;
	this->screenShot = screenShot;

	this->initScreen = initScreen;
	this->refreshBackground = refreshBackground;
	this->drawBackground = drawBackground;

	this->createSurface = video_surface_create;
	this->createSurfacePf = video_surface_create_pf;
	this->createSurfaceSu = video_surface_create_su;
	this->destroySurface = video_surface_destroy;

	if (!aspect_user) {
		video_detect_aspect();
	}

	this->dirty_rects = dirty_rects_create(this->width, this->height);
}

void video_soft_shutdown(video_t *this)
{
	if (this->dirty_rects) {
		dirty_rects_destroy(this->dirty_rects);
		this->dirty_rects = NULL;
	}
}

/* Search biggest video mode, calculate its ratio */

void video_detect_aspect(void)
{
	SDL_Rect **modes;
	int i, max_w = 0, max_h = 0, ratio_w[4];

	modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
	if (modes == (SDL_Rect **) 0) {
		/* No fullscreen mode */
		return;
	} else if (modes == (SDL_Rect **) -1) {
		/* Windowed mode */
		return;
	}

	for (i=0; modes[i]; ++i) {
		if ((modes[i]->w>max_w) && (modes[i]->h>max_h)) {
			max_w = modes[i]->w;
			max_h = modes[i]->h;
		}
	}

	/*printf("Biggest video mode: %dx%d\n", max_w, max_h);*/

	/* Calculate nearest aspect ratio */
	ratio_w[0] = (max_h * 5) / 4;
	ratio_w[1] = (max_h * 4) / 3;
	ratio_w[2] = (max_h * 16) / 10;
	ratio_w[3] = (max_h * 16) / 9;

	if (max_w < (ratio_w[0]+ratio_w[1])>>1) {
		aspect_x = 5; aspect_y = 4;
	} else if (max_w < (ratio_w[1]+ratio_w[2])>>1) {
		aspect_x = 4; aspect_y = 3;
	} else if (max_w < (ratio_w[2]+ratio_w[3])>>1) {
		aspect_x = 16; aspect_y = 10;
	} else {
		aspect_x = 16; aspect_y = 9;
	}

	printf("Calculated aspect ratio %d:%d\n", aspect_x, aspect_y);
}

static void setVideoMode(video_t *this, int width, int height, int bpp)
{
	this->screen = SDL_SetVideoMode(width, height, bpp, this->flags);
	if (!this->screen) {
		fprintf(stderr, "Can not set %dx%dx%d mode\n", width, height, bpp);
		return;
	}

	this->width = this->screen->w;
	this->height = this->screen->h;
	this->bpp = this->screen->format->BitsPerPixel;
	this->flags = this->screen->flags;

	this->dirty_rects->resize(this->dirty_rects, this->width, this->height);
}

static void swapBuffers(video_t *this)
{
	SDL_Rect *list_rects;
	int i, x, y, maxx, maxy;

	if ((this->flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		SDL_Flip(this->screen);
		return;
	}

	/* Update background from rectangle list */
	list_rects = (SDL_Rect *) calloc(this->dirty_rects->width * this->dirty_rects->height,
		sizeof(SDL_Rect));
	if (!list_rects) {
		SDL_UpdateRect(this->screen, 0,0,0,0);
		return;
	}

	i = 0;
	for (y=0; y<this->dirty_rects->height; y++) {
		for (x=0; x<this->dirty_rects->width; x++) {
			int maxw = 1<<4, maxh = 1<<4;

			if (this->dirty_rects->markers[y*this->dirty_rects->width + x] == 0) {
				continue;
			}

			if (this->width - (x<<4) < (1<<4)) {
				maxw = this->width - (x<<4);
			}
			if (this->height - (y<<4) < (1<<4)) {
				maxh = this->height - (y<<4);
			}

			/* Add rectangle */
			list_rects[i].x = x<<4;
			list_rects[i].y = y<<4;
			list_rects[i].w = maxw;
			list_rects[i].h = maxh;
			i++;
		}
	}

	SDL_UpdateRects(this->screen, i, list_rects);
	this->dirty_rects->clear(this->dirty_rects);
	free(list_rects);
}

static void screenShot(video_t *this)
{
	char filename[16];

	if (!this->screen) {
		return;
	}

	sprintf(filename, "%08d.bmp", this->num_screenshot++);

	printf("Screenshot %s: %s\n", filename,
		SDL_SaveBMP(this->screen, filename)==0 ? "done" : "failed");
}

static void initScreen(video_t *this)
{
}

static void refreshBackground(video_t *this)
{
	background_surf = NULL;
}

static void drawBackground(video_t *this, video_surface_t *surf)
{
	SDL_Rect src_rect, dst_rect;

	if (!this->screen) {
		return;
	}

	if (background_surf == surf) {
		return;
	}
	background_surf = surf;

	src_rect.x = src_rect.y = 0;
	src_rect.w = surf->getSurface(surf)->w;
	src_rect.h = surf->getSurface(surf)->h;

	dst_rect.x = dst_rect.y = 0;
	dst_rect.w = this->screen->w;
	dst_rect.h = this->screen->h;

	if (dst_rect.w > src_rect.w) {
		dst_rect.x = (dst_rect.w - src_rect.w) >> 1;
		dst_rect.w = src_rect.w;
	} else {
		src_rect.w = this->screen->w;
	}

	if (dst_rect.h > src_rect.h) {
		dst_rect.y = (dst_rect.h - src_rect.h) >> 1;
		dst_rect.h = src_rect.h;
	} else {
		src_rect.h = this->screen->h;
	}

	SDL_BlitSurface(surf->getSurface(surf), &src_rect, this->screen, &dst_rect);
	this->dirty_rects->setDirty(this->dirty_rects, dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);
}
