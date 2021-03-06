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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "parameters.h"
#include "video.h"
#include "log.h"

#include "r_common/render.h"
#include "r_common/render_text.h"
#include "r_common/r_misc.h"
#include "r_soft/dirty_rects.h"
#include "r_soft/dither.h"

/*--- Defines ---*/

/*#undef DIRTY_RECTS*/

#if SDL_VERSION_ATLEAST(2,0,0)
#define REEVENGI_SDLFULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP
#else
#define REEVENGI_SDLFULLSCREEN_FLAG SDL_FULLSCREEN
#endif

/*--- Function prototypes ---*/

static void shutDown(void);

static void shutDown_sdl2(void);
static void searchBiggestMode(int *w, int *h);

static void findNearestMode(int *width, int *height, int bpp);
static void setVideoMode(int width, int height, int bpp);
static void swapBuffers(void);
static int updateDirtyRects(void);
static void countFps(void);
static void screenShot(void);
static void initViewport(void);
static void setPalette(SDL_Surface *surf);

/*--- Functions ---*/

void video_soft_init(video_t *this)
{
	int i;

	memset(this, 0, sizeof(video_t));

#if SDL_VERSION_ATLEAST(2,0,0)
	this->width = (params.width ? params.width : 640);
	this->height = (params.height ? params.height : 480);
	this->bpp = (params.bpp ? params.bpp : 32);
	this->flags = SDL_WINDOW_RESIZABLE;
#else
	this->width = (params.width ? params.width : 320);
	this->height = (params.height ? params.height : 240);
	this->bpp = (params.bpp ? params.bpp : 16);
	this->flags = SDL_DOUBLEBUF|SDL_RESIZABLE;
#endif
	this->start_tick = SDL_GetTicks();

	this->shutDown = shutDown;
	this->findNearestMode = findNearestMode;
	this->setVideoMode = setVideoMode;
	this->swapBuffers = swapBuffers;
	this->screenShot = screenShot;
	this->countFps = countFps;

	this->initViewport = initViewport;
	this->setPalette = setPalette;

	if (!params.aspect_user) {
		video_detect_aspect();
	}

	for (i=0; i<2; i++) {
		dirty_rects[i] = dirty_rects_create(this->width, this->height);
		upload_rects[i] = dirty_rects_create(this->width, this->height);
	}

	dither_init();
}

static void shutDown(void)
{
	int i;

	for (i=0; i<2; i++) {
		if (dirty_rects[i]) {
			dirty_rects_destroy(dirty_rects[i]);
			dirty_rects[i] = NULL;
		}
		if (upload_rects[i]) {
			dirty_rects_destroy(upload_rects[i]);
			upload_rects[i] = NULL;
		}
	}

	if (video.list_rects) {
		free(video.list_rects);
		video.list_rects = NULL;
		video.num_list_rects = 0;
	}

	shutDown_sdl2();
}

#if SDL_VERSION_ATLEAST(2,0,0)

static void shutDown_sdl2(void)
{
	if (video.screen) {
		SDL_FreeSurface(video.screen);
		video.screen=NULL;
	}
	if (video.texture) {
		SDL_DestroyTexture(video.texture);
		video.texture = NULL;
	}
	if (video.renderer) {
		SDL_DestroyRenderer(video.renderer);
		video.renderer=NULL;
	}
	if (video.window) {
		SDL_DestroyWindow(video.window);
		video.window=NULL;
	}
}

static void searchBiggestMode(int *w, int *h)
{
	int i;

	for(i=0; i<SDL_GetNumDisplayModes(0); i++) {
		SDL_DisplayMode displayMode;

		if (SDL_GetDisplayMode(0, i, &displayMode) != 0) {
			continue;
		}

		logMsg(3, "video: checking mode %dx%d\n", displayMode.w, displayMode.h);
		if ((displayMode.w>=*w) && (displayMode.h>=*h)) {
			*w = displayMode.w;
			*h = displayMode.h;
		}
	}
}

static void findNearestMode(int *width, int *height, int bpp)
{
	SDL_DisplayMode dwmode;

	SDL_GetDesktopDisplayMode(0, &dwmode);
	*width = dwmode.w;
	*height = dwmode.h;
}

static void swapBuffers(void)
{
	int i;

	SDL_UpdateTexture(video.texture, NULL, video.screen->pixels, video.screen->pitch);
	SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);
	SDL_RenderPresent(video.renderer);

	SDL_RenderClear(video.renderer);

#ifdef DIRTY_RECTS
	i = updateDirtyRects();

	SDL_UpdateWindowSurfaceRects(video.window, video.list_rects, i);
	upload_rects[video.numfb]->clear(upload_rects[video.numfb]);
#else
	SDL_UpdateRect(video.screen, 0,0,0,0);
#endif
}

#else	/* SDL 1 */

static void shutDown_sdl2(void)
{
}

static void searchBiggestMode(int *w, int *h)
{
	const int bpps[5]={32,24,16,15,8};

	int i, j;
	SDL_Rect **modes;
	SDL_PixelFormat pixelFormat;

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
			if ((modes[i]->w>=*w) && (modes[i]->h>=*h)) {
				*w = modes[i]->w;
				*h = modes[i]->h;
			}
		}
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

