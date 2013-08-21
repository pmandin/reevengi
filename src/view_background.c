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

#include "parameters.h"
#include "log.h"
#include "clock.h"

#include "g_common/game.h"
#include "g_common/room.h"
#include "g_common/room_map.h"
#include "g_common/room_door.h"
#include "g_common/player.h"

#include "video.h"
#include "render.h"

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
#define KEY_TOGGLE_MAP		SDLK_TAB
#define KEY_TOGGLE_BONES	SDLK_j
#define KEY_TOGGLE_MASKS	SDLK_i

#define KEY_ANIM_DOWN		SDLK_k
#define KEY_ANIM_UP		SDLK_l
#define KEY_TOGGLE_ANIM		SDLK_m

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

/*static render_skel_t *player_model = NULL;*/
static int render_model = RENDER_WIREFRAME;
static int prev_render_model = -1;

static int render_grid = 0;
static int render_restore = 0;
static int render_bones = 0;
static int render_masks = 1;
static int render_depth = 0;
static int render_anim = 0;

static int refresh_player_pos = 1;
static int player_moveforward = 0;
static int player_movebackward = 0;
static int player_moveup = 0;
static int player_movedown = 0;
static int player_turnleft = 0;
static int player_turnright = 0;
static Uint32 tick_anim = 0;

static int shift_pressed = 0;

/*--- Functions prototypes ---*/

static void toggle_map_mode(void);

static void processPlayerMovement(void);
static void processEnterDoor(void);

static void drawPlayer(void);

/*--- Functions ---*/

void view_background_init(void)
{
#ifdef ENABLE_OPENGL
	if (params.use_opengl) {
		render_model = RENDER_TEXTURED;
	}
#endif
	tick_anim = clockGet();
}

