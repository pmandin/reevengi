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
#include <SDL_opengl.h>

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

#include "dyngl.h"
#include "video.h"

/*--- Defines ---*/

#define KEY_GAMMA_DOWN		SDLK_a
#define KEY_GAMMA_UP		SDLK_q
#define KEY_GAMMA_RESET		SDLK_w

/*--- Variables ---*/

int update_screen = 1;
int switch_fs = 0, switch_mode=0;
int width = 320, height = 240;
SDL_Surface *cur_surf = NULL;

/*--- Local variables ---*/

static GLenum textureObject = 0;
static video_t video;

/*--- Functions prototypes ---*/

int viewer_loop(void);

void draw_surface_gl(SDL_Surface *surface);

/*--- Functions ---*/

int main(int argc, char **argv)
{
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
#ifdef ENABLE_FFMPEG
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

	if (use_opengl) {
		if (dyngl_load(NULL)) {
			use_opengl = 1;
		} else {
			fprintf(stderr, "Can not load OpenGL library: using software rendering mode\n");
			use_opengl = 0;
		}
	}

	if (use_opengl) {
		video_opengl_init(&video);
	} else {
		video_soft_init(&video);
	}

	video.setVideoMode(&video, video.width, video.height, video.bpp);
	if (!video.screen) {
		fprintf(stderr, "Unable to create screen: %s\n", SDL_GetError());
		FS_Shutdown();
		exit(1);
	}

	SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE_NAME); 
	SDL_SetGamma(gamma, gamma, gamma);

	int quit = 0;
	while (!quit) {
		quit = viewer_loop();
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

	state_shutdown();
	FS_Shutdown();

	return 0;
}

int viewer_loop(void)
{
	int quit = 0;
	int switch_fs = 0;
	int new_width, new_height;
	SDL_Event event;

	new_width = video.width;
	new_height = video.height;

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
							switch_fs=1;
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

		switch(viewmode) {
			case VIEWMODE_BACKGROUND:
				cur_surf = view_background_update();
				break;
			case VIEWMODE_MOVIE:
				update_screen = view_movie_update(video.screen);
				update_screen = 1;
				break;
		}

		if (cur_surf) {
			if ((cur_surf->w > video.width) || (cur_surf->h > video.height)) {
				new_width = cur_surf->w;
				new_height = cur_surf->h;
				update_screen = 1;
			}
		}
	}

	if (cur_surf) {
		if (use_opengl) {
			gl.ClearColor(0.6,0.4,0.2,0.0);
			gl.Clear(GL_COLOR_BUFFER_BIT);

			draw_surface_gl(cur_surf);
		} else {
			SDL_BlitSurface(cur_surf, NULL, video.screen, NULL);
		}

		video.swapBuffers(&video);

		/* Do not need to redraw image next time */
		cur_surf = NULL;
	}

	if (switch_fs || (new_width!=video.width) || (new_height!=video.height)) {
		video.setVideoMode(&video, new_width, new_height, video.bpp);
		update_screen=1;
		switch_mode=0;
	}

	return(quit);
}

