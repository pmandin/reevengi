/*
	Movie viewer

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

#include "filesystem.h"
#include "log.h"
#include "view_movie.h"
#include "clock.h"
#include "parameters.h"
#include "video.h"

#ifdef ENABLE_OPENGL
#include "r_opengl/dyngl.h"
#endif

#include "g_common/game.h"
#include "g_common/room.h"

#include "g_re1/game_re1.h"
#include "g_re2/game_re2.h"
#include "g_re3/game_re3.h"

#include "r_common/render.h"

/*--- Defines ---*/

#define KEY_MOVIE_DOWN		SDLK_z
#define KEY_MOVIE_UP		SDLK_s
#define KEY_MOVIE_RESET		SDLK_x

#define BUFSIZE 32768

#define RAW_CD_SECTOR_SIZE	2352
#define DATA_CD_SECTOR_SIZE	2048

#define CD_SYNC_SIZE 12
#define CD_SEC_SIZE 4
#define CD_XA_SIZE 8
#define CD_DATA_SIZE 2048

#define STR_MAGIC 0x60010180

/* Pixel formats */

#ifdef ENABLE_MOVIES
#	ifdef HAVE_LIBAVUTIL_PIXFMT_H
#		define REEVENGI_RGBA_FORMAT	AV_PIX_FMT_RGBA
#		define REEVENGI_YUV_FORMAT	AV_PIX_FMT_YUV420P
#	else
#		ifdef AV_PIX_FMT_RGBA
#			define REEVENGI_RGBA_FORMAT	AV_PIX_FMT_RGBA
#			define REEVENGI_YUV_FORMAT	AV_PIX_FMT_YUV420P
#		else
#			define REEVENGI_RGBA_FORMAT	PIX_FMT_RGBA
#			define REEVENGI_YUV_FORMAT	PIX_FMT_YUV420P
#		endif
#  endif
#endif

/*--- Global variables ---*/

view_movie_t view_movie;

/*--- Variables ---*/

static int restart_movie = 1;
static int first_time = 1;

#ifdef ENABLE_MOVIES
static AVInputFormat input_fmt;
static AVFormatContext *fmt_ctx = NULL;
/*static AVFrame *decoded_frame = NULL;*/
/*static AVCodecContext *vCodecCtx = NULL;*/
static AVIOContext *avio_ctx;
struct SwsContext *img_convert_ctx = NULL;
/*static AVPicture pict;*/
#endif
static char *tmpbuf = NULL;
static SDL_RWops *movie_src = NULL;

static int audstream = -1, vidstream = -1;
static int fps_num = 1, fps_den = 1;
static int emul_cd;
static int emul_cd_pos;

#if SDL_VERSION_ATLEAST(2,0,0)
/*static SDL_Texture *overlay = NULL;
static Uint8 *yPlane = NULL, *uPlane = NULL, *vPlane = NULL;*/
#else
static SDL_Overlay *overlay = NULL;
#endif
static Uint32 start_tic, current_tic;

render_texture_t *vid_texture = NULL;

/*--- Functions prototypes ---*/

static void movie_refresh_soft(SDL_Surface *screen);
static void movie_refresh_opengl(SDL_Surface *screen);

static int movie_start(const char *filename, SDL_Surface *screen);

static void movie_stop(void);
static void movie_stop_soft(void);
static void movie_stop_opengl(void);

static void check_emul_cd(void);

static int probe_movie(const char *filename);

static int movie_ioread( void *opaque, uint8_t *buf, int buf_size );
static int64_t movie_ioseek( void *opaque, int64_t offset, int whence );

static int movie_decode_video(SDL_Surface *screen);

static void movie_scale_frame_soft(void);
static void movie_scale_frame_opengl(void);

static void movie_update_frame_soft(SDL_Rect *rect);
static void movie_update_frame_opengl(SDL_Rect *rect);

/* view_movie_sdl2.c */

void view_movie_init_sdl2(void);

/*--- Functions ---*/

