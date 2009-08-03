/*
	Background image viewer

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

#include <stdlib.h>
#include <SDL.h>
#include <math.h>

#include "state.h"
#include "video.h"
#include "render.h"
#include "parameters.h"
#include "model.h"
#include "room.h"
#include "re1_pc_game.h"
#include "re1_ps1.h"
#include "re2_pc_demo.h"
#include "re2_pc_game.h"
#include "re2_ps1.h"
#include "re3_pc.h"
#include "re3_ps1_game.h"

/*--- Defines ---*/

#define KEY_RENDER_WIREFRAME	SDLK_F2
#define KEY_RENDER_FILLED	SDLK_F3
#define KEY_RENDER_GOURAUD	SDLK_F4
#define KEY_RENDER_TEXTURED	SDLK_F5

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

/*--- Variables ---*/

static int reload_bg = 1;
static int reload_room = 1;
static int refresh_bg = 1;
static int reload_model = 1;

static int num_model = 0;
static model_t *player_model = NULL;

static int render_grid = 0;
static int render_restore = 0;
static int render_map = 0;

static int refresh_player_pos = 0;
static float player_x = 0, player_y = 0, player_z = 0;
static float player_a = 0;
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
				break;						
			case KEY_STAGE_UP:
				++game_state.num_stage;
				if (game_state.num_stage > 7) {
					game_state.num_stage = 1;
				}
				reload_room = 1;
				break;						
			case KEY_STAGE_RESET:
				game_state.num_stage = 1;
				reload_room = 1;
				break;						
			case KEY_ROOM_DOWN:
				--game_state.num_room;
				if (game_state.num_room < 0) {
					game_state.num_room = 0x1c;
				}
				reload_room = 1;
				break;						
			case KEY_ROOM_UP:
				++game_state.num_room;
				if (game_state.num_room > 0x1c) {
					game_state.num_room = 0;
				}
				reload_room = 1;
				break;						
			case KEY_ROOM_RESET:
				game_state.num_room = 0;
				reload_room = 1;
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
				--num_model;
				if (num_model<0) {
					num_model=0;
				} else {
					reload_model = 1;
				}
				break;
			case KEY_MODEL_UP:
				++num_model;
				if (num_model>100) {
					num_model=100;
				} else {
					reload_model = 1;
				}
				break;
			case KEY_MODEL_RESET:
				num_model = 0;
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
				render.set_render(&render, RENDER_WIREFRAME);
				break;
			case KEY_RENDER_FILLED:
				render.set_render(&render, RENDER_FILLED);
				break;
			case KEY_RENDER_GOURAUD:
				render.set_render(&render, RENDER_GOURAUD);
				break;
			case KEY_RENDER_TEXTURED:
				render.set_render(&render, RENDER_TEXTURED);
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
		tick_movement = SDL_GetTicks();
		playerstart_x = player_x;
		playerstart_y = player_y;
		playerstart_z = player_z;
		playerstart_a = player_a;
	}
}

void view_background_refresh(void)
{
	refresh_bg = 1;
}

void view_background_update(void)
{
	processPlayerMovement();

	if (reload_room) {
		game_state.load_room();
		reload_room = 0;
		reload_bg = 1;
		refresh_player_pos = 1;
	}
	if (reload_bg) {
		game_state.load_background();
		reload_bg = 0;
		refresh_bg = 1;
	}
	if (refresh_bg) {
		render.initBackground(&video, game_state.back_surf);
		refresh_bg = 0;
	}
	if (reload_model) {
		render.set_texture(0, NULL);	/* To force reloading texture */
		player_model = game_state.load_model(num_model);
		reload_model = 0;
	}
}