void view_background_input(SDL_Event *event)
{
	int start_movement = 0;
	player_t *player = game->player;

	switch(event->type) {
	case SDL_KEYDOWN:
		switch (event->key.keysym.sym) {
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
				shift_pressed=1;
				break;
			case KEY_STAGE_DOWN:
				game->prev_stage(game);
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_STAGE_UP:
				game->next_stage(game);
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_STAGE_RESET:
				game->reset_stage(game);
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_ROOM_DOWN:
				game->prev_room(game);
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_ROOM_UP:
				game->next_room(game);
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_ROOM_RESET:
				game->reset_room(game);
				reload_room = 1;
				refresh_player_pos = 1;
				break;						
			case KEY_CAMERA_DOWN:
				game->prev_camera(game);
				reload_bg = 1;
				break;			
			case KEY_CAMERA_UP:
				game->next_camera(game);
				reload_bg = 1;
				break;
			case KEY_CAMERA_RESET:
				game->reset_camera(game);
				reload_bg = 1;
				break;						
			case KEY_MODEL_DOWN:
				player->prev_model(player);
				reload_model = 1;
				break;
			case KEY_MODEL_UP:
				player->next_model(player);
				reload_model = 1;
				break;
			case KEY_MODEL_RESET:
				player->reset_model(player);
				reload_model = 1;
				break;
			case KEY_TOGGLE_GRID:
				render_grid ^= 1;
				break;
			case KEY_TOGGLE_RESTORE:
				render_restore ^= 1;
				break;
			case KEY_TOGGLE_MAP:
				toggle_map_mode();
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
				render.setRenderDepth(render_depth);
				break;
			case KEY_ANIM_DOWN:
				player->prev_anim(player);
				break;
			case KEY_ANIM_UP:
				player->next_anim(player);
				break;
			case KEY_TOGGLE_ANIM:
				switch(render_anim) {
					case -1:render_anim=0;	break;
					case 0:	render_anim=1;	break;
					case 1:	render_anim=-1;	break;
				}
				tick_anim = clockGet();
				break;
			default:
				break;
		}
		break;
	case SDL_KEYUP:
		switch (event->key.keysym.sym) {
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
				shift_pressed=0;
				break;
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
		player->move_start(player);
	}
}

static void toggle_map_mode(void)
{
	room_t *room = game->room;

	if (shift_pressed) {
		room->toggleMapModePrev(room);
	} else {
		room->toggleMapModeNext(room);
	}
}

void view_background_refresh(void)
{
	refresh_bg = 1;
}

void view_background_update(void)
{
	player_t *player = game->player;

	processPlayerMovement();

	/* Pause game clock while loading stuff */
	if (reload_room | reload_bg | refresh_bg | reload_model) {
		clockPause();

		if (reload_room) {
			logMsg(1, "view_background: Load room\n");
			game->setRoom(game, game->num_stage, game->num_room);
			reload_room = 0;
			reload_bg = 1;
			/*refresh_player_pos = 1;*/
		}
		if (reload_bg) {
			logMsg(1, "view_background: Load background\n");
			if (game->room) {
				game->room->setCamera(game->room, game->num_camera);
			}
			reload_bg = 0;
			refresh_bg = 1;
		}
		if (refresh_bg) {
			video.dirty_rects[0]->setDirty(video.dirty_rects[0], 0,0, video.width, video.height);
			video.dirty_rects[1]->setDirty(video.dirty_rects[1], 0,0, video.width, video.height);

			refresh_bg = 0;
		}
		if (reload_model) {
			player->model = player->load_model(player, player->num_model);
			reload_model = 0;

			if (player->model) {
				int num_anims = player->model->getNumAnims(player->model);

				if (player->num_anim>=num_anims) {
					player->num_anim = num_anims-1;
				}
				if (player->num_anim<0) {
					player->num_anim = 0;
				}
			}
		}

		clockUnpause();

		logMsg(3, "clock: last paused duration: %.3f\n", clockPausedTime()/1000.0f);
	}
}

static void processPlayerMovement(void)
{
	float prev_x, prev_z;
	int new_camera, was_inside, is_inside;
	player_t *player = game->player;
	room_t *room = game->room;

	if (!room) {
		return;
	}

	if (player_moveforward || player_movebackward || player_moveup || player_movedown
	   /*|| player_turnright || player_turnleft*/)
	{
		logMsg(2, "player: %f,%f,%f\n",player->x,player->y,player->z);
	}

	was_inside = (room->checkBoundary(room, game->num_camera, player->x, player->z) == 0);

	if (player_moveforward) {
		prev_x = player->x;
		prev_z = player->z;

		player->move_forward(player);

		is_inside = (room->checkBoundary(room, game->num_camera, player->x, player->z) == 0);
#ifdef DISABLE_CAM_SWITCH
		is_inside = was_inside;
#endif
		if (was_inside && !is_inside) {
			/* Player can not go out */
			player->x = prev_x;
			player->z = prev_z;
		}
		new_camera = room->checkCamSwitch(room, game->num_camera, player->x, player->z);
#ifdef DISABLE_CAM_SWITCH
		new_camera = -1;
#endif
		if (new_camera != -1) {
			game->num_camera = new_camera;
			reload_bg = 1;
		}
	}
	if (player_movebackward) {
		prev_x = player->x;
		prev_z = player->z;

		player->move_backward(player);

		is_inside = (room->checkBoundary(room, game->num_camera, player->x, player->z) == 0);
#ifdef DISABLE_CAM_SWITCH
		is_inside = was_inside;
#endif
		if (was_inside && !is_inside) {
			/* Player can not go out */
			player->x = prev_x;
			player->z = prev_z;
		}
		new_camera = room->checkCamSwitch(room, game->num_camera, player->x, player->z);
#ifdef DISABLE_CAM_SWITCH
		new_camera = -1;
#endif
		if (new_camera != -1) {
			game->num_camera = new_camera;
			reload_bg = 1;
		}
	}
	if (player_moveup) {
		player->move_up(player);
	}
	if (player_movedown) {
		player->move_down(player);
	}
	if (player_turnleft) {
		player->turn_left(player);
	}
	if (player_turnright) {
		player->turn_right(player);
	}
}

static void processEnterDoor(void)
{
	player_t *player = game->player;
	room_t *room = game->room;
	room_door_t *door;

	if (!room->enterDoor) {
		return;
	}

	door = room->enterDoor(room, player->x, player->z);
	if (door) {
		player->x = door->next_x;
		player->y = door->next_y;
		player->z = door->next_z;
		player->a = door->next_dir;

		game->num_stage = door->next_stage;
		game->num_room = door->next_room;
		game->num_camera = door->next_camera;

		reload_room = 1;
	}
}

void view_background_draw(void)
{
	player_t *player = game->player;
	room_t *room = game->room;
	room_camera_t room_camera;

	render.startFrame();

	if (render_restore && !params.use_opengl) {
		SDL_FillRect(video.screen, NULL, 0);
		video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
			0,0, video.width, video.height);
	}

	/* Draw background, dithered if needed */
	if (room && room->background) {
		render_texture_t *room_bg = room->background;

		render.set_dithering(params.dithering);
		render.set_useDirtyRects(1);	/* restore background only on dirtied zones */
		render.set_texture(0, room_bg);

		render.bitmap.clipSource(0,0,0,0);
		render.bitmap.clipDest(
			video.viewport.x,video.viewport.y,
			video.viewport.w,video.viewport.h);
		render.bitmap.setScaler(
			room_bg->w, room_bg->h,
			video.viewport.w,video.viewport.h);
		render.bitmap.setDepth(0, 0.0f);
		render.bitmap.drawImage();

		render.set_dithering(0);
		render.set_useDirtyRects(0);
	}

	/* Background completely restored, clear dirty rectangles list */
	video.dirty_rects[video.numfb]->clear(video.dirty_rects[video.numfb]);

	if (room) {
		if (render_masks) {
			room->drawMasks(room, game->num_camera);
		}

		/*if (!room->getCamera) {
			return;
		}*/

		room->getCamera(room, game->num_camera, &room_camera);

#ifndef ENABLE_DEBUG_POS
		if (refresh_player_pos) {
			player->x = room_camera.to_x;
			player->y = 0 /*room_camera.to_y*/;
			player->z = room_camera.to_z;
			refresh_player_pos = 0;
		}
#endif

		render.set_projection(60.0f, 4.0f/3.0f, RENDER_Z_NEAR, RENDER_Z_FAR);
		render.set_modelview(
			room_camera.from_x, room_camera.from_y, room_camera.from_z,
			room_camera.to_x, room_camera.to_y, room_camera.to_z,
			0.0f, -1.0f, 0.0f
		);
	}

	drawPlayer();

	/* Flush all 3D rendering to screen before drawing 2D stuff */
	render.flushFrame();

	if (room) {
		if (room->map_mode != ROOM_MAP_OFF) {
			room->drawMap(room, render_grid);
		}
	}
}

