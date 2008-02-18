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
#include "re3_pc.h"
#include "view_background.h"
#include "view_movie.h"

#include "video.h"

/*--- Defines ---*/

#define KEY_GAMMA_DOWN		SDLK_a
#define KEY_GAMMA_UP		SDLK_q
#define KEY_GAMMA_RESET		SDLK_w

/*--- Global variables ---*/

video_t video;

/*--- Variables ---*/

static int update_screen = 1;
static int width = 320, height = 240;
static int new_width, new_height;
static int switch_mode = 0;
static video_surface_t *cur_surf = NULL;

/*--- Functions prototypes ---*/

int viewer_loop(void);
void viewer_draw(void);

/*--- Functions ---*/

int main(int argc, char **argv)
{
	if (!CheckParm(argc,argv)) {
		DisplayUsage();
		exit(1);
	}

	if (!FS_Init(strlen(argv[0])>0 ? argv[0] : "./" PACKAGE_NAME)) {
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
		case GAME_RE2_PC_DEMO_P:
		case GAME_RE2_PC_DEMO_U:
			printf("Resident Evil 2, PC, Demo\n");
			re2pcdemo_init(&game_state);
			if (viewmode == VIEWMODE_MOVIE) {
				printf("No movies to play\n");
				viewmode = VIEWMODE_BACKGROUND;
			}
			break;
		case GAME_RE2_PC_GAME_LEON:
			printf("Resident Evil 2, PC, Game Leon\n");
			re2pcgame_init(&game_state);
			break;
		case GAME_RE2_PC_GAME_CLAIRE:
			printf("Resident Evil 2, PC, Game Claire\n");
			re2pcgame_init(&game_state);
			break;
		case GAME_RE3_PC_DEMO:
			printf("Resident Evil 3, PC, Demo\n");
			re3pc_init(&game_state);
			break;
		case GAME_RE3_PC_GAME:
			printf("Resident Evil 3, PC, Game\n");
			re3pc_init(&game_state);
			break;
		default:
			printf("No known version\n");
			FS_Shutdown();
			exit(1);
	}
	if (viewmode == VIEWMODE_MOVIE) {	
#ifdef ENABLE_MOVIES
		state_newmovie();
		use_opengl = 0;
#else
		printf("Movie player disabled\n");
		viewmode == VIEWMODE_BACKGROUND;
#endif
	}

	if (SDL_Init(SDL_INIT_VIDEO)<0) {
		fprintf(stderr, "Can not initialize SDL: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}
	atexit(SDL_Quit);

	/* Try to load OpenGL library first */
	if (use_opengl) {
		use_opengl = video_opengl_loadlib();
	}
	/* Then initialize proper backend */
	if (use_opengl) {
		video_opengl_init(&video);
	} else {
		video_soft_init(&video);
	}
	printf("Calculated aspect ratio %d:%d\n", aspect_x, aspect_y);

	video.setVideoMode(&video, video.width, video.height, video.bpp);
	if (!video.screen) {
		fprintf(stderr, "Unable to create screen: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}

	SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE_NAME); 
	SDL_SetGamma(gamma, gamma, gamma);

	if (model_emd_load("pl0/emd0/em050.emd")) {
		printf("Loaded emd model\n");
	}

	int quit = 0;
	while (!quit) {
		quit = viewer_loop();
		viewer_draw();
		SDL_Delay(1);
	}

	switch(viewmode) {
		case VIEWMODE_BACKGROUND:
			state_unloadbackground();
			break;
		case VIEWMODE_MOVIE:
			movie_shutdown();
			break;
	}

	model_emd_close();

	state_shutdown();
	FS_Shutdown();

	return 0;
}

int viewer_loop(void)
{
	int quit = 0;
	SDL_Event event;

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
						video.screenShot(&video);
						break;
					case SDLK_RETURN:
						if (event.key.keysym.mod & KMOD_ALT) {
							video.flags ^= SDL_FULLSCREEN;
							switch_mode=1;
						}
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
					default:
						switch(viewmode) {
							case VIEWMODE_BACKGROUND:
								update_screen |= view_background_input(&event);
								break;
							case VIEWMODE_MOVIE:
								update_screen |= view_movie_input(&event);
								break;
						}
						break;
				}
				break;
			case SDL_VIDEORESIZE:
				new_width = event.resize.w;
				new_height = event.resize.h;
				switch_mode = 1;
				break;
			default:
				switch(viewmode) {
					case VIEWMODE_BACKGROUND:
						update_screen |= view_background_input(&event);
						break;
					case VIEWMODE_MOVIE:
						update_screen |= view_movie_input(&event);
						break;
				}
				break;
		}
	}

	if (update_screen) {
		/* Update ? */
		update_screen = 0;
		video.refreshScreen(&video);

		switch(viewmode) {
			case VIEWMODE_BACKGROUND:
				cur_surf = view_background_update();
				break;
			case VIEWMODE_MOVIE:
				update_screen = view_movie_update(video.screen);
				update_screen = 1;
				break;
		}
	}

	return(quit);
}

void viewer_draw(void)
{
	video.initScreen(&video);
	if (cur_surf) {
		video.drawBackground(&video, cur_surf);
	}
	model_emd_draw(&video);
	video.swapBuffers(&video);

	if (switch_mode) {
		video.setVideoMode(&video, new_width, new_height, video.bpp);
		update_screen=1;
		switch_mode = 0;
		new_width = video.width;
		new_height = video.height;
	}
}
