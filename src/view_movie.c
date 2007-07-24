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

static AVInputFormat *input_fmt = NULL;
static AVFormatContext *fmt_ctx = NULL;
static ByteIOContext bio_ctx;
static char *tmpbuf = NULL;
static SDL_RWops *movie_src = NULL;

static int audstream = -1, vidstream = -1;

/*--- Functions prototypes ---*/

static int movie_init(const char *filename);
void movie_shutdown(void);

static AVInputFormat *probe_movie(const char *filename);
static int movie_ioread( void *opaque, uint8_t *buf, int buf_size );
static offset_t movie_ioseek( void *opaque, offset_t offset, int whence );

static SDL_Surface *movie_decode_video(void);

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

SDL_Surface *view_movie_update(void)
{
	/* Init ffmpeg ? */
	if (first_time) {
		first_time = 0;

		av_register_all();
	}

	if (restart_movie) {
		printf("Playing movie %d: %s\n", game_state.num_movie, game_state.cur_movie);
		restart_movie = 0;

		if (movie_init(game_state.cur_movie)!=0) {
			printf("error init\n");
			return NULL;
		}

	}

	return movie_decode_video();
}

static int movie_init(const char *filename)
{
	int i, err;

	input_fmt = probe_movie(filename);
	if (input_fmt) {
		printf("movie: %s\n", input_fmt->long_name);
	}

	/* Close current movie playing */
	if (movie_src) {
		movie_shutdown();
	}

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
	input_fmt->flags |= AVFMT_NOFILE;

	err = av_open_input_stream(&fmt_ctx, &bio_ctx, game_state.cur_movie, input_fmt, NULL);
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

		printf("movie: stream %d: ", i);
		switch(cc->codec_type) {
			case CODEC_TYPE_VIDEO:
				printf("video");
				vidstream = i;
				break;
			case CODEC_TYPE_AUDIO:
				printf("audio");
				audstream = i;
				break;
			default:
				printf("other type");
				break;
		}
		printf(" %s\n", codec->name);
	}

	return 0;
}

void movie_shutdown(void)
{
	audstream = vidstream = -1;

	input_fmt = NULL;
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

static AVInputFormat *probe_movie(const char *filename)
{
	SDL_RWops	*src;
	AVProbeData	pd;
	AVInputFormat	*fmt = NULL;

	pd.filename = filename;
	pd.buf_size = 2048;
	pd.buf = (unsigned char *) malloc(pd.buf_size);
	if (!pd.buf) {
		fprintf(stderr, "Can not allocate %d bytes to probe movie\n", pd.buf_size);
		return NULL;
	}

	src = FS_makeRWops(pd.filename);
	if (src) {
		if (SDL_RWread( src, pd.buf, pd.buf_size, 1)) {
			fmt = av_probe_input_format(&pd, 1);
		}
		SDL_RWclose(src);
	}

	free(pd.buf);
	return fmt;
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

static SDL_Surface *movie_decode_video(void)
{
	AVPacket pkt1, *pkt = &pkt1;
	int err;

	err = av_read_frame(fmt_ctx, pkt);
	if (err<0) {
		return NULL;
	}

	if (pkt->stream_index == vidstream) {
		/* Decode video packet */
	}

	av_free_packet(pkt);

	return NULL;
}