void movie_init(void)
{
#ifdef ENABLE_MOVIES
	logMsg(2, "movie: init\n");

	memset(&view_movie, 0, sizeof(view_movie));

	if (params.use_opengl) {
		view_movie.refresh = movie_refresh_opengl;
		view_movie.stop = movie_stop_opengl;
		view_movie.scale_frame = movie_scale_frame_opengl;
		view_movie.update_frame = movie_update_frame_opengl;
	} else {
#	if SDL_VERSION_ATLEAST(2,0,0)
		view_movie_init_sdl2();
#	else
		view_movie.refresh = movie_refresh_soft;
		view_movie.stop = movie_stop_soft;
		view_movie.scale_frame = movie_scale_frame_soft;
		view_movie.update_frame = movie_update_frame_soft;
#	endif
	}

#endif
}

void movie_shutdown(void)
{
#ifdef ENABLE_MOVIES
	view_movie.stop();
#endif
}

void movie_refresh(SDL_Surface *screen)
{
#ifdef ENABLE_MOVIES
	view_movie.refresh(screen);
#endif
}

static void movie_refresh_soft(SDL_Surface *screen)
{
#if defined(ENABLE_MOVIES) && (!SDL_VERSION_ATLEAST(2,0,0))
	AVCodecContext *vCodecCtx = (AVCodecContext *) view_movie.vCodecCtx;

	if (overlay) {
		SDL_FreeYUVOverlay(overlay);
		overlay=NULL;
	}

	if (!vCodecCtx) {
		return;
	}

	logMsg(1, "movie: create overlay %dx%d\n", vCodecCtx->width, vCodecCtx->height);
	overlay = SDL_CreateYUVOverlay(vCodecCtx->width, vCodecCtx->height,
		SDL_YV12_OVERLAY, screen);

	if (!overlay) {
		fprintf(stderr, "Can not create overlay\n");
	}
#endif
}

static void movie_refresh_opengl(SDL_Surface *screen)
{
#if defined(ENABLE_MOVIES) && defined(ENABLE_OPENGL)
	AVCodecContext *vCodecCtx = (AVCodecContext *) view_movie.vCodecCtx;

	if (!vCodecCtx) {
		return;
	}

	if (!vid_texture) {
		SDL_Surface *vid_surf;

		vid_surf = SDL_CreateRGBSurface(0, vCodecCtx->width, vCodecCtx->height,
			32, 0,0,0,0);

		if (vid_surf) {
			vid_texture = render.createTexture(0);
			if (vid_texture) {
				vid_texture->load_from_surf(vid_texture, vid_surf);
			}

			SDL_FreeSurface(vid_surf);
		}
	}

	if (!vid_texture) {
		return;
	}

	vid_texture->upload(vid_texture, 0);
#endif
}

int view_movie_input(SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN) {
		switch (event->key.keysym.sym) {
			case KEY_MOVIE_DOWN:
				game->prev_movie(game);
				restart_movie = 1;
				break;
			case KEY_MOVIE_UP:
				game->next_movie(game);
				restart_movie = 1;
				break;
			case KEY_MOVIE_RESET:
				game->reset_movie(game);
				restart_movie = 1;
				break;
			default:
				break;
		}
	}

	return 1;
}

int view_movie_update(SDL_Surface *screen)
{
	if (restart_movie) {
		logMsg(1, "Playing movie %d: %s\n", game->num_movie, game->cur_movie);
		restart_movie = 0;

		movie_stop();

		if (movie_start(game->cur_movie, screen)!=0) {
			return 0;
		}
	}

	return movie_decode_video(screen);
}

static void check_emul_cd(void)
{
	emul_cd = 0;

	/* Emulate CD read for PS1 games */
	switch(game->major) {
		case GAME_RE1:
			{
				switch(game->minor) {
					case GAME_RE1_PS1_DEMO:
					case GAME_RE1_PS1_GAME:
					case GAME_RE1_PS1_SHOCK:
						emul_cd=1;
						break;
					default:
						break;
				}
			}
			break;
		case GAME_RE2:
			{
				switch(game->minor) {
					case GAME_RE2_PS1_DEMO:
					case GAME_RE2_PS1_GAME_LEON:
					case GAME_RE2_PS1_GAME_CLAIRE:
						emul_cd=1;
						break;
					default:
						break;
				}
			}
			break;
		case GAME_RE3:
			{
				switch(game->minor) {
					case GAME_RE3_PS1_DEMO:
					case GAME_RE3_PS1_GAME:
						emul_cd=1;
						break;
					default:
						break;
				}
			}
			break;
		default:
			break;
	}
}

