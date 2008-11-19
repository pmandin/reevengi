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

/*--- Variables ---*/

static int reload_bg = 1;
static int reload_room = 1;
static int refresh_bg = 1;

/*--- Functions prototypes ---*/

static void drawOrigin(void);
static void drawGrid(void);
static void drawCameraSwitches(void);

/*--- Functions ---*/

void view_background_input(SDL_Event *event)
{
	if (event->type == SDL_KEYDOWN) {
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
		}
	}
}

void view_background_refresh(void)
{
	refresh_bg = 1;
}

void view_background_update(void)
{
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
	}
}

void view_background_draw(void)
{
	long cam_pos[6];

	render.drawBackground(&video);
	/*model_emd_draw(&video);*/

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

	render.set_projection(60.0, 4.0/3.0, 1.0, 100000.0);
	render.set_modelview(
		cam_pos[0], cam_pos[1], cam_pos[2],
		cam_pos[3], cam_pos[4], cam_pos[5],
		0.0, -1.0, 0.0
	);

	/* World origin */
	drawOrigin();
	/*drawCameraSwitches();*/

	render.translate(cam_pos[3], cam_pos[4], cam_pos[5]);
	/*drawGrid();*/
	drawOrigin();	/* what the camera looks at */
}

static void drawOrigin(void)
{
	render.push_matrix();
	render.scale(3000.0, 3000.0, 3000.0);

	render.set_color(0x00ff0000);
	render.line(
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0);
	render.set_color(0x0000ff00);
	render.line(
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0);
	render.set_color(0x000000ff);
	render.line(
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0);

	render.pop_matrix();
}

static void drawGrid(void)
{
	int i;

	render.set_color(0x00ffffff);

	render.push_matrix();
	render.scale(1000.0, 1000.0, 1000.0);
	for (i=-40; i<=40; i+=10) {
		render.line(
			-40.0, 20.0, i,
			40.0, 20.0, i);
		render.line(
			i, 20.0, -40.0,
			i, 20.0, 40.0);
	}
	render.pop_matrix();
}

static void drawCameraSwitches(void)
{
	int i, num_switches = 0;
	short switchPos[8];

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

		render.line(
			switchPos[0], 20, switchPos[1],
			switchPos[2], 20, switchPos[3]);
		render.line(
			switchPos[2], 20, switchPos[3],
			switchPos[4], 20, switchPos[5]);
		render.line(
			switchPos[4], 20, switchPos[5],
			switchPos[6], 20, switchPos[7]);
		render.line(
			switchPos[6], 20, switchPos[7],
			switchPos[0], 20, switchPos[1]);
	}
	render.pop_matrix();
}
