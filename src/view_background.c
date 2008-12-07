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

#include "state.h"
#include "video.h"
#include "render.h"

#include "math.h"

/*--- Defines ---*/

#define KEY_STAGE_DOWN		SDLK_z
#define KEY_STAGE_UP		SDLK_s
#define KEY_STAGE_RESET		SDLK_x

#define KEY_ROOM_DOWN		SDLK_e
#define KEY_ROOM_UP		SDLK_d
#define KEY_ROOM_RESET		SDLK_c

#define KEY_CAMERA_DOWN		SDLK_r
#define KEY_CAMERA_UP		SDLK_f
#define KEY_CAMERA_RESET	SDLK_v

#define KEY_TOGGLE_GRID		SDLK_g

#define KEY_MOVE_FORWARD	SDLK_UP
#define KEY_MOVE_BACKWARD	SDLK_DOWN
#define KEY_TURN_LEFT		SDLK_LEFT
#define KEY_TURN_RIGHT		SDLK_RIGHT

/*--- Variables ---*/

static int reload_bg = 1;
static int reload_room = 1;
static int refresh_bg = 1;

static int render_grid = 0;

static int refresh_player_pos = 0;
static float player_x = 0, player_y = 0, player_z = 0;
static float player_a = 0;

static int player_moveforward = 0;
static int player_movebackward = 0;
static int player_turnleft = 0;
static int player_turnright = 0;
static float playerstart_x = 0, playerstart_z = 0, playerstart_a = 0;
static Uint32 tick_moveforward = 0;
static Uint32 tick_movebackward = 0;
static Uint32 tick_turnleft = 0;
static Uint32 tick_turnright = 0;

/*--- Functions prototypes ---*/

static void drawOrigin(void);
static void drawGrid(void);
static void drawCameraSwitches(void);

static void drawPlayer(void);

/*--- Functions ---*/

void view_background_input(SDL_Event *event)
{
	switch(event->type) {
	case SDL_KEYDOWN:
		switch (event->key.keysym.sym) {
			case KEY_STAGE_DOWN:
				game_state.stage -= 1;
				if (game_state.stage < 1) {
					game_state.stage = 7;
				}
				reload_room = 1;
				break;						
			case KEY_STAGE_UP:
				game_state.stage += 1;
				if (game_state.stage > 7) {
					game_state.stage = 1;
				}
				reload_room = 1;
				break;						
			case KEY_STAGE_RESET:
				game_state.stage = 1;
				reload_room = 1;
				break;						
			case KEY_ROOM_DOWN:
				game_state.room -= 1;
				if (game_state.room < 0) {
					game_state.room = 0x1c;
				}
				reload_room = 1;
				break;						
			case KEY_ROOM_UP:
				game_state.room += 1;
				if (game_state.room > 0x1c) {
					game_state.room = 0;
				}
				reload_room = 1;
				break;						
			case KEY_ROOM_RESET:
				game_state.room = 0;
				reload_room = 1;
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
			case KEY_TOGGLE_GRID:
				render_grid ^= 1;
				break;
			case KEY_MOVE_FORWARD:
				player_moveforward = 1;
				tick_moveforward = SDL_GetTicks();
				playerstart_x = player_x;
				playerstart_z = player_z;
				break;
			case KEY_MOVE_BACKWARD:
				player_movebackward = 1;
				tick_movebackward = SDL_GetTicks();
				playerstart_x = player_x;
				playerstart_z = player_z;
				break;
			case KEY_TURN_LEFT:
				player_turnleft = 1;
				tick_turnleft = SDL_GetTicks();
				playerstart_a = player_a;
				break;
			case KEY_TURN_RIGHT:
				player_turnright = 1;
				tick_turnright = SDL_GetTicks();
				playerstart_a = player_a;
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
			case KEY_TURN_LEFT:
				player_turnleft = 0;
				break;
			case KEY_TURN_RIGHT:
				player_turnright = 0;
				break;
		}
		break;
	}
}

void view_background_refresh(void)
{
	refresh_bg = 1;
}

void view_background_update(void)
{
	Uint32 tick_current = SDL_GetTicks();

	if (reload_room) {
		state_loadroom();
		reload_room = 0;
		reload_bg = 1;
	}
	if (reload_bg) {
		state_loadbackground();
		reload_bg = 0;
		refresh_bg = 1;
	}
	if (refresh_bg) {
		render.initBackground(&video, game_state.back_surf);
		refresh_bg = 0;
		refresh_player_pos = 1;
	}

	/* Move player ? */
	if (player_moveforward) {
		player_x = playerstart_x + cos((player_a*M_PI)/180)*5.0*(tick_current-tick_moveforward);
		player_z = playerstart_z - sin((player_a*M_PI)/180)*5.0*(tick_current-tick_moveforward);
	}
	if (player_movebackward) {
		player_x = playerstart_x - cos((player_a*M_PI)/180)*5.0*(tick_current-tick_movebackward);
		player_z = playerstart_z + sin((player_a*M_PI)/180)*5.0*(tick_current-tick_movebackward);
	}
	if (player_turnleft) {
		player_a = playerstart_a - 0.1*(tick_current-tick_turnleft);
	}
	if (player_turnright) {
		player_a = playerstart_a + 0.1*(tick_current-tick_turnright);
	}
}

