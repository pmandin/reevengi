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

#ifndef VIEW_MOVIE_H
#define VIEW_MOVIE_H 1

/*--- Structures ---*/

typedef struct view_movie_s view_movie_t;

struct view_movie_s {
	void *vCodecCtx;	/* AVCodecContext * */
	void *img_convert_ctx;	/* struct SwsContext * */
	void *decoded_frame; /* AVFrame * */

	void (*movie_refresh)(SDL_Surface *screen);
	void (*movie_stop)(void);
	void (*movie_scale_frame)(void);
	void (*movie_update_frame)(SDL_Rect *rect);
};

/*--- Variables ---*/

extern view_movie_t view_movie;

/*--- Function prototypes ---*/

int view_movie_input(SDL_Event *event);
int view_movie_update(SDL_Surface *screen);

void movie_init(void);
void movie_shutdown(void);
void movie_refresh(SDL_Surface *screen);

#endif /* VIEW_MOVIE_H */
