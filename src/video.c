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
#include "render.h"

/*--- Function prototypes ---*/

static void shutDown(video_t *this);
static void setVideoMode(video_t *this, int width, int height, int bpp);
static void swapBuffers(video_t *this);
static void screenShot(video_t *this);
static void initViewport(video_t *this);
static void drawScreen(video_t *this);
static void refreshScreen(video_t *this);

/*--- Functions ---*/

void video_soft_init(video_t *this)
{
	this->width = 320;
	this->height = 240;
	this->bpp = 16;
	this->flags = SDL_DOUBLEBUF|SDL_RESIZABLE;

	this->screen = NULL;
	this->num_screenshot = 0;

	this->shutDown = shutDown;
	this->setVideoMode = setVideoMode;
	this->swapBuffers = swapBuffers;
	this->screenShot = screenShot;

	this->initViewport = initViewport;

	this->createSurface = video_surface_create;
	this->createSurfacePf = video_surface_create_pf;
	this->createSurfaceSu = video_surface_create_su;
	this->destroySurface = video_surface_destroy;

	if (!params.aspect_user) {
		video_detect_aspect();
	}

	this->dirty_rects = dirty_rects_create(this->width, this->height);
}

static void shutDown(video_t *this)
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
	SDL_PixelFormat pixelFormat;
	int i, j, max_w = 0, max_h = 0, ratio_w[4];
	const int bpps[5]={32,24,16,15,8};

	memset(&pixelFormat, 0, sizeof(SDL_PixelFormat));
	for (j=0; j<5; j++) {
		pixelFormat.BitsPerPixel = bpps[j];

		modes = SDL_ListModes(&pixelFormat, SDL_FULLSCREEN);
		if (modes == (SDL_Rect **) 0) {
			/* No fullscreen mode */
			continue;
		} else if (modes == (SDL_Rect **) -1) {
			/* Windowed mode */
			continue;
		}

		logMsg(3, "video: checking modes for %d bpp\n", bpps[j]);
		for (i=0; modes[i]; ++i) {
			logMsg(3, "video: checking mode %dx%d\n", modes[i]->w, modes[i]->h);
			if ((modes[i]->w>=max_w) && (modes[i]->h>=max_h)) {
				max_w = modes[i]->w;
				max_h = modes[i]->h;
			}
		}
	}

	logMsg(2,"Biggest video mode: %dx%d\n", max_w, max_h);
	if ((max_w == 0) && (max_h == 0)) {
		return;
	}

	/* Calculate nearest aspect ratio */
	ratio_w[0] = (max_h * 5) / 4;
	ratio_w[1] = (max_h * 4) / 3;
	ratio_w[2] = (max_h * 16) / 10;
	ratio_w[3] = (max_h * 16) / 9;

	if (max_w < (ratio_w[0]+ratio_w[1])>>1) {
		params.aspect_x = 5; params.aspect_y = 4;
	} else if (max_w < (ratio_w[1]+ratio_w[2])>>1) {
		params.aspect_x = 4; params.aspect_y = 3;
	} else if (max_w < (ratio_w[2]+ratio_w[3])>>1) {
		params.aspect_x = 16; params.aspect_y = 10;
	} else {
		params.aspect_x = 16; params.aspect_y = 9;
	}
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
	logMsg(1, "video: switched to %dx%d\n", video.width, video.height);

	video.initViewport(&video);
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

static void initViewport(video_t *this)
{
	int cur_asp_w = 1, cur_asp_h = 1;	/* Square pixel as default */
	int img_w, img_h;
	int pos_x, pos_y, scr_w, scr_h;

	/* Only keep non 5:4 ratio in fullscreen */
	if ((this->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) {
		if ((params.aspect_x != 5) || (params.aspect_y != 4)) {
			cur_asp_w = params.aspect_x * 3;
			cur_asp_h = params.aspect_y * 4;
		}
	}

	/* Adapt source images in 4:3 ratio to the screen ratio */
	img_w = (320*cur_asp_h)/cur_asp_w;
	img_h = (240*cur_asp_w)/cur_asp_h;
	if (img_w>320) {
		img_w=320;
	} else if (img_h>240) {
		img_h=240;
	}

	/* Resize to fill screen */
	scr_w = (this->height*img_w)/img_h;
	scr_h = (this->width*img_h)/img_w;
	if (scr_w>this->width) {
		scr_w = this->width;
	} else if (scr_h>this->height) {
		scr_h = this->height;
	}

	/* Center */
	pos_x = (this->width - scr_w)>>1;
	pos_y = (this->height - scr_h)>>1;

	if (pos_x>0) {
		this->viewport.x = pos_x;
		this->viewport.y = 0;
		this->viewport.w = scr_w;
		this->viewport.h = this->height;
	} else {
		this->viewport.x = 0;
		this->viewport.y = pos_y;
		this->viewport.w = this->width;
		this->viewport.h = scr_h;
	}

	render.set_viewport(this->viewport.x, this->viewport.y,
		this->viewport.w, this->viewport.h);
}