static int movie_start(const char *filename, SDL_Surface *screen)
{
#ifdef ENABLE_MOVIES
	int i, err;
	int dstFormat;
/*	AVPixelFormat dstFormat;*/
	int create_sws_scaler = 1;
	AVCodecContext *vCodecCtx = NULL;

	logMsg(2, "movie: init\n");

	check_emul_cd();
	emul_cd_pos = 0;

	if (probe_movie(filename)!=0) {
		fprintf(stderr, "Can not probe movie %s\n", filename);
		movie_shutdown();
		return 1;
	}
	emul_cd_pos = 0;

	movie_src = FS_makeRWops(filename);
	if (!movie_src) {
		fprintf(stderr, "Can not open movie %s\n", filename);
		movie_shutdown();
		return 1;
	}

	if (!tmpbuf) {
		tmpbuf = (char *) av_malloc(BUFSIZE);
	}

	avio_ctx = avio_alloc_context(
		tmpbuf, BUFSIZE, 0, movie_src,
		movie_ioread, NULL, movie_ioseek
	);

	fmt_ctx = avformat_alloc_context();
	fmt_ctx->pb = avio_ctx;

	err = avformat_open_input(&fmt_ctx, filename, &input_fmt, NULL);

	if (err<0) {
		fprintf(stderr,"Can not open stream: %d\n", err);
		movie_shutdown();
		return 1;
	}

	logMsg(2, "movie: avformat_find_stream_info %p\n", fmt_ctx);
	err = avformat_find_stream_info(fmt_ctx, NULL);
	if (err<0) {
		fprintf(stderr,"Can not find stream info: %d\n", err);
		movie_shutdown();
		return 1;
	}

	logMsg(2, "movie: av_dump_format %p\n", fmt_ctx);
	av_dump_format(fmt_ctx, 0, filename, 0);

	for (i=0; i<fmt_ctx->nb_streams; i++) {
		AVCodecContext *cc;
		AVCodec *codec;
		AVStream *stream;
		int ret;

		cc = avcodec_alloc_context3(NULL);
		if (!cc) {
			fprintf(stderr, "Out of memory\n");
			continue;
		}

		stream = fmt_ctx->streams[i];

		ret = avcodec_parameters_to_context(cc, stream->codecpar);
		if (ret < 0) {
			avcodec_free_context(&cc);
			fprintf(stderr, "Can not read stream parameters\n");
			continue;
		}
		cc->pkt_timebase = stream->time_base;

		logMsg(2, "movie: avcodec_find_decoder 0x%08x\n", cc->codec_id);
		codec = avcodec_find_decoder(cc->codec_id);
		if (!codec) {
			avcodec_free_context(&cc);
			fprintf(stderr, "Can not find codec for stream %d\n", i);
			continue;
		}

		logMsg(2, "movie: avcodec_open2 %p %p\n", cc, codec);
		err = avcodec_open2(cc, codec, NULL);
		if (err<0) {
			avcodec_free_context(&cc);
			fprintf(stderr, "Can not open codec for stream %d\n", i);
			continue;
		}

		switch(cc->codec_type) {
			case AVMEDIA_TYPE_VIDEO:
				vidstream = i;
				view_movie.vCodecCtx = vCodecCtx = cc;
				fps_num = stream->time_base.num;
				fps_den = stream->time_base.den;
				logMsg(1, "movie: fps: %d\n", fps_den/fps_num);
				break;
			case AVMEDIA_TYPE_AUDIO:
				audstream = i;
				break;
			default:
				break;
		}
	}

	logMsg(2, "movie: avcodec_alloc_frame\n");
	/*decoded_frame = avcodec_alloc_frame();*/
	view_movie.decoded_frame = av_frame_alloc();
	if (!view_movie.decoded_frame) {
		fprintf(stderr, "Can not alloc decoded frame\n");
		return 1;
	}

	if (params.use_opengl) {
		dstFormat = REEVENGI_RGBA_FORMAT;
	} else {
		dstFormat = REEVENGI_YUV_FORMAT;
	}

#if SDL_VERSION_ATLEAST(2,0,0)
	if (vCodecCtx->pix_fmt == REEVENGI_YUV_FORMAT) {
		create_sws_scaler = 0;
	} else {
		int yPlaneSz, uvPlaneSz;

		yPlaneSz = vCodecCtx->width * vCodecCtx->height;
		uvPlaneSz = vCodecCtx->width * vCodecCtx->height / 4;

		view_movie.yPlane = (Uint8*) realloc(view_movie.yPlane, yPlaneSz + uvPlaneSz*2);
		if (view_movie.yPlane) {
			view_movie.uPlane = &view_movie.yPlane[yPlaneSz];
			view_movie.vPlane = &view_movie.yPlane[yPlaneSz+uvPlaneSz];
		}
	}
#endif

	if (create_sws_scaler) {
		logMsg(2, "movie: sws_getContext\n");
		view_movie.img_convert_ctx = sws_getContext(
			vCodecCtx->width, vCodecCtx->height, vCodecCtx->pix_fmt,
			vCodecCtx->width, vCodecCtx->height, dstFormat,
			SWS_POINT, NULL, NULL, NULL);
		if (!view_movie.img_convert_ctx) {
			fprintf(stderr, "Could not create sws scaling context\n");
			return 1;
		}
	}

	view_movie.refresh(screen);

	start_tic = 0;
#endif
	return 0;
}

