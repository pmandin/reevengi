/*
	Main

	Copyright (C) 2007-2010	Patrice Mandin

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

#include "render_texture.h"

#include "g_common/game.h"
#include "g_common/menu.h"

#include "g_re1/game_re1.h"
#include "g_re2/game_re2.h"
#include "g_re3/game_re3.h"

#include "view_background.h"
#include "view_movie.h"
#include "r_common/render_texture_list.h"
#include "r_common/render_skel_list.h"

#include "clock.h"
#include "parameters.h"
#include "filesystem.h"
#include "log.h"

#include "video.h"
#include "render.h"

/*--- Global variables ---*/

game_t *game;

video_t video;
render_t render;

/*--- Variables ---*/

static int new_width, new_height;
static int switch_mode = 0;
static int disp_menu = 0;

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

	/* Detect version */
	game = game_ctor();
	if (game->major == GAME_UNKNOWN) {
		game_re1_detect(game);
	}
	if (game->major == GAME_UNKNOWN) {
		game_re2_detect(game);
	}
	if (game->major == GAME_UNKNOWN) {
		game_re3_detect(game);
	}
	logMsg(0, "Game version: %s\n", game->name);
	if (game->major == GAME_UNKNOWN) {
		FS_Shutdown();
		exit(1);
	}

	/* Then create game specific stuff */
	switch(game->major) {
		case GAME_RE1:
			game = game_re1_ctor(game);
			break;
		case GAME_RE2:
			game = game_re2_ctor(game);
			break;
		case GAME_RE3:
			game = game_re3_ctor(game);
			break;
		default:
			break;
	}




	if (params.viewmode == VIEWMODE_MOVIE) {	
#ifdef ENABLE_MOVIES
		game->switch_movie(game);
		params.use_opengl = 0;
#else
		logMsg(0,"Movie player disabled\n");
		params.viewmode = VIEWMODE_BACKGROUND;
#endif
	}

	if (SDL_Init(SDL_INIT_VIDEO)<0) {
		fprintf(stderr, "Can not initialize SDL: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}
	atexit(SDL_Quit);
	logEnableTicks();

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

	video.setVideoMode(video.width, video.height, video.bpp);
	if (!video.screen) {
		fprintf(stderr, "Unable to create screen: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}

	SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE_NAME); 

	/* Force a mode switch */
	new_width = video.width;
	new_height = video.height;
	switch_mode=1;

	/* Init viewer */
	game->load_font(game);
	if (!game->font) {
		logMsg(0, "No font. Menu disabled.\n");
	}
	game->menu->init(game->menu, game, game->player);

	clockInit();
	switch(params.viewmode) {
		case VIEWMODE_BACKGROUND:
			view_background_init();
			break;
		case VIEWMODE_MOVIE:
			break;
	}

	/* Viewer loop */
	quit = 0;
	while (!quit) {
		quit = viewer_loop();
		viewer_update();
#ifndef __MINT__
		SDL_Delay(1);
#endif
	}

	/* Shutdown viewer */
	switch(params.viewmode) {
		case VIEWMODE_BACKGROUND:
			break;
		case VIEWMODE_MOVIE:
			movie_shutdown();
			break;
	}

	video.shutDown();
	render.shutdown();

	game->dtor(game);

	logMsg(0,"fs: shutdown\n");
	FS_Shutdown();

	SDL_Quit();
	return 0;
}

static int viewer_loop(void)
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
						/*if (!game->font) {*/
							quit=1;
						/*} else {
							disp_menu ^= 1;
						}*/
						break;
					case SDLK_F1:
						video.screenShot();
						break;
					case SDLK_F10:
						disp_menu ^= 1;
						break;
					case SDLK_RETURN:
						if (event.key.keysym.mod & KMOD_ALT) {
							video.flags ^= SDL_FULLSCREEN;
							new_width = video.width;
							new_height = video.height;
							switch_mode=1;
						}
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
		list_render_texture_download();
		list_render_skel_download();
		video.setVideoMode(new_width, new_height, video.bpp);
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

	if (disp_menu) {
		game->menu->draw(game->menu);
	}

	video.swapBuffers();
	switch_mode = 0;
}
