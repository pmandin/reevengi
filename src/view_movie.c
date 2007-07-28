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

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <ffmpeg/avformat.h>

#include "state.h"
#include "filesystem.h"

/*--- Defines ---*/

#define KEY_MOVIE_DOWN		SDLK_z
#define KEY_MOVIE_UP		SDLK_s
#define KEY_MOVIE_RESET		SDLK_x

#define BUFSIZE 32768

/*--- Variables ---*/

static int restart_movie = 1;
static int first_time = 1;

static AVInputFormat input_fmt;
static AVFormatContext *fmt_ctx = NULL;
static ByteIOContext bio_ctx;
static char *tmpbuf = NULL;
static SDL_RWops *movie_src = NULL;

static int audstream = -1, vidstream = -1;
static AVFrame *decoded_frame = NULL;

static SDL_Overlay *overlay = NULL;

/*--- Functions prototypes ---*/

static int movie_init(const char *filename);
void movie_shutdown(void);

static int probe_movie(const char *filename);
static int movie_ioread( void *opaque, uint8_t *buf, int buf_size );
static offset_t movie_ioseek( void *opaque, offset_t offset, int whence );

static int movie_decode_video(SDL_Surface *screen);

/*--- Functions ---*/

int view_movie_input(SDL_Event *event)
{
	int max_movies;

	if (event->type == SDL_KEYDOWN) {
		switch (event->key.keysym.sym) {
			case KEY_MOVIE_DOWN:
				game_state.num_movie -= 1;
				if (game_state.num_movie<0) {
					game_state.num_movie = 0;
				}
				state_newmovie();
				restart_movie = 1;
				break;						
			case KEY_MOVIE_UP:
				max_movies = state_getnummovies();
				game_state.num_movie += 1;
				if (game_state.num_movie >= max_movies) {
					game_state.num_movie = max_movies-1;
				}
				state_newmovie();
				restart_movie = 1;
				break;						
			case KEY_MOVIE_RESET:
				game_state.num_movie = 0;
				state_newmovie();
				restart_movie = 1;
				break;						
		}
	}

	return 1;
}

int view_movie_update(SDL_Surface *screen)
{
	/* Init ffmpeg ? */
	if (first_time) {
		first_time = 0;

		av_register_all();
	}

	if (restart_movie) {
		printf("Playing movie %d: %s\n", game_state.num_movie, game_state.cur_movie);
		restart_movie = 0;

		movie_shutdown();

		if (movie_init(game_state.cur_movie)!=0) {
			return 0;
		}
	}

	if (!fmt_ctx) {
		return 0;
	}

	return movie_decode_video(screen);
}

static int movie_init(const char *filename)
{
	int i, err;

	if (probe_movie(filename)!=0) {
		fprintf(stderr, "Can not probe movie %s\n", filename);
		movie_shutdown();
		return 1;
	}

	printf("movie: %s\n", input_fmt.long_name);

	if (!tmpbuf) {
		tmpbuf = (char *)malloc(BUFSIZE);
	}

	movie_src = FS_makeRWops(filename);
	if (!movie_src) {
		fprintf(stderr, "Can not open movie %s\n", filename);
		movie_shutdown();
		return 1;
	}

	init_put_byte(&bio_ctx, tmpbuf, BUFSIZE, 0, movie_src,
		movie_ioread, NULL, movie_ioseek);

	input_fmt.flags |= AVFMT_NOFILE;

	err = av_open_input_stream(&fmt_ctx, &bio_ctx, game_state.cur_movie, &input_fmt, NULL);
	if (err<0) {
		fprintf(stderr,"Can not open stream: %d\n", err);
		movie_shutdown();
		return 1;
	}

	err = av_find_stream_info(fmt_ctx);
	if (err<0) {
		fprintf(stderr,"Can not find stream info: %d\n", err);
		movie_shutdown();
		return 1;
	}

	printf("movie: %d streams\n", fmt_ctx->nb_streams);
	for (i=0; i<fmt_ctx->nb_streams; i++) {
		AVCodec *codec;
		AVCodecContext *cc = fmt_ctx->streams[i]->codec;

		codec = avcodec_find_decoder(cc->codec_id);
		if (!codec) {
			fprintf(stderr, "Can not find codec for stream %d\n", i);
			continue;
		}

		err = avcodec_open(cc, codec);
		if (err<0) {
			fprintf(stderr, "Can not open codec for stream %d\n", i);
			continue;
		}

		printf("movie: stream %d: %s: ", i, codec->name);
		switch(cc->codec_type) {
			case CODEC_TYPE_VIDEO:
				printf("video, %dx%d, %08x", cc->width, cc->height, cc->pix_fmt);
				vidstream = i;
				break;
			case CODEC_TYPE_AUDIO:
				printf("audio, %d Hz, %d channels",
					cc->sample_rate, cc->channels
				);
				audstream = i;
				break;
			default:
				printf("other type");
				break;
		}
		printf("\n");
	}

	return 0;
}

