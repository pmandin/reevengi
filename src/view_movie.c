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

#include "state.h"

/*--- Defines ---*/

#define KEY_MOVIE_DOWN		SDLK_z
#define KEY_MOVIE_UP		SDLK_s
#define KEY_MOVIE_RESET		SDLK_x

/*--- Variables ---*/

static int restart_movie = 1;

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
	if (restart_movie) {
		printf("Playing movie %d: %s\n", game_state.num_movie, game_state.cur_movie);
		restart_movie = 0;
	}

	return NULL;
}
