/*
	Background image viewer

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

/*--- Includes ---*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <SDL.h>
#include <math.h>

#include "state.h"
#include "video.h"
#include "render.h"
#include "parameters.h"
#include "room.h"
#include "re1_pc_game.h"
#include "re1_ps1.h"
#include "re2_pc_demo.h"
#include "re2_pc_game.h"
#include "re2_ps1.h"
#include "re3_pc.h"
#include "re3_ps1_game.h"
#include "clock.h"
#include "log.h"

/*--- Defines ---*/

#define KEY_RENDER_WIREFRAME	SDLK_F2
#define KEY_RENDER_FILLED	SDLK_F3
#define KEY_RENDER_GOURAUD	SDLK_F4
#define KEY_RENDER_TEXTURED	SDLK_F5
#define KEY_RENDER_DEPTH	SDLK_F6

#define KEY_STAGE_DOWN		SDLK_z
#define KEY_STAGE_UP		SDLK_s
#define KEY_STAGE_RESET		SDLK_x

#define KEY_ROOM_DOWN		SDLK_e
#define KEY_ROOM_UP		SDLK_d
#define KEY_ROOM_RESET		SDLK_c

#define KEY_CAMERA_DOWN		SDLK_r
#define KEY_CAMERA_UP		SDLK_f
#define KEY_CAMERA_RESET	SDLK_v

#define KEY_MODEL_DOWN		SDLK_t
#define KEY_MODEL_UP		SDLK_g
#define KEY_MODEL_RESET		SDLK_b

#define KEY_TOGGLE_GRID		SDLK_y
#define KEY_TOGGLE_RESTORE	SDLK_h
#define KEY_TOGGLE_MAP		SDLK_n
#define KEY_TOGGLE_BONES	SDLK_j
#define KEY_TOGGLE_MASKS	SDLK_i

#define KEY_FORCE_REFRESH	SDLK_SPACE

#define KEY_MOVE_FORWARD	SDLK_UP
#define KEY_MOVE_BACKWARD	SDLK_DOWN
#define KEY_TURN_LEFT		SDLK_LEFT
#define KEY_TURN_RIGHT		SDLK_RIGHT
#ifdef __MINT__
#define KEY_MOVE_UP		SDLK_INSERT
#define KEY_MOVE_DOWN		SDLK_HOME
#else
#define KEY_MOVE_UP		SDLK_PAGEUP
#define KEY_MOVE_DOWN		SDLK_PAGEDOWN
#endif
#define KEY_ENTER_DOOR		SDLK_u

/*#define DISABLE_CAM_SWITCH	1*/

/*--- Variables ---*/

static int reload_bg = 1;
static int reload_room = 1;
static int refresh_bg = 1;
static int reload_model = 1;

static render_skel_t *player_model = NULL;
static int render_model = RENDER_WIREFRAME;
static int prev_render_model = -1;

static int render_grid = 0;
static int render_restore = 0;
static int render_map = 0;
static int render_bones = 0;
static int render_masks = 1;
static int render_depth = 0;

static int refresh_player_pos = 1;
static int player_moveforward = 0;
static int player_movebackward = 0;
static int player_moveup = 0;
static int player_movedown = 0;
static int player_turnleft = 0;
static int player_turnright = 0;
static float playerstart_x = 0, playerstart_y = 0, playerstart_z = 0, playerstart_a = 0;
static Uint32 tick_movement = 0;

/*--- Functions prototypes ---*/

static void processPlayerMovement(void);
static void processEnterDoor(void);

static void drawOrigin(void);
static void drawGrid(void);

static void drawPlayer(void);

/*--- Functions ---*/

void view_background_init(void)
{
#ifdef ENABLE_OPENGL
	if (params.use_opengl) {
		render_model = RENDER_TEXTURED;
	}
#endif
}