void movie_shutdown(void)
{
	audstream = vidstream = -1;

	if (overlay) {
		SDL_FreeYUVOverlay(overlay);
		overlay=NULL;
	}

	if (decoded_frame) {
		av_free(decoded_frame);
		decoded_frame = NULL;
	}

	if (fmt_ctx) {
		av_close_input_file( fmt_ctx );
		fmt_ctx = NULL;
	}
	if (tmpbuf) {
		free(tmpbuf);
		tmpbuf=NULL;
	}
	if (movie_src) {
		SDL_RWclose(movie_src);
		movie_src = NULL;
	}
}

/* Read first 2Ko to probe */

static int probe_movie(const char *filename)
{
	SDL_RWops	*src;
	AVProbeData	pd;
	int retval = 0;	

	pd.filename = filename;
	pd.buf_size = 2048;
	pd.buf = (unsigned char *) malloc(pd.buf_size);
	if (!pd.buf) {
		fprintf(stderr, "Can not allocate %d bytes to probe movie\n", pd.buf_size);
		return retval;
	}

	src = FS_makeRWops(filename);
	if (src) {
		if (SDL_RWread( src, pd.buf, pd.buf_size, 1)) {
			AVInputFormat *fmt = av_probe_input_format(&pd, 1);
			memcpy(&input_fmt, fmt, sizeof(AVInputFormat));
			retval = 0;
		} else {
			fprintf(stderr, "Error reading file %s for probing\n", filename);
		}
		SDL_RWclose(src);
	} else {
		fprintf(stderr, "Can not open file %s for probing\n", filename);
	}

	free(pd.buf);
	return retval;
}

static int movie_ioread( void *opaque, uint8_t *buf, int buf_size )
{
	if (SDL_RWread((SDL_RWops *)opaque, buf, buf_size, 1)) {
		return buf_size;
	}

	return -1;
}

static offset_t movie_ioseek( void *opaque, offset_t offset, int whence )
{
	return SDL_RWseek((SDL_RWops *)opaque, offset, whence);
}

static void update_overlay_yuv420(void)
{
	int x,y;
	Uint8 *dst[3], *dst_line[3];
	Uint8 *src[3], *src_line[3];

	src[0] = decoded_frame->data[0];
	src[1] = decoded_frame->data[1];
	src[2] = decoded_frame->data[2];

	dst[0] = overlay->pixels[0];
	dst[1] = overlay->pixels[1];
	dst[2] = overlay->pixels[2];

	for (y=0; y<overlay->h; y++) {
		src_line[0] = src[0];
		dst_line[0] = dst[0];

		for (x=0; x<overlay->w; x++) {
			*(dst_line[0]++) = *(src_line[0]++);
		}

		src[0] += decoded_frame->linesize[0];
		dst[0] += overlay->pitches[0];
	}

	for (y=0; y<overlay->h>>1; y++) {
		src_line[1] = src[1];
		src_line[2] = src[2];

		dst_line[1] = dst[1];
		dst_line[2] = dst[2];

		for (x=0; x<overlay->w>>1; x++) {
			*(dst_line[1]++) = *(src_line[2]++);
			*(dst_line[2]++) = *(src_line[1]++);
		}

		src[1] += decoded_frame->linesize[1];
		src[2] += decoded_frame->linesize[2];

		dst[1] += overlay->pitches[1];
		dst[2] += overlay->pitches[2];
	}
}

static int movie_decode_video(SDL_Surface *screen)
{
	AVPacket pkt1, *pkt = &pkt1;
	int err, got_pic, retval = 0;

	err = av_read_frame(fmt_ctx, pkt);
	if (err<0) {
		return retval;
	}

	if (pkt->stream_index == vidstream) {
		if (!decoded_frame) {
			decoded_frame = avcodec_alloc_frame();
			if (!decoded_frame) {
				fprintf(stderr, "Can not alloc decoded frame\n");
				return retval;
			}
		}

		/* Decode video packet */
		err = avcodec_decode_video(
			fmt_ctx->streams[vidstream]->codec,
			decoded_frame,
			&got_pic,
			pkt->data, pkt->size);

		if (err<0) {
			fprintf(stderr, "Error decoding frame: %d\n", err);
		} else if (got_pic) {
			SDL_Rect rect;
			int vid_w = fmt_ctx->streams[vidstream]->codec->width;
			int vid_h = fmt_ctx->streams[vidstream]->codec->height;
			int w2, h2;

			rect.x = rect.y = 0;
			rect.w = vid_w;
			rect.h = vid_h;

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

			/* Create overlay */
			if (!overlay) {
				overlay = SDL_CreateYUVOverlay(vid_w, vid_h,
					SDL_YV12_OVERLAY, screen);
				if (!overlay) {
					fprintf(stderr, "Can not create overlay\n");
					return retval;
				}
			}

			/* Update overlay surface */
			SDL_LockYUVOverlay(overlay);
			update_overlay_yuv420();
			SDL_UnlockYUVOverlay(overlay);

			/* Display overlay */
			SDL_DisplayYUVOverlay(overlay, &rect);

			retval = 1;
		}
	}

	av_free_packet(pkt);

	return retval;
}