void movie_stop(void)
{
	logMsg(2, "movie: stop\n");

	audstream = vidstream = -1;
	emul_cd = 0;

	view_movie.stop();

#ifdef ENABLE_MOVIES
	if (view_movie.decoded_frame) {
		av_free((AVFrame *) view_movie.decoded_frame);
		view_movie.decoded_frame = NULL;
	}

	if (view_movie.vCodecCtx) {
		AVCodecContext *cc = (AVCodecContext *) view_movie.vCodecCtx;

		avcodec_free_context(&cc);
		view_movie.vCodecCtx = NULL;
	}

	if (fmt_ctx) {
		avformat_free_context(fmt_ctx);
		fmt_ctx = NULL;
	}

	if (tmpbuf) {
		av_free(tmpbuf);
		tmpbuf=NULL;
	}

	if (view_movie.img_convert_ctx) {
		sws_freeContext(view_movie.img_convert_ctx);
		view_movie.img_convert_ctx = NULL;
	}
#endif

	if (movie_src) {
		SDL_RWclose(movie_src);
		movie_src = NULL;
	}
}

static void movie_stop_soft(void)
{
#if defined(ENABLE_MOVIES) && (!SDL_VERSION_ATLEAST(2,0,0))
	if (overlay) {
		SDL_FreeYUVOverlay(overlay);
		overlay=NULL;
	}
#endif
}

static void movie_stop_opengl(void)
{
#ifdef ENABLE_OPENGL
	if (vid_texture) {
		vid_texture->shutdown(vid_texture);
		vid_texture = NULL;
	}
#endif
}

/* Read first 2Ko to probe */

static int probe_movie(const char *filename)
{
	int retval = 1;
#ifdef ENABLE_MOVIES
	SDL_RWops	*src;
	AVProbeData	pd;

	pd.filename = filename;
	pd.buf_size = RAW_CD_SECTOR_SIZE;
	pd.buf = (unsigned char *) malloc(pd.buf_size);
	if (!pd.buf) {
		fprintf(stderr, "Can not allocate %d bytes to probe movie\n", pd.buf_size);
		return retval;
	}

	src = FS_makeRWops(filename);
	if (src) {
		if (movie_ioread( src, pd.buf, pd.buf_size) == pd.buf_size) {
			AVInputFormat *fmt = av_probe_input_format(&pd, 1);
			if (fmt) {
				logMsg(1, "movie: input format probed\n");
				memcpy(&input_fmt, fmt, sizeof(AVInputFormat));
				retval = 0;
			} else {
				fprintf(stderr, "Failed probing\n");
			}
		} else {
			fprintf(stderr, "Error reading file %s for probing\n", filename);
		}
		SDL_RWclose(src);
	} else {
		fprintf(stderr, "Can not open file %s for probing\n", filename);
	}

	free(pd.buf);
#endif
	return retval;
}