static void drawPlayer(void)
{
	player_t *player = game->player;

	render.set_render(render_model);

	render.set_color(0x004488cc);

	render.push_matrix();
	render.translate(player->x, player->y, player->z);
	render.rotate((player->a * 360.0f) / 4096.0f, 0.0f,1.0f,0.0f);

	if (player->model) {
		/*Sint16 posx,posy,posz;*/
		Uint32 cur_tick = clockGet();

		if (render_model!=prev_render_model) {
			player->model->download(player->model);
			prev_render_model = render_model;
		}

		switch(render_anim) {
			case -1:
				player->num_frame = ((tick_anim - cur_tick)*15)/1000;
				break;
			case 1:
				player->num_frame = ((cur_tick - tick_anim)*15)/1000;
				break;
			case 0:
			default:
				break;
		}

		if (player->model->setAnimFrame(player->model, player->num_anim, player->num_frame)==0) {
			tick_anim = cur_tick;
		}

		render.set_blending(1);

		/*player->model->getAnimPosition(player->model, &posx, &posy, &posz);
		render.translate((float) -posx, (float) -posy, (float) -posz);

		logMsg(1,"player: %f %f %f, %d,%d,%d\n",player->x, player->y, player->z, posx,posy,posz);*/

		player->model->draw(player->model, 0);
		render.set_blending(0);
		if (render_bones) {
			render.set_color(0x0000ff00);
			player->model->drawBones(player->model, 0);
		}
#if 0
	} else {
		render.translate(0.0f, 2000.0f, 0.0f);

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

	render.set_render(RENDER_TEXTURED);
}