void draw_surface_gl(SDL_Surface *surface)
{
	static int w=0, h=0;
	long cam_pos[6];
	int i, j;
	float end[3], start[3];

	gl.Enable(GL_TEXTURE_RECTANGLE_ARB);

	if (textureObject==0) {
		gl.GenTextures(1, &textureObject);
	}
	gl.BindTexture(GL_TEXTURE_RECTANGLE_ARB, textureObject);

	if (surface) {
		GLenum pixelType = GL_UNSIGNED_INT;
		GLenum textureFormat = GL_RGBA;
		/*printf("pitch: %d, bpp: %d\n",
			surface->pitch, surface->format->BitsPerPixel);
		printf("R=0x%08x G=0x%08x B=0x%08x A=0x%08x\n",
			surface->format->Rmask, surface->format->Gmask,
			surface->format->Bmask, surface->format->Amask
			);*/
		switch (surface->format->BitsPerPixel) {
			case 15:
			case 16:
				if (surface->format->Bmask == 0x7c00) {
					pixelType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				} else {
					pixelType = GL_UNSIGNED_SHORT_5_6_5;
				}
				textureFormat = GL_RGBA;
				break;
			case 24:
				pixelType = GL_UNSIGNED_BYTE;
				textureFormat = GL_RGB;
				break;
		}
		w = surface->w;
		h = surface->h;
		gl.TexImage2D(GL_TEXTURE_RECTANGLE_ARB,0, GL_RGBA,
			surface->w, surface->h, 0,
			textureFormat, pixelType, surface->pixels
		);
	}

 	gl.TexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 	gl.TexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 	gl.TexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
 	gl.TexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);

 	gl.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	gl.Enable(GL_DITHER);

	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE, GL_ZERO);

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0, width, height, 0.0, -1.0, 1.0);

	gl.MatrixMode(GL_TEXTURE);
	gl.LoadIdentity();
	gl.Scalef(w,h,1.0);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();
	//gl.Translatef(0.375, 0.375, 0.0);
	gl.Scalef(width, height, 1.0);

	gl.Begin(GL_QUADS);
		gl.TexCoord2f(0.0, 0.0);
		gl.Vertex2f(0.0, 0.0);

		gl.TexCoord2f(1.0, 0.0);
		gl.Vertex2f(1.0, 0.0);

		gl.TexCoord2f(1.0, 1.0);
		gl.Vertex2f(1.0, 1.0);

		gl.TexCoord2f(0.0, 1.0);
		gl.Vertex2f(0.0, 1.0);
	gl.End();

	gl.Disable(GL_DITHER);
	gl.Disable(GL_BLEND);
	gl.Disable(GL_TEXTURE_RECTANGLE_ARB);

	switch(game_state.version) {
		case GAME_RE2_PC_DEMO:
			re2pcdemo_get_camera(cam_pos);
			break;
		case GAME_RE3_PC_DEMO:
		case GAME_RE3_PC_GAME:
			re3pc_get_camera(cam_pos);
			break;
		default:
			return;
	}

	/*printf("from %6.3f,%6.3f,%6.3f to %6.3f,%6.3f,%6.3f\n",
		cam_pos[0]/256.0, cam_pos[1]/256.0, cam_pos[2]/256.0,
		cam_pos[3]/256.0, cam_pos[4]/256.0, cam_pos[5]/256.0
	);*/

	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gluPerspective(60.0, 4.0/3.0, 0.1, 1000.0);

	gl.MatrixMode(GL_MODELVIEW);
	gl.LoadIdentity();

	gluLookAt(
		cam_pos[0]/256.0, cam_pos[1]/256.0, cam_pos[2]/256.0,
		cam_pos[3]/256.0, cam_pos[4]/256.0, cam_pos[5]/256.0,
		0.0, -1.0, 0.0
	);

	gl.Begin(GL_LINES);
		/* Origin */
		gl.Color3f(1.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(10.0,0.0,0.0);

		gl.Color3f(0.0,1.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,10.0,0.0);

		gl.Color3f(0.0,0.0,1.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,10.0);
	gl.End();

	/*start[0] = (cam_pos[0]+cam_pos[3])/512.0;
	start[1] = (cam_pos[1]+cam_pos[4])/512.0;
	start[2] = (cam_pos[2]+cam_pos[5])/512.0;
	end[0] = cam_pos[0]+(start[0]*2.0);
	end[1] = cam_pos[1]+(start[1]*2.0);
	end[2] = cam_pos[2]+(start[2]*2.0);*/

	/*gl.Translatef(start[0], start[1], start[2]);*/
	gl.Translatef(cam_pos[3]/256.0, cam_pos[4]/256.0, cam_pos[5]/256.0);

	gl.Begin(GL_LINES);
		/* Camera target */
		gl.Color3f(1.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(10.0,0.0,0.0);

		gl.Color3f(0.0,1.0,0.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,10.0,0.0);

		gl.Color3f(0.0,0.0,1.0);
		gl.Vertex3f(0.0,0.0,0.0);
		gl.Vertex3f(0.0,0.0,10.0);

		/* Ground */
		gl.Color3f(1.0,1.0,1.0);
		for (i=-50; i<=50; i+=10) {
			gl.Vertex3f(-50.0,20.0,i);
			gl.Vertex3f(50.0,20.0,i);
			gl.Vertex3f(i,20.0,-50);
			gl.Vertex3f(i,20.0,50);
		}
	gl.End();
}
