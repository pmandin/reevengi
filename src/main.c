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
#include "re1_ps1.h"
#include "re1_pc_game.h"
#include "re2_ps1.h"
#include "re2_pc_demo.h"
#include "re3_ps1_game.h"
#include "re3_pc_demo.h"

/*--- Defines ---*/

#define KEY_STAGE_DOWN		SDLK_a
#define KEY_STAGE_UP		SDLK_q
#define KEY_STAGE_RESET		SDLK_w
#define KEY_ROOM_DOWN		SDLK_z
#define KEY_ROOM_UP		SDLK_s
#define KEY_ROOM_RESET		SDLK_x
#define KEY_CAMERA_DOWN		SDLK_e
#define KEY_CAMERA_UP		SDLK_d
#define KEY_CAMERA_RESET	SDLK_c
#define KEY_GAMMA_DOWN		SDLK_r
#define KEY_GAMMA_UP		SDLK_f
#define KEY_GAMMA_RESET		SDLK_v

/*--- Variables ---*/

int num_screenshot = 0;

/*--- Functions prototypes ---*/

void screenshot(SDL_Surface *screen);

/*--- Functions ---*/

int main(int argc, char **argv)
{
	int quit=0;
	int reload_bg = 1, redraw_bg = 0;
	int switch_fs = 0, switch_mode=0;
	int width = 320, height = 240;
	int videoflags = 0;
	SDL_Surface *screen, *texture;
	SDL_Surface *bg;

	if (!CheckParm(argc,argv)) {
		DisplayUsage();
		exit(1);
	}

	if (!FS_Init(argv[0])) {
		exit(1);
	}

	FS_AddArchive(basedir);

	state_init();
	printf("Game version: ");
	switch(game_state.version) {
		case GAME_RE1_PS1_DEMO:
			printf("Resident Evil, PS1, Demo\n");
			re1ps1_init(&game_state);
			break;
		case GAME_RE1_PS1_GAME:
			printf("Resident Evil, PS1, Game\n");
			re1ps1_init(&game_state);
			break;
		case GAME_RE2_PS1_DEMO:
			printf("Resident Evil 2, PS1, Demo\n");
			re2ps1_init(&game_state);
			break;
		case GAME_RE2_PS1_GAME_LEON:
			printf("Resident Evil 2, PS1, Game Leon\n");
			re2ps1_init(&game_state);
			break;
		case GAME_RE2_PS1_GAME_CLAIRE:
			printf("Resident Evil 2, PS1, Game Claire\n");
			re2ps1_init(&game_state);
			break;
		case GAME_RE3_PS1_GAME:
			printf("Resident Evil 3, PS1, Game\n");
			re3ps1game_init(&game_state);
			break;
		case GAME_RE1_PC_GAME:
			printf("Resident Evil, PC, Game\n");
			re1pcgame_init(&game_state);
			break;
		case GAME_RE2_PC_DEMO:
			printf("Resident Evil 2, PC, Demo\n");
			re2pcdemo_init(&game_state);
			break;
		case GAME_RE2_PC_GAME_LEON:
			printf("Resident Evil 2, PC, Game Leon\n");
			FS_Shutdown();
			exit(1);
			break;
		case GAME_RE2_PC_GAME_CLAIRE:
			printf("Resident Evil 2, PC, Game Claire\n");
			FS_Shutdown();
			exit(1);
			break;
		case GAME_RE3_PC_DEMO:
			printf("Resident Evil 3, PC, Demo\n");
			re3pcdemo_init(&game_state);
#if 0
			{
				SDL_RWops *src;
				
				/*src = FS_makeRWops("data_a/pld/pl006.tim");*/
				/*src = FS_makeRWops("data_a/pld/pl000.tim");*/
				/*src = FS_makeRWops("data/etc/capcom.tim");*/
				src = FS_makeRWops("data/etc/eidos.tim");
				if (src) {
					texture = background_tim_load(src);
					if (texture) {
						printf("Loaded %dx%d image\n", texture->w, texture->h);
						SDL_SaveBMP(texture, "coincoin.bmp");
					} else {
						fprintf(stderr, "Can not load texture from tim\n");
					}
				
					SDL_RWclose(src);
				} else {
					fprintf(stderr, "Can not load texture\n");
				}
			}
#endif
			break;
		default:
			printf("No known version\n");
			FS_Shutdown();
			exit(1);
	}

	if (SDL_Init(SDL_INIT_VIDEO)<0) {
		fprintf(stderr, "Can not initialize SDL: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}
	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(width, height, 16, videoflags);
	if (!screen) {
		fprintf(stderr, "Unable to create screen: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}
	videoflags = screen->flags;
	SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE_NAME); 
	SDL_SetGamma(gamma, gamma, gamma);
	bg = NULL;

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
						case SDLK_F1:
							screenshot(screen);
							break;
						case SDLK_RETURN:
							if (event.key.keysym.mod & KMOD_ALT) {
								switch_fs=1;
							}
							break;
						case KEY_STAGE_DOWN:
							game_state.stage -= 1;
							if (game_state.stage < 1) {
								game_state.stage = 7;
							}
							reload_bg = 1;
							break;						
						case KEY_STAGE_UP:
							game_state.stage += 1;
							if (game_state.stage > 7) {
								game_state.stage = 1;
							}
							reload_bg = 1;
							break;						
						case KEY_STAGE_RESET:
							game_state.stage = 1;
							reload_bg = 1;
							break;						
						case KEY_ROOM_DOWN:
							game_state.room -= 1;
							if (game_state.room < 0) {
								game_state.room = 0x1c;
							}
							reload_bg = 1;
							break;						
						case KEY_ROOM_UP:
							game_state.room += 1;
							if (game_state.room > 0x1c) {
								game_state.room = 0;
							}
							reload_bg = 1;
							break;						
						case KEY_ROOM_RESET:
							game_state.room = 0;
							reload_bg = 1;
							break;						
						case KEY_CAMERA_DOWN:
							game_state.camera -= 1;
							if ((game_state.camera<0) && (game_state.num_cameras>0)) {
								game_state.camera = game_state.num_cameras-1;
							}
							reload_bg = 1;
							break;						
						case KEY_CAMERA_UP:
							game_state.camera += 1;
							if (game_state.camera>=game_state.num_cameras) {
								game_state.camera = 0;
							}
							reload_bg = 1;
							break;						
						case KEY_CAMERA_RESET:
							game_state.camera = 0;
							reload_bg = 1;
							break;						
						case KEY_GAMMA_DOWN:
							gamma -= 0.1;
							if (gamma<0.1) {
								gamma = 0.1;
							}
							SDL_SetGamma(gamma, gamma, gamma);
							break;
						case KEY_GAMMA_UP:
							gamma += 0.1;
							if (gamma>2.0) {
								gamma = 2.0;
							}
							SDL_SetGamma(gamma, gamma, gamma);
							break;
						case KEY_GAMMA_RESET:
							gamma = 1.0;
							SDL_SetGamma(gamma, gamma, gamma);
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
			bg = game_state.background_surf;
			if (bg) {
				redraw_bg = 1;
				if ((bg->w != width) || (bg->h != height)) {
					width = bg->w;
					height = bg->h;
					switch_mode = 1;
					redraw_bg = 0;
				}
			}
		}

		if (bg && redraw_bg) {

			if (SDL_MUSTLOCK(screen)) {
				SDL_LockSurface(screen);
			}
			SDL_BlitSurface(game_state.background_surf, NULL, screen, NULL);
			if (SDL_MUSTLOCK(screen)) {
				SDL_UnlockSurface(screen);
			}
			if (screen->flags & SDL_DOUBLEBUF) {
				SDL_Flip(screen);
			} else {
				SDL_UpdateRect(screen, 0,0,0,0);
			}
		}

		if (switch_fs) {
			videoflags ^= SDL_FULLSCREEN;
			switch_fs=0;
			switch_mode=1;
		}

		if (switch_mode) {
			screen = SDL_SetVideoMode(width, height, 16, videoflags);
			videoflags = screen->flags;
			redraw_bg=1;
			switch_mode=0;
		}

		SDL_Delay(1);
	}
	
	state_unloadbackground();
	state_shutdown();
	FS_Shutdown();

	return 0;
}

void screenshot(SDL_Surface *screen)
{
	char filename[16];

	sprintf(filename, "%08d.bmp", num_screenshot++);

	printf("Screenshot %s: %s\n", filename,
		SDL_SaveBMP(screen, filename)==0 ? "done" : "failed");
}