static int movie_ioread( void *opaque, uint8_t *buf, int buf_size )
{
	int size_read = 0;

	if (emul_cd) {
		/* TODO: skip non video sectors */
		/* TODO: detect audio sectors */

		logMsg(2, "cd: pos %d, read %d\n", emul_cd_pos, buf_size);
		if (emul_cd_pos<0)
			return -1;

		while (buf_size>0) {
			int sector_pos = emul_cd_pos % RAW_CD_SECTOR_SIZE;
			int max_size;
			int pos_data_type = -1; /* need to set data type */
			int is_video = 0;

			logMsg(2,"cd:  generate sector %d, pos %d, remains %d\n",
				emul_cd_pos / RAW_CD_SECTOR_SIZE, sector_pos,
				buf_size);
			while ((sector_pos<CD_SYNC_SIZE) && (buf_size>0)) {
				buf[size_read++] = ((sector_pos==0) || (sector_pos==11)) ? 0 : 0xff;
				buf_size--;
				sector_pos++;
				emul_cd_pos++;
			}
			while ((sector_pos<CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE) && (buf_size>0)) {
				if (sector_pos == 0x12) {
					pos_data_type = size_read;
				}
				buf[size_read++] = 0;
				buf_size--;
				sector_pos++;
				emul_cd_pos++;
			}
			while ((sector_pos<CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE+CD_DATA_SIZE) && (buf_size>0)) {
				max_size = CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE+CD_DATA_SIZE-sector_pos;
				max_size = (max_size>buf_size) ? buf_size : max_size;
				logMsg(3, "cd: reading real data at 0x%08x in file, %d\n", SDL_RWtell((SDL_RWops *)opaque), max_size);
				if (SDL_RWread((SDL_RWops *)opaque, &buf[size_read], max_size, 1)<1) {
					return -1;
				}
				if ((sector_pos == CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE) && (max_size>=4)) {
					/* Read first bytes */
					Uint32 magic = *((Uint32 *) &buf[size_read]);
					is_video = (SDL_SwapBE32(magic) == STR_MAGIC);
				}
				buf_size -= max_size;
				size_read += max_size;
				sector_pos += max_size;
				emul_cd_pos += max_size;
			}
			while ((sector_pos<RAW_CD_SECTOR_SIZE) && (buf_size>0)) {
				buf[size_read++] = 0;
				buf_size--;
				sector_pos++;
				emul_cd_pos++;
			}

			/* set data type */
			if (pos_data_type>=0) {
				if (is_video) {
					logMsg(3, "cd: generate video\n");
					buf[pos_data_type] = 0x08;
				} else {
					logMsg(3, "cd: generate audio\n");
					buf[pos_data_type] = 0x04;
				}
			}
		}
	} else {
		if (SDL_RWread((SDL_RWops *)opaque, buf, buf_size, 1)<1) {
			return -1;
		}
		size_read = buf_size;
	}

	logMsg(2, "cd: after read: pos %d, read %d\n", emul_cd_pos, size_read);
	return size_read;
}

