/*
	Movie viewer
	SDL2 backend

	Copyright (C) 2017	Patrice Mandin

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#ifdef ENABLE_OPENGL
#include <SDL_opengl.h>
#endif

#ifdef ENABLE_MOVIES
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avio.h>
#include <libavutil/pixdesc.h>
#	ifdef HAVE_LIBAVUTIL_PIXFMT_H
#	include <libavutil/pixfmt.h>
#	endif
#include <libswscale/swscale.h>
#endif

#include "log.h"
#include "view_movie.h"
#include "video.h"

#if defined(ENABLE_MOVIES) && SDL_VERSION_ATLEAST(2,0,0)

/*--- Defines ---*/

/*--- Variables ---*/

static SDL_Texture *overlay = NULL;

/*--- Functions prototypes ---*/

static void movie_refresh_soft_sdl2(SDL_Surface *screen);
static void movie_stop_soft_sdl2(void);
static void movie_scale_frame_soft_sdl2(void);
static void movie_update_frame_soft_sdl2(SDL_Rect *rect);

/*--- Functions ---*/

void view_movie_init_sdl2(void)
{
	view_movie.refresh = movie_refresh_soft_sdl2;
	view_movie.stop = movie_stop_soft_sdl2;
	view_movie.scale_frame = movie_scale_frame_soft_sdl2;
	view_movie.update_frame = movie_update_frame_soft_sdl2;
}

static void movie_refresh_soft_sdl2(SDL_Surface *screen)
{
	AVCodecContext *vCodecCtx = (AVCodecContext *) view_movie.vCodecCtx;

	if (overlay) {
		SDL_DestroyTexture(overlay);
		overlay=NULL;
	}

	if (!vCodecCtx) {
		return;
	}

	logMsg(1, "movie: create overlay %dx%d\n", vCodecCtx->width, vCodecCtx->height);

	overlay = SDL_CreateTexture(video.renderer, SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STATIC/*STREAMING*/,
		vCodecCtx->width, vCodecCtx->height);
	if (!overlay) {
		fprintf(stderr, "Can not create overlay\n");
	}
}

static void movie_stop_soft_sdl2(void)
{
	if (view_movie.yPlane) {
		free(view_movie.yPlane);
		view_movie.yPlane = view_movie.uPlane = view_movie.vPlane = NULL;
	}

	if (overlay) {
		SDL_DestroyTexture(overlay);
		overlay=NULL;
	}
}

static void movie_scale_frame_soft_sdl2(void)
{
	AVCodecContext *vCodecCtx = (AVCodecContext *) view_movie.vCodecCtx;
	struct SwsContext *img_convert_ctx = (struct SwsContext *) view_movie.img_convert_ctx;
	AVFrame *decoded_frame = (AVFrame *) view_movie.decoded_frame;
	AVPicture pict;

	if (!img_convert_ctx) {
		SDL_UpdateYUVTexture(overlay, NULL,
			decoded_frame->data[0], decoded_frame->linesize[0],
			decoded_frame->data[1], decoded_frame->linesize[1],
			decoded_frame->data[2], decoded_frame->linesize[2]);
	} else {
		pict.data[0] = view_movie.yPlane;
		pict.data[1] = view_movie.uPlane;
		pict.data[2] = view_movie.vPlane;

		pict.linesize[0] = vCodecCtx->width;
		pict.linesize[1] = vCodecCtx->width >> 2;
		pict.linesize[2] = vCodecCtx->width >> 2;

		sws_scale(img_convert_ctx,
			(const uint8_t * const*) decoded_frame->data, decoded_frame->linesize,
			0, vCodecCtx->height,
			pict.data, pict.linesize);

		SDL_UpdateYUVTexture(overlay, NULL,
			pict.data[0], pict.linesize[0],
			pict.data[1], pict.linesize[1],
			pict.data[2], pict.linesize[2]);
	}
}

static void movie_update_frame_soft_sdl2(SDL_Rect *rect)
{
	SDL_RenderCopy(video.renderer, overlay, NULL, rect);
}

#endif /* defined(ENABLE_MOVIES) && SDL_VERSION_ATLEAST(2,0,0) */