void view_background_input(SDL_Event *event)
{
	int start_movement = 0;

	switch(event->type) {
	case SDL_KEYDOWN:
		switch (event->key.keysym.sym) {
			case KEY_STAGE_DOWN:
				--game_state.num_stage;
				if (game_state.num_stage < 1) {
					game_state.num_stage = 7;
				}
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_STAGE_UP:
				++game_state.num_stage;
				if (game_state.num_stage > 7) {
					game_state.num_stage = 1;
				}
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_STAGE_RESET:
				game_state.num_stage = 1;
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_ROOM_DOWN:
				--game_state.num_room;
				if (game_state.num_room < 0) {
					game_state.num_room = 0x1c;
				}
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_ROOM_UP:
				++game_state.num_room;
				if (game_state.num_room > 0x1c) {
					game_state.num_room = 0;
				}
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_ROOM_RESET:
				game_state.num_room = 0;
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_CAMERA_DOWN:
				if (!game_state.room) {
					game_state.num_camera = 0;
				} else {
					--game_state.num_camera;
					if (game_state.num_camera<0) {
						if (game_state.room->num_cameras>0) {
							game_state.num_camera = game_state.room->num_cameras-1;
						} else {
							game_state.num_camera = 0;
						}
					}
				}
				reload_bg = 1;
				break;			
			case KEY_CAMERA_UP:
				if (!game_state.room) {
					game_state.num_camera = 0;
				} else {
					++game_state.num_camera;
					if (game_state.num_camera>=game_state.room->num_cameras) {
						game_state.num_camera = 0;
					}
				}
				reload_bg = 1;
				break;						
			case KEY_CAMERA_RESET:
				game_state.num_camera = 0;
				reload_bg = 1;
				break;						
			case KEY_MODEL_DOWN:
				--game_state.num_model;
				if (game_state.num_model<0) {
					game_state.num_model=0;
				} else {
					reload_model = 1;
				}
				break;
			case KEY_MODEL_UP:
				++game_state.num_model;
				if (game_state.num_model>100) {
					game_state.num_model=100;
				} else {
					reload_model = 1;
				}
				break;
			case KEY_MODEL_RESET:
				game_state.num_model = 0;
				reload_model = 1;
				break;
			case KEY_TOGGLE_GRID:
				render_grid ^= 1;
				break;
			case KEY_TOGGLE_RESTORE:
				render_restore ^= 1;
				break;
			case KEY_TOGGLE_MAP:
				render_map ^= 1;
				break;
			case KEY_TOGGLE_MASKS:
				render_masks ^= 1;
				break;
			case KEY_TOGGLE_BONES:
				render_bones ^= 1;
				break;
			case KEY_FORCE_REFRESH:
				refresh_bg = 1;
				refresh_player_pos = 1;
				break;
			case KEY_MOVE_FORWARD:
				player_moveforward = 1;
				start_movement = 1;
				break;
			case KEY_MOVE_BACKWARD:
				player_movebackward = 1;
				start_movement = 1;
				break;
			case KEY_MOVE_UP:
				player_moveup = 1;
				start_movement = 1;
				break;
			case KEY_MOVE_DOWN:
				player_movedown = 1;
				start_movement = 1;
				break;
			case KEY_TURN_LEFT:
				player_turnleft = 1;
				start_movement = 1;
				break;
			case KEY_TURN_RIGHT:
				player_turnright = 1;
				start_movement = 1;
				break;
			case KEY_ENTER_DOOR:
				processEnterDoor();
				break;
			case KEY_RENDER_WIREFRAME:
				render_model = RENDER_WIREFRAME;
				break;
			case KEY_RENDER_FILLED:
				render_model = RENDER_FILLED;
				break;
			case KEY_RENDER_GOURAUD:
				render_model = RENDER_GOURAUD;
				break;
			case KEY_RENDER_TEXTURED:
				render_model = RENDER_TEXTURED;
				break;
			case KEY_RENDER_DEPTH:
				render_depth ^= 1;
				render.setRenderDepth(&render, render_depth);
				break;
			default:
				break;
		}
		break;
	case SDL_KEYUP:
		switch (event->key.keysym.sym) {
			case KEY_MOVE_FORWARD:
				player_moveforward = 0;
				break;
			case KEY_MOVE_BACKWARD:
				player_movebackward = 0;
				break;
			case KEY_MOVE_UP:
				player_moveup = 0;
				break;
			case KEY_MOVE_DOWN:
				player_movedown = 0;
				break;
			case KEY_TURN_LEFT:
				player_turnleft = 0;
				break;
			case KEY_TURN_RIGHT:
				player_turnright = 0;
				break;
			default:
				break;
		}
		break;
	}

	if (start_movement) {
		tick_movement = clockGet();
		playerstart_x = game_state.player_x;
		playerstart_y = game_state.player_y;
		playerstart_z = game_state.player_z;
		playerstart_a = game_state.player_a;
	}
}

void view_background_refresh(void)
{
	refresh_bg = 1;
}

void view_background_update(void)
{
	processPlayerMovement();

	/* Pause game clock while loading stuff */
	if (reload_room | reload_bg | refresh_bg | reload_model) {
		clockPause();

		if (reload_room) {
			game_state.load_room();
			reload_room = 0;
			reload_bg = 1;
			/*refresh_player_pos = 1;*/
		}
		if (reload_bg) {
			game_state.load_background();
			reload_bg = 0;
			refresh_bg = 1;
		}
		if (refresh_bg) {
			video.dirty_rects[0]->setDirty(video.dirty_rects[0], 0,0, video.width, video.height);
			video.dirty_rects[1]->setDirty(video.dirty_rects[1], 0,0, video.width, video.height);

			refresh_bg = 0;
		}
		if (reload_model) {
			player_model = game_state.load_model(game_state.num_model);
			reload_model = 0;
		}

		clockUnpause();

		logMsg(3, "clock: last paused duration: %.3f\n", clockPausedTime()/1000.0f);
	}
}