static int64_t movie_ioseek( void *opaque, int64_t offset, int whence )
{
	int64_t new_offset = offset;

	logMsg(2, "cd: ioseek %d, %d\n", offset, whence);

	if (emul_cd) {
		int sector_num, sector_pos;
		int64_t file_offset;

		switch(whence) {
			case RW_SEEK_SET:
			case RW_SEEK_END:
				emul_cd_pos = (int) offset;
				break;
			case RW_SEEK_CUR:
				emul_cd_pos += (int) offset;
				break;
		}

		sector_num = emul_cd_pos / RAW_CD_SECTOR_SIZE;
		sector_pos = emul_cd_pos % RAW_CD_SECTOR_SIZE;

		if (sector_pos<CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE) {
			sector_pos = 0;
		} else if ((sector_pos>=CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE) &&
			  (sector_pos<CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE+CD_DATA_SIZE))
		{
			sector_pos -= CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE;
		} else if (sector_pos>=CD_SYNC_SIZE+CD_SEC_SIZE+CD_XA_SIZE+CD_DATA_SIZE) {
			sector_pos = 0;
			sector_num++;
		}

		file_offset = (sector_num * DATA_CD_SECTOR_SIZE) + sector_pos;
		SDL_RWseek((SDL_RWops *)opaque, file_offset, whence);

		new_offset = emul_cd_pos;
	} else {
		new_offset = SDL_RWseek((SDL_RWops *)opaque, offset, whence);
	}

	return new_offset;
}

static int movie_decode_video(SDL_Surface *screen)
{
	int retval = 0;
#ifdef ENABLE_MOVIES
	AVCodecContext *vCodecCtx = (AVCodecContext *) view_movie.vCodecCtx;
	AVFrame *decoded_frame = (AVFrame *) view_movie.decoded_frame;
	AVPacket pkt;
	int err, got_pic = 0;
	int current_frame;

	if (start_tic == 0) {
		current_tic = start_tic = clockGet();
	} else {
		current_tic = clockGet();
	}
	/* 33333/1000000 = 0.033333s per frame or 33.333ms per frame  */
	current_frame = ((current_tic-start_tic) * fps_den) / (fps_num * 1000);

	if (!fmt_ctx) {
		return retval;
	}

	logMsg(2, "movie: av_read_frame %p %p at 0x%08x\n", fmt_ctx, &pkt, emul_cd_pos);
	err = av_read_frame(fmt_ctx, &pkt);
	if (err<0) {
		logMsg(1, "movie: eof\n");
		movie_stop();
		return retval;
	}

	if (pkt.stream_index == vidstream) {
		/* Decode video packet */
		logMsg(2, "movie: avcodec_send_packet %p %p\n", vCodecCtx, &pkt);
		err = avcodec_send_packet(vCodecCtx, &pkt);

		if (err<0) {
			fprintf(stderr, "Error decoding frame: %d\n", err);
		} else {
			err = avcodec_receive_frame(vCodecCtx, decoded_frame);
			if (err==0) {
				got_pic=1;
			}
			retval = 1;
		}
	} else if (pkt.stream_index == audstream) {
		logMsg(2, "movie: audio packet\n");
	} else {
		logMsg(2, "movie: unknown packet\n");
	}

	/* Scale and decode frame */
	if (got_pic) {
		if (current_frame < decoded_frame->pts) {
			int delay_frame = decoded_frame->pts - current_frame;
			int delay_ms = (delay_frame * 1000 * fps_num) / fps_den;
			if (delay_ms>0) {
				/*logMsg(2, "movie: wait %d\n", delay_ms);*/
				SDL_Delay(delay_ms);
			}
		}

		logMsg(2, "movie: decode and scale frame\n");
		view_movie.scale_frame();
	}

	/* Display current decoded frame */
	{
		SDL_Rect rect;
		int vid_w = vCodecCtx->width;
		int vid_h = vCodecCtx->height;
		int w2, h2;

		/* Rescale to biggest visible screen size */
		w2 = (vid_w * screen->h) / vid_h;
		h2 = (vid_h * screen->w) / vid_w;
		if (w2>screen->w) {
			w2 = screen->w;
		} else if (h2>screen->h) {
			h2 = screen->h;
		}
		rect.x = (screen->w - w2)>>1;
		rect.y = (screen->h - h2)>>1;
		rect.w = w2;
		rect.h = h2;

		/*logMsg(2, "movie: update frame to %d,%d %dx%d\n",rect.x,rect.y,rect.w,rect.h);*/
		view_movie.update_frame(&rect);
	}

	av_packet_unref(&pkt);
#endif

	return retval;
}