void view_background_draw(void)
{
	long cam_pos[6];

	render.drawBackground(&video);

	if (!game_state.room_file) {
		return;
	}

	switch(game_state.version) {
		case GAME_RE1_PS1_DEMO:
		case GAME_RE1_PS1_GAME:
			re1ps1_get_camera(cam_pos);
			break;
		case GAME_RE1_PC_GAME:
			re1pcgame_get_camera(cam_pos);
			break;
		case GAME_RE2_PS1_DEMO:
		case GAME_RE2_PS1_GAME_LEON:
		case GAME_RE2_PS1_GAME_CLAIRE:
			re2ps1_get_camera(cam_pos);
			break;
		case GAME_RE2_PC_DEMO_P:
		case GAME_RE2_PC_DEMO_U:
			re2pcdemo_get_camera(cam_pos);
			break;
		case GAME_RE3_PS1_GAME:
			re3ps1game_get_camera(cam_pos);
			break;
		case GAME_RE3_PC_DEMO:
		case GAME_RE3_PC_GAME:
			re3pc_get_camera(cam_pos);
			break;
		default:
			return;
	}

	if (refresh_player_pos) {
		player_x = cam_pos[3];
		player_y = cam_pos[4];
		player_z = cam_pos[5];
		refresh_player_pos = 0;
	}

	render.set_projection(60.0, 4.0/3.0, 1.0, 100000.0);
	render.set_modelview(
		cam_pos[0], cam_pos[1], cam_pos[2],
		cam_pos[3], cam_pos[4], cam_pos[5],
		0.0, -1.0, 0.0
	);

	drawPlayer();

	/* World origin */
	drawOrigin();
	drawCameraSwitches();

	render.translate(cam_pos[3], cam_pos[4], cam_pos[5]);
	if (render_grid) {
		drawGrid();
	}
	drawOrigin();	/* what the camera looks at */
}

static void drawOrigin(void)
{
	Sint16 v1[3],v2[3];

	render.push_matrix();

	v1[0] = v1[1] = v1[2] = 0;
	v2[0] = 3000; v2[1] = v2[2] = 0;
	render.set_color(0x00ff0000);
	render.line(v1,v2);

	v2[1] = 3000; v2[0] = v2[2] = 0;
	render.set_color(0x0000ff00);
	render.line(v1,v2);

	v2[2] = 3000; v2[0] = v2[1] = 0;
	render.set_color(0x000000ff);
	render.line(v1,v2);

	render.pop_matrix();
}

static void drawGrid(void)
{
	int i;
	Sint16 v1[3],v2[3];

	render.set_color(0x00ffffff);

	render.push_matrix();
	render.scale(500.0,500.0,500.0);
	for (i=-40; i<=40; i+=10) {
		v1[0] = -400;
		v1[1] = 200;
		v1[2] = i;
		v2[0] = 400;
		v2[1] = 200;
		v2[2] = i;
		render.line(v1,v2);

		v1[0] = i;
		v1[1] = 200;
		v1[2] = -400;
		v2[0] = i;
		v2[1] = 200;
		v2[2] = 400;
		render.line(v1,v2);
	}
	render.pop_matrix();
}

static void drawCameraSwitches(void)
{
	int i, num_switches = 0;
	short switchPos[8];
	Sint16 v1[3],v2[3],v3[3],v4[3];

	switch(game_state.version) {
		case GAME_RE2_PC_DEMO_P:
		case GAME_RE2_PC_DEMO_U:
			num_switches = re2pcdemo_get_num_camswitch();
			break;
		default:
			return;
	}

	render.set_color(0x00cc8844);
	render.push_matrix();
	render.scale(1.0, 100.0, 1.0);
	for (i=0; i<num_switches; i++) {
		if (!re2pcdemo_get_camswitch(i,switchPos)) {
			continue;
		}

		v1[0] = switchPos[0];
		v1[1] = 2000;
		v1[2] = switchPos[1];

		v2[0] = switchPos[2];
		v2[1] = 2000;
		v2[2] = switchPos[3];
		
		v3[0] = switchPos[4];
		v3[1] = 2000;
		v3[2] = switchPos[5];

		v3[0] = switchPos[6];
		v3[1] = 2000;
		v3[2] = switchPos[7];

		render.quad(v1,v2,v3,v4);
	}
	render.pop_matrix();
}

static void drawPlayer(void)
{
	render.set_color(0x004488cc);

	render.push_matrix();
	render.translate(player_x, player_y+2000, player_z);
	render.rotate(player_a, 0.0,1.0,0.0);

#if 0
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
#else
	model_emd_draw(&video);
#endif
	render.pop_matrix();
}