static void processPlayerMovement(void)
{
	float new_x, new_z;
	int new_camera, was_inside, is_inside;
	Uint32 tick_current = SDL_GetTicks();

	was_inside = (room_checkBoundary(game_state.room, game_state.num_camera, player_x, player_z) == 0);

	if (player_moveforward) {
		new_x = playerstart_x + cos((player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		new_z = playerstart_z - sin((player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		is_inside = (room_checkBoundary(game_state.room, game_state.num_camera, new_x, new_z) == 0);
		if (was_inside && !is_inside) {
			/* Player can not go out */
		} else {
			player_x = new_x;
			player_z = new_z;
		}
		new_camera = room_checkCamswitch(game_state.room, game_state.num_camera, player_x, player_z);
		if (new_camera != -1) {
			game_state.num_camera = new_camera;
			reload_bg = 1;
		}
	}
	if (player_movebackward) {
		new_x = playerstart_x - cos((player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		new_z = playerstart_z + sin((player_a*M_PI)/180.0f)*5.0f*(tick_current-tick_movement);
		is_inside = (room_checkBoundary(game_state.room, game_state.num_camera, new_x, new_z) == 0);
		if (was_inside && !is_inside) {
			/* Player can not go out */
		} else {
			player_x = new_x;
			player_z = new_z;
		}
		new_camera = room_checkCamswitch(game_state.room, game_state.num_camera, player_x, player_z);
		if (new_camera != -1) {
			game_state.num_camera = new_camera;
			reload_bg = 1;
		}
	}
	if (player_moveup) {
		player_y = playerstart_y - 5.0f*(tick_current-tick_movement);
	}
	if (player_movedown) {
		player_y = playerstart_y + 5.0f*(tick_current-tick_movement);
	}
	if (player_turnleft) {
		player_a = playerstart_a - 0.1f*(tick_current-tick_movement);
	}
	if (player_turnright) {
		player_a = playerstart_a + 0.1f*(tick_current-tick_movement);
	}
}

static void processEnterDoor(void)
{
	room_doorswitch_t doorSwitch;

	if (!game_state.room) {
		return;
	}
	if (!game_state.room->enterDoor) {
		return;
	}

	doorSwitch.x = player_x;
	doorSwitch.y = player_z;
	if (game_state.room->enterDoor(game_state.room, &doorSwitch)) {
		player_x = doorSwitch.next_x;
		player_y = doorSwitch.next_y;
		player_z = doorSwitch.next_z;
		player_a = doorSwitch.next_angle;

		game_state.num_stage = doorSwitch.next_stage;
		game_state.num_room = doorSwitch.next_room;
		game_state.num_camera = doorSwitch.next_camera;

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

	render.drawBackground(&video);

	if (!game_state.room) {
		return;
	}
	if (!game_state.room->getCamera) {
		return;
	}

	(*game_state.room->getCamera)(game_state.room, game_state.num_camera, &room_camera);

	if (refresh_player_pos) {
		player_x = room_camera.to_x;
		player_y = room_camera.to_y;
		player_z = room_camera.to_z;
		refresh_player_pos = 0;
	}

	render.set_projection(60.0f, 4.0f/3.0f, 1.0f, 100000.0f);
	render.set_modelview(
		room_camera.from_x, room_camera.from_y, room_camera.from_z,
		room_camera.to_x, room_camera.to_y, room_camera.to_z,
		0.0f, -1.0f, 0.0f
	);

	drawPlayer();

	if (render_grid) {
		/* World origin */
		drawOrigin();

		render.translate(room_camera.to_x, room_camera.to_y, room_camera.to_z);
		drawGrid();

		drawOrigin();	/* what the camera looks at */
	}

	if (render_map) {
		room_map_draw(game_state.room);
		room_map_drawPlayer(player_x, player_z, player_a);
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
	render.set_color(0x004488cc);

	render.push_matrix();
	render.translate(player_x, player_y+2000.0f, player_z);
	render.rotate(player_a, 0.0f,1.0f,0.0f);

	if (player_model) {
		render.set_blending(1);
		player_model->draw(player_model);
		render.set_blending(0);
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
}
