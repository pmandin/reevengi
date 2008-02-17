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

/*--- Variables ---*/

static int zoomw = 0, zoomh = 0;
static int *zoomx = NULL, *zoomy = NULL;
static SDL_Surface *zoom_surf = NULL;

/*--- Function prototypes ---*/

static void setVideoMode(video_t *this, int width, int height, int bpp);
static void swapBuffers(video_t *this);
static void screenShot(video_t *this);
static void initScreen(video_t *this);
static void refreshViewport(video_t *this);
static void refreshScreen(video_t *this);
static void drawBackground(video_t *this, video_surface_t *surf);
static void drawBackgroundZoomed(video_t *this, video_surface_t *surf);

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
	this->refreshViewport = refreshViewport;
	this->refreshScreen = refreshScreen;
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

	if (zoomx) {
		free(zoomx);
		zoomx = NULL;
		zoomw = 0;
	}
	if (zoomy) {
		free(zoomy);
		zoomy = NULL;
		zoomh = 0;
	}
	if (zoom_surf) {
		SDL_FreeSurface(zoom_surf);
		zoom_surf = NULL;
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
	this->refreshViewport(this);
}

static void refreshViewport(video_t *this)
{
	int cur_asp_x = aspect_x, cur_asp_y = aspect_y;
	int pos_x, pos_y, scr_w, scr_h;

	/* Disable 5:4 ratio in fullscreen */
	if ((this->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) {
		if ((aspect_x == 5) && (aspect_y == 4)) {
			cur_asp_x = 4;
			cur_asp_y = 3;
		}
	}

	scr_w = (this->height * cur_asp_x) / cur_asp_y;
	scr_h = (this->width * cur_asp_y) / cur_asp_x;
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

	/*printf("viewport %d,%d %dx%d\n",
		this->viewport.x, this->viewport.y,
		this->viewport.w, this->viewport.h
	);*/
}

static void refreshScreen(video_t *this)
{
	this->dirty_rects->setDirty(this->dirty_rects, 0,0, video.width, video.height);
}

static void drawBackground(video_t *this, video_surface_t *surf)
{
	SDL_Rect src_rect, dst_rect;
	int x,y;

	if (!this->screen || !surf) {
		return;
	}

	drawBackgroundZoomed(this, surf);
	return;

	/* Center background image on target screen */
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

	/* Refresh only dirtied parts of background */
	for (y=0; y<surf->getSurface(surf)->h >> 4; y++) {
		for (x=0; x<surf->getSurface(surf)->w >> 4; x++) {
			SDL_Rect src_rect1, dst_rect1;

			memcpy(&src_rect1, &src_rect, sizeof(SDL_Rect));
			src_rect1.x += (x<<4);
			src_rect1.y += (y<<4);
			src_rect1.w = src_rect1.h = 1<<4;
			memcpy(&dst_rect1, &dst_rect, sizeof(SDL_Rect));
			dst_rect1.x += (x<<4);
			dst_rect1.y += (y<<4);
			dst_rect1.w = dst_rect1.h = 1<<4;

			/* Out of target screen, out of source image ? */
			if ((dst_rect1.x<-16) || (dst_rect1.x>this->dirty_rects->width<<4)
			  || (dst_rect1.y<-16) || (dst_rect1.y>this->dirty_rects->height<<4)
			  || (src_rect1.x<-16) || (src_rect1.x>surf->getSurface(surf)->w)
			  || (src_rect1.y<-16) || (src_rect1.y>surf->getSurface(surf)->h))
			{
				continue;
			}

			if (this->dirty_rects->markers[(dst_rect1.y>>4)*this->dirty_rects->width + (dst_rect1.x>>4)] == 0) {
				continue;
			}

			SDL_BlitSurface(surf->getSurface(surf), &src_rect1, this->screen, &dst_rect1);
		}
	}
}

/* Zoom background image to viewport */

static void drawBackgroundZoomed(video_t *this, video_surface_t *surf)
{
	int recreate_surf = 0, i, x,y;
	SDL_Surface *src_surf;

	if (!this->screen || !surf) {
		return;
	}

	src_surf = surf->getSurface(surf);
	if (zoomw != this->viewport.w) {
		zoomw = this->viewport.w;
		zoomx = realloc(zoomx, sizeof(int) * zoomw);
		for (i=0; i<zoomw; i++) {
			zoomx[i] = (i * src_surf->w) / zoomw;
		}
		recreate_surf = 1;
	}
	if (zoomh != this->viewport.h) {
		zoomh = this->viewport.h;
		zoomy = realloc(zoomy, sizeof(int) * zoomh);
		for (i=0; i<zoomh; i++) {
			zoomy[i] = (i * src_surf->h) / zoomh;
		}
		recreate_surf = 1;
	}
	if (recreate_surf) {
		if (zoom_surf) {
			SDL_FreeSurface(zoom_surf);
		}
		/* Create surface with same bpp as source one */
		zoom_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, zoomw, zoomh,
			surf->getSurface(surf)->format->BitsPerPixel, 0,0,0,0);
		/* Set palette */
		if (zoom_surf->format->BitsPerPixel == 8) {
			SDL_Color palette[256];
			for (i=0;i<256;i++) {
				palette[i].r = src_surf->format->palette->colors[i].r;
				palette[i].g = src_surf->format->palette->colors[i].g;
				palette[i].b = src_surf->format->palette->colors[i].b;
			}
			SDL_SetPalette(zoom_surf, SDL_LOGPAL, palette, 0, 256);
		}
	}
	if (!zoom_surf) {
		return;
	}

	/* For each target dirty rectangle, zoom the corresponding source rectangle */
	for (y=0; y<this->dirty_rects->height; y++) {
		int dst_y1, dst_y2;

		/* Target destination of zoomed image */
		dst_y1 = ((y<<4) * zoomh) / src_surf->h;
		dst_y2 = (((y+1)<<4) * zoomh) / src_surf->h;

		/* Clip to viewport */
		if (dst_y1<this->viewport.y) {
			dst_y1 = this->viewport.y;
		}
		if (dst_y2>this->viewport.y+this->viewport.h) {
			dst_y2 = this->viewport.y+this->viewport.h;
		}

		if (dst_y1>=dst_y2) {
			continue;
		}

		for (x=0; x<this->dirty_rects->width; x++) {
			/*SDL_Rect src_rect, dst_rect;*/
			int dst_x1, dst_x2;
			Uint8 *src_pixels, dst_pixels;

			if (this->dirty_rects->markers[y*this->dirty_rects->width + x] == 0) {
				continue;
			}

			/* Zoom 16x16 block */
			dst_x1 = ((x<<4) * zoomw) / src_surf->w;
			dst_x2 = (((x+1)<<4) * zoomw) / src_surf->w;

			/* Clip to viewport */
			if (dst_x1<this->viewport.x) {
				dst_x1 = this->viewport.x;
			}
			if (dst_x2>this->viewport.x+this->viewport.w) {
				dst_x2 = this->viewport.x+this->viewport.w;
			}

			if (dst_x1>=dst_x2) {
				continue;
			}

			/*printf("refresh %d,%d -> %d,%d\n",
				dst_x1, dst_y1, dst_x2,dst_y2
			);*/

		}
	}
}
