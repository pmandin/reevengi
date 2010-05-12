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
#include <stdio.h>

#include "parameters.h"
#include "video.h"
#include "render.h"
#include "dither.h"
#include "log.h"
#include "draw.h"
#include "render_text.h"

/*--- Function prototypes ---*/

static void shutDown(video_t *this);
static void findNearestMode(video_t *this, int *width, int *height, int bpp);
static void setVideoMode(video_t *this, int width, int height, int bpp);
static void swapBuffers(video_t *this);
static void countFps(video_t *this);
static void screenShot(video_t *this);
static void initViewport(video_t *this);

/*--- Functions ---*/

void video_soft_init(video_t *this)
{
	memset(this, 0, sizeof(video_t));

	this->width = (params.width ? params.width : 320);
	this->height = (params.height ? params.height : 240);
	this->bpp = (params.bpp ? params.bpp : 16);

	this->flags = SDL_DOUBLEBUF|SDL_RESIZABLE;
	this->start_tick = SDL_GetTicks();

	this->shutDown = shutDown;
	this->findNearestMode = findNearestMode;
	this->setVideoMode = setVideoMode;
	this->swapBuffers = swapBuffers;
	this->screenShot = screenShot;
	this->countFps = countFps;

	this->initViewport = initViewport;

	if (!params.aspect_user) {
		video_detect_aspect();
	}

	this->dirty_rects[0] = dirty_rects_create(this->width, this->height);
	this->upload_rects[0] = dirty_rects_create(this->width, this->height);

	this->dirty_rects[1] = dirty_rects_create(this->width, this->height);
	this->upload_rects[1] = dirty_rects_create(this->width, this->height);
}

