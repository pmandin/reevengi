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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#ifdef ENABLE_MOVIES
#include <libavformat/avformat.h>
#endif

#include "filesystem.h"

#include "g_common/game.h"
#include "g_common/room.h"

#include "g_re1/game_re1.h"
#include "g_re2/game_re2.h"
#include "g_re3/game_re3.h"

#include "view_movie.h"

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

/*#define USE_OLD_AVIO_API 1*/

/*--- Variables ---*/

static int restart_movie = 1;

#ifdef ENABLE_MOVIES
static AVInputFormat input_fmt;
static AVFormatContext *fmt_ctx = NULL;
# ifdef USE_OLD_AVIO_API
static ByteIOContext bio_ctx;
# else
static AVIOContext *avio_ctx;
# endif
#endif
static char *tmpbuf = NULL;
static SDL_RWops *movie_src = NULL;

static int audstream = -1, vidstream = -1;
static int emul_cd;
static int emul_cd_pos;
#ifdef ENABLE_MOVIES
static AVFrame *decoded_frame = NULL;
#endif

static SDL_Overlay *overlay = NULL;

/*--- Functions prototypes ---*/

static int movie_init(const char *filename);

static void check_emul_cd(void);

static int probe_movie(const char *filename);
#ifdef ENABLE_MOVIES
static int movie_ioread( void *opaque, uint8_t *buf, int buf_size );
static int64_t movie_ioseek( void *opaque, int64_t offset, int whence );
#endif

static int movie_decode_video(SDL_Surface *screen);

/*--- Functions ---*/

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
#ifdef ENABLE_MOVIES
	static int first_time = 1;

	/* Init ffmpeg ? */
	if (first_time) {
		first_time = 0;

		avcodec_register_all();
		av_register_all();
	}

	if (restart_movie) {
		printf("Playing movie %d: %s\n", game->num_movie, game->cur_movie);
		restart_movie = 0;

		movie_shutdown();

		if (movie_init(game->cur_movie)!=0) {
			return 0;
		}
	}

	if (!fmt_ctx) {
		return 0;
	}

	return movie_decode_video(screen);
#else
	return 0;
#endif
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

