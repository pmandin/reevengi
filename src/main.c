/*
	Main

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

#include <stdio.h>
#include <SDL.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "state.h"
#include "parameters.h"
#include "re1_ps1_demo.h"
#include "re2_pc_demo.h"

int main(int argc, char **argv)
{
	int quit=0;
	int reload_bg = 1;
	int redraw_bg = 1;
	int switch_fs = 0;
	int videoflags;
	SDL_Surface *screen;

	if (!CheckParm(argc,argv)) {
		DisplayUsage();
		exit(1);
	}

	state_init();
	/*re1ps1demo_init(&game_state);*/
	re2pcdemo_init(&game_state);

	if (SDL_Init(SDL_INIT_VIDEO)<0) {
		fprintf(stderr, "Can not initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(320, 240, 16, 0);
	if (!screen) {
		fprintf(stderr, "Unable to create screen: %s\n", SDL_GetError());
		return 0;
	}
	videoflags = screen->flags;
	SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE_NAME); 
	SDL_SetGamma(gamma, gamma, gamma);

	while (!quit) {
		SDL_Event event;

		/* Evenements */
		while (SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					quit=1;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							quit=1;
							break;
						case SDLK_RETURN:
							if (event.key.keysym.mod & KMOD_ALT) {
								switch_fs=1;
							}
							break;
						case SDLK_a:
							game_state.stage -= 1;
							if (game_state.stage < 1) {
								game_state.stage = 2;
							}
							reload_bg = 1;
							break;						
						case SDLK_q:
							game_state.stage += 1;
							if (game_state.stage > 2) {
								game_state.stage = 1;
							}
							reload_bg = 1;
							break;						
						case SDLK_z:
							game_state.room -= 1;
							if (game_state.room < 0) {
								game_state.room = 0x1c;
							}
							reload_bg = 1;
							break;						
						case SDLK_s:
							game_state.room += 1;
							if (game_state.room > 0x1c) {
								game_state.room = 0;
							}
							reload_bg = 1;
							break;						
						case SDLK_e:
							game_state.camera -= 1;
							if ((game_state.camera<0) && (game_state.num_cameras>0)) {
								game_state.camera = game_state.num_cameras-1;
							}
							reload_bg = 1;
							break;						
						case SDLK_d:
							game_state.camera += 1;
							if (game_state.camera>=game_state.num_cameras) {
								game_state.camera = 0;
							}
							reload_bg = 1;
							break;						
					}
					break;
			}
		}

		/* Etat */
		if (reload_bg) {
			reload_bg = 0;

			/* depack background image */
			state_loadbackground();

			/* redraw */
			if (game_state.surface_bg) {
				if (SDL_MUSTLOCK(screen)) {
					SDL_LockSurface(screen);
				}
				SDL_BlitSurface(game_state.surface_bg, NULL, screen, NULL);
				if (SDL_MUSTLOCK(screen)) {
					SDL_UnlockSurface(screen);
				}
				if (screen->flags & SDL_DOUBLEBUF) {
					SDL_Flip(screen);
				} else {
					SDL_UpdateRect(screen, 0,0,0,0);
				}
			}
		}

		if (switch_fs) {
			switch_fs=0;
			videoflags ^= SDL_FULLSCREEN;
			screen = SDL_SetVideoMode(320, 240, 16, videoflags);
			videoflags = screen->flags;
			reload_bg=1;
		}

		SDL_Delay(1);
	}
	
	state_unloadbackground();

	return 0;
}