static void swapBuffers(void)
{
	int i;

	if ((video.flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		video.numfb ^= 1;
		SDL_Flip(video.screen);
		return;
	}

#ifdef DIRTY_RECTS
	i = updateDirtyRects();

	SDL_UpdateRects(video.screen, i, video.list_rects);
	upload_rects[video.numfb]->clear(upload_rects[video.numfb]);
#else
	SDL_UpdateRect(video.screen, 0,0,0,0);
#endif
}

#endif

/* Search biggest video mode, calculate its ratio */

void video_detect_aspect(void)
{
	int max_w=0, max_h=0, ratio_w[4];

	searchBiggestMode(&max_w, &max_h);

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

static void setVideoMode(int width, int height, int bpp)
{
	int i, sw,sh;

	/* Search nearest fullscreen mode */
	if (video.flags & REEVENGI_SDLFULLSCREEN_FLAG) {
		findNearestMode(&width, &height, bpp);
		logMsg(1, "video: found nearest %dx%d\n", width, height);
	}

#if SDL_VERSION_ATLEAST(2,0,0)
	/* Toggle fullscreen ? */
	if (video.window) {
		Uint32 flags = SDL_GetWindowFlags(video.window);

		if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != (video.flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			logMsg(1, "video: toggle fullscreen\n");
			SDL_SetWindowFullscreen(video.window,
				(video.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP ?
				SDL_WINDOW_FULLSCREEN_DESKTOP :
				SDL_FALSE
			);
		}
	}

	/* Resize window ? */
	if (video.renderer && video.window) {
		logMsg(1, "video: resize window to %dx%d\n", width, height);
		SDL_SetWindowSize(video.window, width, height);
		SDL_RenderSetViewport(video.renderer, NULL);
	} else {
		if (video.renderer) {
			SDL_DestroyRenderer(video.renderer);
			video.renderer=NULL;
		}
		if (video.window) {
			SDL_DestroyWindow(video.window);
			video.window=NULL;
		}

		logMsg(1, "video: create window %dx%d\n", width, height);
		video.window = SDL_CreateWindow(PACKAGE_STRING, SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, width, height, video.flags);
		if (video.window) {
			logMsg(1, "video: create renderer\n");
			video.renderer = SDL_CreateRenderer(video.window, -1, SDL_RENDERER_PRESENTVSYNC);
		}
	}

	sw = MAX(320, width);
	sh = MAX(240, height);
	/* Recreate video surface if bigger is needed */
	if (video.screen) {
		if ((sw>video.screen->w) || (sh>video.screen->h)) {
			SDL_FreeSurface(video.screen);
			video.screen=NULL;
		}
	}
	/* And then the texture */
	if (!video.screen || !video.texture) {
		if (video.texture) {
			SDL_DestroyTexture(video.texture);
			video.texture = NULL;
		}
	}

	if (!video.screen) {
		logMsg(1, "video: get surface\n");
		video.screen = SDL_CreateRGBSurface(0, sw, sh, 32, 255<<16,255<<8,255,255<24);
	}
	if (!video.texture && video.renderer) {
		logMsg(1, "video: get texture\n");
		video.texture = SDL_CreateTexture(video.renderer, SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING, sw, sh);
	}
#else
	video.screen = SDL_SetVideoMode(width, height, bpp, video.flags);
	if (!video.screen) {
		/* Try 8 bpp if failed in true color */
		video.screen = SDL_SetVideoMode(width, height, 8, video.flags);
	}
#endif

	if (!video.screen) {
		fprintf(stderr, "Can not set %dx%dx%d mode\n", width, height, bpp);
		return;
	}

	video.width = video.screen->w;
	video.height = video.screen->h;
	video.bpp = video.screen->format->BitsPerPixel;
#if SDL_VERSION_ATLEAST(2,0,0)
#else
	video.flags = video.screen->flags;
#endif

	/* Set 216 color palette */
	if (video.bpp==8) {
		dither_setpalette(video.screen);
	}

	for (i=0; i<2; i++) {
		dirty_rects[i]->resize(dirty_rects[i], video.width, video.height);
		upload_rects[i]->resize(upload_rects[i], video.width, video.height);
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

/* Update background from rectangle list */
static int updateDirtyRects(void)
{
	int i, x, y;

	i = upload_rects[video.numfb]->width * upload_rects[video.numfb]->height;
	if (i>video.num_list_rects) {
		video.list_rects = (SDL_Rect *) realloc(video.list_rects, i * sizeof(SDL_Rect));
		video.num_list_rects = i;
	}

	i = 0;
	for (y=0; y<upload_rects[video.numfb]->height; y++) {
		int block_w = 0;
		int block_x = 0;
		int num_rows = 16;

		if (((y+1)<<4) > video.height) {
			num_rows = video.height - (y<<4);
		}

		for (x=0; x<upload_rects[video.numfb]->width; x++) {
			/* Force update on last column */
			int block_update = (x==upload_rects[video.numfb]->width-1);
			int num_cols = 16;

			if (((x+1)<<4) > video.width) {
				num_cols = video.width - (x<<4);
			}

			if (upload_rects[video.numfb]->markers[y*upload_rects[video.numfb]->width + x]) {
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

	return i;
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
	if ((video.flags & REEVENGI_SDLFULLSCREEN_FLAG) == REEVENGI_SDLFULLSCREEN_FLAG)
	{
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

static void setPalette(SDL_Surface *surf)
{
	SDL_Palette *surf_palette;

	if (video.bpp>8)
		return;

	surf_palette = surf->format->palette;

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetPaletteColors(video.screen->format->palette, &(surf_palette->colors[0]), 0, surf_palette->ncolors);
#else
	SDL_SetPalette(video.screen, SDL_LOGPAL|SDL_PHYSPAL, &(surf_palette->colors[0]), 0, surf_palette->ncolors);
#endif
}
