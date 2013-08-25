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
#include "log.h"

#include "r_common/render.h"
#include "r_common/render_text.h"
#include "r_soft/dither.h"

/*--- Function prototypes ---*/

static void shutDown(void);
static void findNearestMode(int *width, int *height, int bpp);
static void setVideoMode(int width, int height, int bpp);
static void swapBuffers(void);
static void countFps(void);
static void screenShot(void);
static void initViewport(void);

/*--- Functions ---*/

void video_soft_init(video_t *this)
{
	int i;

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

	for (i=0; i<2; i++) {
		this->dirty_rects[i] = dirty_rects_create(this->width, this->height);
		this->upload_rects[i] = dirty_rects_create(this->width, this->height);
	}
}

static void shutDown(void)
{
	int i;

	for (i=0; i<2; i++) {
		if (video.dirty_rects[i]) {
			dirty_rects_destroy(video.dirty_rects[i]);
			video.dirty_rects[i] = NULL;
		}
		if (video.upload_rects[i]) {
			dirty_rects_destroy(video.upload_rects[i]);
			video.upload_rects[i] = NULL;
		}
	}

	if (video.list_rects) {
		free(video.list_rects);
		video.list_rects = NULL;
		video.num_list_rects = 0;
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

static void findNearestMode(int *width, int *height, int bpp)
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

static void setVideoMode(int width, int height, int bpp)
{
	int i;

	/* Search nearest fullscreen mode */
	if (video.flags & SDL_FULLSCREEN) {
		findNearestMode(&width, &height, bpp);
		logMsg(1, "video: found nearest %dx%d\n", width, height);
	}

	video.screen = SDL_SetVideoMode(width, height, bpp, video.flags);
	if (!video.screen) {
		/* Try 8 bpp if failed in true color */
		video.screen = SDL_SetVideoMode(width, height, 8, video.flags);
	}
	if (!video.screen) {
		fprintf(stderr, "Can not set %dx%dx%d mode\n", width, height, bpp);
		return;
	}

	video.width = video.screen->w;
	video.height = video.screen->h;
	video.bpp = video.screen->format->BitsPerPixel;
	video.flags = video.screen->flags;

	/* Set 216 color palette */
	if (video.bpp==8) {
		dither_setpalette(video.screen);
	}

	for (i=0; i<2; i++) {
		video.dirty_rects[i]->resize(video.dirty_rects[i], video.width, video.height);
		video.upload_rects[i]->resize(video.upload_rects[i], video.width, video.height);
	}

	logMsg(1, "video: switched to %dx%d\n", video.width, video.height);

	render.resize(video.width, video.height, video.bpp);
	video.initViewport();
}

static void countFps(void)
{
	static int cur_fps = 0;
	Uint32 cur_tick = SDL_GetTicks();

	++video.fps;
	if (cur_tick-video.start_tick>1000) {
		cur_fps = video.fps;
		if (cur_fps>999) cur_fps=999;

		video.fps = 0;
		video.start_tick = cur_tick;
	}

	if (params.fps) {
		char fps_fmt[16];

		sprintf(fps_fmt, "fps: %d", cur_fps);
		render_text(fps_fmt, video.viewport.w-8*8, 0);
	}
}

static void swapBuffers(void)
{
	int i, x, y;

	video.countFps();

	render.endFrame();

	if ((video.flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		video.numfb ^= 1;
		SDL_Flip(video.screen);
		return;
	}

	/* Update background from rectangle list */
	i = video.upload_rects[video.numfb]->width * video.upload_rects[video.numfb]->height;
	if (i>video.num_list_rects) {
		video.list_rects = (SDL_Rect *) realloc(video.list_rects, i * sizeof(SDL_Rect));
		video.num_list_rects = i;
	}

	i = 0;
	for (y=0; y<video.upload_rects[video.numfb]->height; y++) {
		int block_w = 0;
		int block_x = 0;
		int num_rows = 16;

		if (((y+1)<<4) > video.height) {
			num_rows = video.height - (y<<4);
		}

		for (x=0; x<video.upload_rects[video.numfb]->width; x++) {
			/* Force update on last column */
			int block_update = (x==video.upload_rects[video.numfb]->width-1);
			int num_cols = 16;

			if (((x+1)<<4) > video.width) {
				num_cols = video.width - (x<<4);
			}

			if (video.upload_rects[video.numfb]->markers[y*video.upload_rects[video.numfb]->width + x]) {
				/* Dirty */
				if (block_w==0) {
					/* First dirty block, mark x pos */
					block_x = x;
				}
				block_w += num_cols;
			} else {
				/* Non dirty, force update of previously merged blocks */
				block_update = 1;
			}

			/* Update only if we have a dirty block */
			if (block_update && (block_w>0)) {
				/* Add rectangle */
				video.list_rects[i].x = block_x<<4;
				video.list_rects[i].y = y<<4;
				video.list_rects[i].w = block_w;
				video.list_rects[i].h = num_rows;

				i++;

				block_w = 0;
			}
		}
	}

	SDL_UpdateRects(video.screen, i, video.list_rects);
	video.upload_rects[video.numfb]->clear(video.upload_rects[video.numfb]);
}

static void screenShot(void)
{
	char filename[16];

	if (!video.screen) {
		return;
	}

	sprintf(filename, "%08d.bmp", video.num_screenshot++);

	printf("Screenshot %s: %s\n", filename,
		SDL_SaveBMP(video.screen, filename)==0 ? "done" : "failed");
}

static void initViewport(void)
{
	int cur_asp_w = 1, cur_asp_h = 1;	/* Square pixel as default */
	int img_w, img_h;
	int pos_x, pos_y, scr_w, scr_h;

	/* Only keep non 5:4 ratio in fullscreen */
	if ((video.flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) {
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
	scr_w = (video.height*img_w)/img_h;
	scr_h = (video.width*img_h)/img_w;
	if (scr_w>video.width) {
		scr_w = video.width;
	} else if (scr_h>video.height) {
		scr_h = video.height;
	}

	/* Center */
	pos_x = (video.width - scr_w)>>1;
	pos_y = (video.height - scr_h)>>1;

	if (pos_x>0) {
		video.viewport.x = pos_x;
		video.viewport.y = 0;
		video.viewport.w = scr_w;
		video.viewport.h = video.height;
	} else {
		video.viewport.x = 0;
		video.viewport.y = pos_y;
		video.viewport.w = video.width;
		video.viewport.h = scr_h;
	}

	logMsg(2, "video: viewport %d,%d %dx%d\n",
		video.viewport.x, video.viewport.y,
		video.viewport.w, video.viewport.h);

	render.set_viewport(video.viewport.x, video.viewport.y,
		video.viewport.w, video.viewport.h);
}