static void shutDown(video_t *this)
{
	if (this->dirty_rects[0]) {
		dirty_rects_destroy(this->dirty_rects[0]);
		this->dirty_rects[0] = NULL;
	}
	if (this->upload_rects[0]) {
		dirty_rects_destroy(this->upload_rects[0]);
		this->upload_rects[0] = NULL;
	}

	if (this->dirty_rects[1]) {
		dirty_rects_destroy(this->dirty_rects[1]);
		this->dirty_rects[1] = NULL;
	}
	if (this->upload_rects[1]) {
		dirty_rects_destroy(this->upload_rects[1]);
		this->upload_rects[1] = NULL;
	}

	if (this->list_rects) {
		free(this->list_rects);
		this->list_rects = NULL;
		this->num_list_rects = 0;
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

static void findNearestMode(video_t *this, int *width, int *height, int bpp)
{
	SDL_Rect **modes;
	SDL_PixelFormat pixelFormat;
	int i, j=-1, pixcount, minpixcount, pixcount2, w=*width, h=*height;

	memset(&pixelFormat, 0, sizeof(SDL_PixelFormat));
	pixelFormat.BitsPerPixel = bpp;

	modes = SDL_ListModes(NULL /*&pixelFormat*/, SDL_FULLSCREEN);
	if (modes == (SDL_Rect **) 0) {
		/* No fullscreen mode */
		logMsg(2, "video: no fullscreen mode\n");
		return;
	} else if (modes == (SDL_Rect **) -1) {
		/* Windowed mode */
		logMsg(2, "video: windowed mode\n");
		return;
	}

	pixcount = w*h;
	minpixcount = 2000000000;
	for (i=0; modes[i]; ++i) {
		logMsg(2, "video: check nearest %dx%d\n", modes[i]->w, modes[i]->h);

		/* Mode in list */
		if ((modes[i]->w == w) && (modes[i]->h == h)) {
			return;
		}

		/* Stop at first mode bigger than needed */
		pixcount2 = modes[i]->w * modes[i]->h;
		if (pixcount2 >= pixcount) {
			if (pixcount2<=minpixcount) {
				minpixcount = pixcount2;
				j = i;
			}
		}
	}

	if (j>=0) {
		*width = modes[j]->w;
		*height = modes[j]->h;
	}
}

static void setVideoMode(video_t *this, int width, int height, int bpp)
{
	/* Search nearest fullscreen mode */
	if (this->flags & SDL_FULLSCREEN) {
		findNearestMode(this, &width, &height, bpp);
		logMsg(1, "video: found nearest %dx%d\n", width, height);
	}

	this->screen = SDL_SetVideoMode(width, height, bpp, this->flags);
	if (!this->screen) {
		/* Try 8 bpp if failed in true color */
		this->screen = SDL_SetVideoMode(width, height, 8, this->flags);
	}
	if (!this->screen) {
		fprintf(stderr, "Can not set %dx%dx%d mode\n", width, height, bpp);
		return;
	}

	this->width = this->screen->w;
	this->height = this->screen->h;
	this->bpp = this->screen->format->BitsPerPixel;
	this->flags = this->screen->flags;

	/* Set 216 color palette */
	if (this->bpp==8) {
		dither_setpalette(this->screen);
	}

	this->dirty_rects[0]->resize(this->dirty_rects[0], this->width, this->height);
	this->upload_rects[0]->resize(this->upload_rects[0], this->width, this->height);
	this->dirty_rects[1]->resize(this->dirty_rects[1], this->width, this->height);
	this->upload_rects[1]->resize(this->upload_rects[1], this->width, this->height);

	logMsg(1, "video: switched to %dx%d\n", video.width, video.height);

	render.resize(&render, this->width, this->height, this->bpp);
	video.initViewport(&video);
}

static void countFps(video_t *this)
{
	static int cur_fps = 0;
	Uint32 cur_tick = SDL_GetTicks();

	++this->fps;
	if (cur_tick-this->start_tick>1000) {
		cur_fps = this->fps;
		if (cur_fps>999) cur_fps=999;

		this->fps = 0;
		this->start_tick = cur_tick;
	}

	if (params.fps) {
		char fps_fmt[16];

		sprintf(fps_fmt, "fps: %d", cur_fps);
		render_text(fps_fmt, video.viewport.w-8*8, 0);
	}
}

static void swapBuffers(video_t *this)
{
	int i, x, y;

	this->countFps(this);

	render.endFrame(&render);

	if ((this->flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		this->numfb ^= 1;
		SDL_Flip(this->screen);
		return;
	}

	/* Update background from rectangle list */
	i = this->upload_rects[this->numfb]->width * this->upload_rects[this->numfb]->height;
	if (i>this->num_list_rects) {
		this->list_rects = (SDL_Rect *) realloc(this->list_rects, i * sizeof(SDL_Rect));
		this->num_list_rects = i;
	}

	i = 0;
	for (y=0; y<this->upload_rects[this->numfb]->height; y++) {
		int num_rows = 16;

		if (((y+1)<<4) > this->height) {
			num_rows = this->height - (y<<4);
		}

		for (x=0; x<this->upload_rects[this->numfb]->width; x++) {
			int num_cols = 16;

			if (this->upload_rects[this->numfb]->markers[y*this->upload_rects[this->numfb]->width + x] == 0) {
				continue;
			}

			if (((x+1)<<4) > this->width) {
				num_cols = this->width - (x<<4);
			}

			/* Add rectangle */
			this->list_rects[i].x = x<<4;
			this->list_rects[i].y = y<<4;
			this->list_rects[i].w = num_cols;
			this->list_rects[i].h = num_rows;
			i++;
		}
	}

	SDL_UpdateRects(this->screen, i, this->list_rects);
	this->upload_rects[this->numfb]->clear(this->upload_rects[this->numfb]);
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

	logMsg(2, "video: viewport %d,%d %dx%d\n",
		this->viewport.x, this->viewport.y,
		this->viewport.w, this->viewport.h);

	render.set_viewport(this->viewport.x, this->viewport.y,
		this->viewport.w, this->viewport.h);
}
