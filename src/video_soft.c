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

#include "video.h"

/*--- Function prototypes ---*/

static void setVideoMode(video_t *this, int width, int height, int bpp);
static void swapBuffers(video_t *this);
static void screenShot(video_t *this);
static void initScreen(video_t *this);
static void drawBackground(video_t *this, SDL_Surface *surf);

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
	this->drawBackground = drawBackground;
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
}

static void swapBuffers(video_t *this)
{
	if ((this->flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		SDL_Flip(this->screen);
	} else {
		SDL_UpdateRect(this->screen, 0,0,0,0);
	}
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

static void drawBackground(video_t *this, SDL_Surface *surf)
{
	SDL_Rect src_rect, dst_rect;

	if (!this->screen || !surf) {
		return;
	}

	src_rect.x = src_rect.y = 0;
	src_rect.w = surf->w;
	src_rect.h = surf->h;

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

	SDL_BlitSurface(surf, &src_rect, this->screen, &dst_rect);
}