static void movie_scale_frame_soft(void)
{
#if defined(ENABLE_MOVIES) && (!SDL_VERSION_ATLEAST(2,0,0))
	AVCodecContext *vCodecCtx = (AVCodecContext *) view_movie.vCodecCtx;
	struct SwsContext *img_convert_ctx = (struct SwsContext *) view_movie.img_convert_ctx;
	AVFrame *decoded_frame = (AVFrame *) view_movie.decoded_frame;
	uint8_t *pixels[4]={NULL,NULL,NULL,NULL};
	int linesize[4]={0,0,0,0};

	SDL_LockYUVOverlay(overlay);

	pixels[0] = overlay->pixels[0];
	pixels[1] = overlay->pixels[2];
	pixels[2] = overlay->pixels[1];

	linesize[0] = overlay->pitches[0];
	linesize[1] = overlay->pitches[2];
	linesize[2] = overlay->pitches[1];

	sws_scale(img_convert_ctx,
		(const uint8_t * const*) decoded_frame->data, decoded_frame->linesize,
		0, vCodecCtx->height,
		pixels, linesize);

	SDL_UnlockYUVOverlay(overlay);
#endif
}

static void movie_update_frame_soft(SDL_Rect *rect)
{
#if defined(ENABLE_MOVIES) && (!SDL_VERSION_ATLEAST(2,0,0))
	SDL_DisplayYUVOverlay(overlay, rect);
#endif
}

static void movie_scale_frame_opengl(void)
{
#if defined(ENABLE_MOVIES) && defined(ENABLE_OPENGL)
	AVCodecContext *vCodecCtx = (AVCodecContext *) view_movie.vCodecCtx;
	struct SwsContext *img_convert_ctx = (struct SwsContext *) view_movie.img_convert_ctx;
	AVFrame *decoded_frame = (AVFrame *) view_movie.decoded_frame;
	uint8_t *pixels[4]={NULL,NULL,NULL,NULL};
	int linesize[4]={0,0,0,0};

	pixels[0] = vid_texture->pixels;
	linesize[0] = vid_texture->pitch;

	sws_scale(img_convert_ctx,
		(const uint8_t * const*) decoded_frame->data, decoded_frame->linesize,
		0, vCodecCtx->height,
		pixels, linesize);

	vid_texture->update(vid_texture, 0);
#endif
}

static void movie_update_frame_opengl(SDL_Rect *rect)
{
#if defined(ENABLE_MOVIES) && defined(ENABLE_OPENGL)
	int dst_x, dst_y, dst_w, dst_h;

	render.set_dithering(params.dithering);
	render.set_useDirtyRects(0);
	render.set_texture(0, vid_texture);

	dst_w = (vid_texture->w * video.height) / vid_texture->h;
	dst_h = (vid_texture->h * video.width) / vid_texture->w;
	if (dst_w > video.width) {
		/* Use dst_h */
		dst_w = (vid_texture->w * dst_h) / vid_texture->h;
	} else if (dst_h > video.height) {
		/* Use dst_w */
		dst_h = (vid_texture->h * dst_w) / vid_texture->w;
	}
	dst_x = (video.width - dst_w)>>1;
	dst_y = (video.height - dst_h)>>1;

/*	logMsg(2, "movie: screen: %dx%d, viewport: %d,%d %dx%d movie: %dx%d with ratio: %d,%d %dx%d\n",
		video.width, video.height,
		video.viewport.x,video.viewport.y,
		video.viewport.w,video.viewport.h,
		vid_texture->w, vid_texture->h,
		dst_x, dst_y, dst_w, dst_h);*/

	render.bitmap.clipSource(0,0,0,0);
	render.bitmap.clipDest(
		dst_x, dst_y,
		dst_w, dst_h);
	render.bitmap.setScaler(
		vid_texture->w, vid_texture->h,
		dst_w, dst_h);
	render.bitmap.setDepth(0, 0.0f);
	render.bitmap.drawImage();

	render.set_dithering(0);
	render.set_useDirtyRects(0);
#endif
}
