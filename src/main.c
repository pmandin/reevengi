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
#include "re2_ps1_demo2.h"
#include "re2_pc_demo.h"
#include "re3_ps1_game.h"
#include "re3_pc.h"
#include "view_background.h"
#include "view_movie.h"

#include "video.h"
#include "render.h"

/*--- Defines ---*/

#define KEY_GAMMA_DOWN		SDLK_a
#define KEY_GAMMA_UP		SDLK_q
#define KEY_GAMMA_RESET		SDLK_w

/*--- Global variables ---*/

video_t video;
render_t render;

/*--- Variables ---*/

static int new_width, new_height;
static int switch_mode = 0;

/*--- Functions prototypes ---*/

static int viewer_loop(void);
static void viewer_update(void);

/*--- Functions ---*/

int main(int argc, char **argv)
{
	int quit;

	if (!CheckParm(argc,argv)) {
		DisplayUsage();
		exit(1);
	}

	if (!FS_Init(strlen(argv[0])>0 ? argv[0] : "./" PACKAGE_NAME)) {
		exit(1);
	}

	FS_AddArchive(params.basedir);

	state_init();
	logMsg(0, "Game version: %s\n", state_getGameName());
	switch(game_state.version) {
		case GAME_RE1_PS1_DEMO:
		case GAME_RE1_PS1_GAME:
			re1ps1_init(&game_state);
			break;
		case GAME_RE2_PS1_DEMO:
		case GAME_RE2_PS1_GAME_LEON:
		case GAME_RE2_PS1_GAME_CLAIRE:
			re2ps1_init(&game_state);
			break;
		case GAME_RE2_PS1_DEMO2:
			re2ps1demo2_init(&game_state);
			break;
		case GAME_RE3_PS1_GAME:
			re3ps1game_init(&game_state);
			break;
		case GAME_RE1_PC_GAME:
			re1pcgame_init(&game_state);
			break;
		case GAME_RE2_PC_DEMO_P:
		case GAME_RE2_PC_DEMO_U:
			re2pcdemo_init(&game_state);
			if (params.viewmode == VIEWMODE_MOVIE) {
				logMsg(1, "No movies to play\n");
				params.viewmode = VIEWMODE_BACKGROUND;
			}
			break;
		case GAME_RE2_PC_GAME_LEON:
		case GAME_RE2_PC_GAME_CLAIRE:
			re2pcgame_init(&game_state);
			break;
		case GAME_RE3_PC_DEMO:
		case GAME_RE3_PC_GAME:
			re3pc_init(&game_state);
			break;
		default:
			FS_Shutdown();
			exit(1);
	}
	if (params.viewmode == VIEWMODE_MOVIE) {	
#ifdef ENABLE_MOVIES
		state_newmovie();
		params.use_opengl = 0;
#else
		logMsg(0,"Movie player disabled\n");
		params.viewmode == VIEWMODE_BACKGROUND;
#endif
	}

	if (SDL_Init(SDL_INIT_VIDEO)<0) {
		fprintf(stderr, "Can not initialize SDL: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}
	atexit(SDL_Quit);

	/* Try to load OpenGL library first */
	if (params.use_opengl) {
		params.use_opengl = video_opengl_loadlib();
	}
	/* Then initialize proper backend */
	if (params.use_opengl) {
		video_opengl_init(&video);
		render_opengl_init(&render);
	} else {
		video_soft_init(&video);
		render_soft_init(&render);
	}
	logMsg(1,"Calculated aspect ratio %d:%d\n", params.aspect_x, params.aspect_y);

	video.setVideoMode(&video, video.width, video.height, video.bpp);
	if (!video.screen) {
		fprintf(stderr, "Unable to create screen: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}

	SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE_NAME); 
	SDL_SetGamma(params.gamma, params.gamma, params.gamma);

	if (model_emd_load("pl0/emd0/em050.emd")) {
		logMsg(2,"Loaded emd model\n");
	}

	/* Force a mode switch */
	new_width = video.width;
	new_height = video.height;
	switch_mode=1;

	quit = 0;
	while (!quit) {
		quit = viewer_loop();
		viewer_update();
#ifndef __MINT__
		SDL_Delay(1);
#endif
	}

	switch(params.viewmode) {
		case VIEWMODE_BACKGROUND:
			state_unloadbackground();
			break;
		case VIEWMODE_MOVIE:
			movie_shutdown();
			break;
	}

	model_emd_close();

	state_shutdown();
	video.shutDown(&video);
	render.shutdown(&render);
	FS_Shutdown();

	SDL_Quit();
	return 0;
}

static int viewer_loop(void)
{
	int quit = 0;
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		logMsg(4, "Process event %d\n", event.type);
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
							new_width = video.width;
							new_height = video.height;
							switch_mode=1;
						}
						break;
					case KEY_GAMMA_DOWN:
						params.gamma -= 0.1;
						if (params.gamma<0.1) {
							params.gamma = 0.1;
						}
						SDL_SetGamma(params.gamma, params.gamma, params.gamma);
						break;
					case KEY_GAMMA_UP:
						params.gamma += 0.1;
						if (params.gamma>2.0) {
							params.gamma = 2.0;
						}
						SDL_SetGamma(params.gamma, params.gamma, params.gamma);
						break;
					case KEY_GAMMA_RESET:
						params.gamma = 1.0;
						SDL_SetGamma(params.gamma, params.gamma, params.gamma);
						break;						
					default:
						switch(params.viewmode) {
							case VIEWMODE_BACKGROUND:
								view_background_input(&event);
								break;
							case VIEWMODE_MOVIE:
								view_movie_input(&event);
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
				switch(params.viewmode) {
					case VIEWMODE_BACKGROUND:
						view_background_input(&event);
						break;
					case VIEWMODE_MOVIE:
						view_movie_input(&event);
						break;
				}
				break;
		}
	}

	return(quit);
}

static void viewer_update(void)
{
	if (switch_mode) {
		video.setVideoMode(&video, new_width, new_height, video.bpp);
	}

	switch(params.viewmode) {
		case VIEWMODE_BACKGROUND:
			if (switch_mode) {
				view_background_refresh();
			}
			view_background_update();
			view_background_draw();
			break;
		case VIEWMODE_MOVIE:
			view_movie_update(video.screen);
			break;
	}

	video.swapBuffers(&video);
	switch_mode = 0;
}