static void processPlayerMovement(void)
{
	float new_x, new_z;
	int new_camera, was_inside, is_inside;
	Uint32 tick_current = clockGet();

	was_inside = (room_checkBoundary(game_state.room, game_state.num_camera, game_state.player_x, game_state.player_z) == 0);

	if (player_moveforward) {
		new_x = playerstart_x + cos((game_state.player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		new_z = playerstart_z - sin((game_state.player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		is_inside = (room_checkBoundary(game_state.room, game_state.num_camera, new_x, new_z) == 0);
#ifdef DISABLE_CAM_SWITCH
		is_inside = was_inside;
#endif
		if (was_inside && !is_inside) {
			/* Player can not go out */
		} else {
			game_state.player_x = new_x;
			game_state.player_z = new_z;
		}
		new_camera = room_checkCamswitch(game_state.room, game_state.num_camera, game_state.player_x, game_state.player_z);
#ifdef DISABLE_CAM_SWITCH
		new_camera = -1;
#endif
		if (new_camera != -1) {
			game_state.num_camera = new_camera;
			reload_bg = 1;
		}
	}
	if (player_movebackward) {
		new_x = playerstart_x - cos((game_state.player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		new_z = playerstart_z + sin((game_state.player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		is_inside = (room_checkBoundary(game_state.room, game_state.num_camera, new_x, new_z) == 0);
#ifdef DISABLE_CAM_SWITCH
		is_inside = was_inside;
#endif
		if (was_inside && !is_inside) {
			/* Player can not go out */
		} else {
			game_state.player_x = new_x;
			game_state.player_z = new_z;
		}
		new_camera = room_checkCamswitch(game_state.room, game_state.num_camera, game_state.player_x, game_state.player_z);
#ifdef DISABLE_CAM_SWITCH
		new_camera = -1;
#endif
		if (new_camera != -1) {
			game_state.num_camera = new_camera;
			reload_bg = 1;
		}
	}
	if (player_moveup) {
		game_state.player_y = playerstart_y - 5.0f*(tick_current-tick_movement);
	}
	if (player_movedown) {
		game_state.player_y = playerstart_y + 5.0f*(tick_current-tick_movement);
	}
	if (player_turnleft) {
		game_state.player_a = playerstart_a - 0.1f*(tick_current-tick_movement);
		while (game_state.player_a < 0.0f) {
			game_state.player_a += 360.0f;
		}
	}
	if (player_turnright) {
		game_state.player_a = playerstart_a + 0.1f*(tick_current-tick_movement);
		while (game_state.player_a > 360.0f) {
			game_state.player_a -= 360.0f;
		}
	}
}

static void processEnterDoor(void)
{
	room_door_t *door;

	if (!game_state.room) {
		return;
	}
	if (!game_state.room->enterDoor) {
		return;
	}

	door = game_state.room->enterDoor(game_state.room, game_state.player_x, game_state.player_z);
	if (door) {
		game_state.player_x = door->next_x;
		game_state.player_y = door->next_y;
		game_state.player_z = door->next_z;
		game_state.player_a = (door->next_dir * 360.0f) / 4096.0f;

		game_state.num_stage = door->next_stage;
		game_state.num_room = door->next_room;
		game_state.num_camera = door->next_camera;

		reload_room = 1;
	}
}

void view_background_draw(void)
{
	room_camera_t room_camera;

	render.startFrame(&render);

	if (render_restore && !params.use_opengl) {
		SDL_FillRect(video.screen, NULL, 0);
		video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
			0,0, video.width, video.height);
	}

	/* Draw background, dithered if needed */
	if (game_state.background) {
		render.set_dithering(params.dithering);
		render.set_useDirtyRects(1);	/* restore background only on dirtied zones */
		render.set_texture(0, game_state.background);

		render.bitmap.clipSource(0,0,0,0);
		render.bitmap.clipDest(
			video.viewport.x,video.viewport.y,
			video.viewport.w,video.viewport.h);
		render.bitmap.setScaler(
			game_state.background->w, game_state.background->h,
			video.viewport.w,video.viewport.h);
		render.bitmap.setDepth(0, 0.0f);
		render.bitmap.drawImage(&video);

		render.set_dithering(0);
		render.set_useDirtyRects(0);
	}

	/* Background completely restored, clear dirty rectangles list */
	video.dirty_rects[video.numfb]->clear(video.dirty_rects[video.numfb]);

	if (!game_state.room) {
		return;
	}
	if (!game_state.room->getCamera) {
		return;
	}

	if (render_masks) {
		(*game_state.room->drawMasks)(game_state.room, game_state.num_camera);
	}

	(*game_state.room->getCamera)(game_state.room, game_state.num_camera, &room_camera);

#ifndef ENABLE_DEBUG_POS
	if (refresh_player_pos) {
		game_state.player_x = room_camera.to_x;
		game_state.player_y = room_camera.to_y;
		game_state.player_z = room_camera.to_z;
		refresh_player_pos = 0;
	}
#endif

	render.set_projection(60.0f, 4.0f/3.0f, RENDER_Z_NEAR, RENDER_Z_FAR);
	render.set_modelview(
		room_camera.from_x, room_camera.from_y, room_camera.from_z,
		room_camera.to_x, room_camera.to_y, room_camera.to_z,
		0.0f, -1.0f, 0.0f
	);

	drawPlayer();

	/* Flush all 3D rendering to screen before drawing 2D stuff */
	render.flushFrame(&render);

	/* No texture for grid and map */
	render.set_texture(0, NULL);

	if (render_grid) {
		/* World origin */
		drawOrigin();

		render.translate(room_camera.to_x, room_camera.to_y, room_camera.to_z);
		drawGrid();

		drawOrigin();	/* what the camera looks at */
	}

	if (render_map) {
		room_map_draw(game_state.room);
		room_map_drawPlayer(game_state.player_x, game_state.player_z, game_state.player_a);
	}
}

static void drawOrigin(void)
{
	vertex_t v[2];

	render.push_matrix();

	v[0].x = v[0].y = v[0].z = 0;
	v[1].x = 3000; v[1].y = v[1].z = 0;

	render.set_color(0x00ff0000);
	render.line(&v[0], &v[1]);

	v[1].y = 3000; v[1].x = v[1].z = 0;
	render.set_color(0x0000ff00);
	render.line(&v[0], &v[1]);

	v[1].z = 3000; v[1].x = v[1].y = 0;
	render.set_color(0x000000ff);
	render.line(&v[0], &v[1]);

	render.pop_matrix();
}

static void drawGrid(void)
{
	int i;
	vertex_t v[2];

	render.set_color(0x00ffffff);

	render.push_matrix();
	render.scale(50.0f, 50.0f, 50.0f);
	for (i=-400; i<=400; i+=100) {
		v[0].x = -400;
		v[0].y = 200;
		v[0].z = i;

		v[1].x = 400;
		v[1].y = 200;
		v[1].z = i;

		render.line(&v[0], &v[1]);

		v[0].x = i;
		v[0].y = 200;
		v[0].z = -400;

		v[1].x = i;
		v[1].y = 200;
		v[1].z = 400;

		render.line(&v[0], &v[1]);
	}
	render.pop_matrix();
}

static void drawPlayer(void)
{
	render.set_render(&render, render_model);

	render.set_color(0x004488cc);

	render.push_matrix();
	render.translate(game_state.player_x, game_state.player_y+2000.0f, game_state.player_z);
	render.rotate(game_state.player_a, 0.0f,1.0f,0.0f);

	if (player_model) {
		if (render_model!=prev_render_model) {
			player_model->download(player_model);
			prev_render_model = render_model;
		}

		render.set_blending(1);
		player_model->draw(player_model, NULL);
		render.set_blending(0);
		if (render_bones) {
			player_model->drawBones(player_model, NULL);
		}
#if 0
	} else {
		render.set_texture(0, NULL);
		render.scale(2500.0, 2500.0, 2500.0);

		render.line(0.2,0.0,0.0, -0.2,0.0,0.0);	/* head */
		render.line(-0.2,0.0,0.0, -0.2,-0.4,0.0);
		render.line(-0.2,-0.4,0.0, 0.2,-0.4,0.0);
		render.line(0.2,-0.4,0.0, 0.2,0.0,0.0);
		render.line(0.0,0.0,0.0, 0.0,1.0,0.0);	/* body */
		render.line(0.0,0.0,0.0, 0.5,0.5,0.0);	/* right arm */
		render.line(0.0,0.0,0.0, -0.5,0.5,0.0);	/* left arm */
		render.line(0.0,1.0,0.0, 0.5,1.5,0.0);	/* right leg */
		render.line(0.0,1.0,0.0, -0.5,1.5,0.0);	/* left leg */
#endif
	}

	render.pop_matrix();

	render.set_render(&render, RENDER_TEXTURED);
}