static int movie_init(const char *filename)
{
#ifdef ENABLE_MOVIES
	int i, err;

	check_emul_cd();
	emul_cd_pos = 0;

	if (probe_movie(filename)!=0) {
		fprintf(stderr, "Can not probe movie %s\n", filename);
		movie_shutdown();
		return 1;
	}

	printf("movie: %s\n", input_fmt.long_name);

	movie_src = FS_makeRWops(filename);
	if (!movie_src) {
		fprintf(stderr, "Can not open movie %s\n", filename);
		movie_shutdown();
		return 1;
	}

	emul_cd_pos = 0;

# ifdef USE_OLD_AVIO_API
	if (!tmpbuf) {
		tmpbuf = (char *) malloc(BUFSIZE);
	}

	init_put_byte(&bio_ctx,
		tmpbuf, BUFSIZE, 0, movie_src,
		movie_ioread, NULL, movie_ioseek
	);

	input_fmt.flags |= AVFMT_NOFILE;

	err = avformat_open_input(&fmt_ctx, &bio_ctx, filename, &input_fmt, NULL);
# else
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
# endif

	if (err<0) {
		fprintf(stderr,"Can not open stream: %d\n", err);
		movie_shutdown();
		return 1;
	}


# ifdef USE_OLD_AVIO_API
	err = av_find_stream_info(fmt_ctx);
# else
	err = avformat_find_stream_info(fmt_ctx, NULL);
# endif
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

# ifdef USE_OLD_AVIO_API
		err = avcodec_open(cc, codec);
# else
		err = avcodec_open2(cc, codec, NULL);
# endif
		if (err<0) {
			fprintf(stderr, "Can not open codec for stream %d\n", i);
			continue;
		}

		printf("movie: stream %d: %s: ", i, codec->name);
		switch(cc->codec_type) {
			case AVMEDIA_TYPE_VIDEO:
				printf("video, %dx%d, %08x", cc->width, cc->height, cc->pix_fmt);
				vidstream = i;
				break;
			case AVMEDIA_TYPE_AUDIO:
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
#endif

	return 0;
}

void movie_shutdown(void)
{
	audstream = vidstream = -1;
	emul_cd = 0;

	if (overlay) {
		SDL_FreeYUVOverlay(overlay);
		overlay=NULL;
	}

#ifdef ENABLE_MOVIES
	if (decoded_frame) {
		av_free(decoded_frame);
		decoded_frame = NULL;
	}

	if (fmt_ctx) {
# ifdef USE_OLD_AVIO_API
		avformat_close_input(fmt_ctx);
# else
		avformat_free_context(fmt_ctx);
# endif
		fmt_ctx = NULL;
	}

	if (tmpbuf) {
# ifdef USE_OLD_AVIO_API
		free(tmpbuf);
# else
		av_free(tmpbuf);
# endif
		tmpbuf=NULL;
	}
#endif

	if (movie_src) {
		SDL_RWclose(movie_src);
		movie_src = NULL;
	}
}

/* Read first 2Ko to probe */

static int probe_movie(const char *filename)
{
	int retval = 1;	
#ifdef ENABLE_MOVIES
	SDL_RWops	*src;
	AVProbeData	pd;

	pd.filename = filename;
	pd.buf_size = 2048;
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
				memcpy(&input_fmt, fmt, sizeof(AVInputFormat));
				retval = 0;
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

#ifdef ENABLE_MOVIES
static int movie_ioread( void *opaque, uint8_t *buf, int buf_size )
{
	int size_read = 0;

	if (emul_cd) {
		/* TODO: skip non video sectors */
		/* TODO: detect audio sectors */

		while (buf_size>0) {
			int sector_pos = emul_cd_pos % RAW_CD_SECTOR_SIZE;
			int max_size;
			int pos_data_type = -1; /* need to set data type */
			int is_video = 0;
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
			if ((pos_data_type>=0) && is_video) {
				buf[pos_data_type] = 0x08;
			}
		}
	} else {
		if (SDL_RWread((SDL_RWops *)opaque, buf, buf_size, 1)<1) {
			return -1;
		}
		size_read = buf_size;
	}

	return size_read;
}

static int64_t movie_ioseek( void *opaque, int64_t offset, int whence )
{
	int64_t new_offset;

	if (emul_cd) {
		switch(whence) {
			case RW_SEEK_SET:
			case RW_SEEK_END:
				emul_cd_pos = (int) offset;
				break;
			case RW_SEEK_CUR:
				emul_cd_pos += (int) offset;
				break;
		}

		offset = (emul_cd_pos * DATA_CD_SECTOR_SIZE) / RAW_CD_SECTOR_SIZE;
	}

	new_offset = SDL_RWseek((SDL_RWops *)opaque, offset, whence);

	if (emul_cd) {
		new_offset = (new_offset * RAW_CD_SECTOR_SIZE) / DATA_CD_SECTOR_SIZE;
	}

	return new_offset;
}
#endif

static void update_overlay_yuv420(void)
{
#ifdef ENABLE_MOVIES
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
#endif
}

static int movie_decode_video(SDL_Surface *screen)
{
	int retval = 0;
#ifdef ENABLE_MOVIES
	AVPacket pkt1, *pkt = &pkt1;
	int err, got_pic;

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
# ifdef USE_OLD_AVIO_API
		err = avcodec_decode_video(
			fmt_ctx->streams[vidstream]->codec,
			decoded_frame,
			&got_pic,
			pkt->data, pkt->size);
# else
		err = avcodec_decode_video2(
			fmt_ctx->streams[vidstream]->codec,
			decoded_frame,
			&got_pic,
			pkt);
# endif

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
#endif

	return retval;
}
