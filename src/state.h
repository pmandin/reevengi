/*
	Game state

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

#ifndef STATE_H
#define STATE_H 1

#include <SDL.h>

/*--- Types ---*/

typedef struct {
	/*--- Data ---*/

	/* Stage of game */
	int stage;
	/* Room of stage */
	int room;
	/* Camera in room */
	int camera;

	/* Camera angles in background file */
	int num_cameras;
	/* Background image */
	SDL_Surface *surface_bg;
	/* Background file */
	char *background;

	/*--- Functions ---*/
	void (*load_background)(void);
} state_t;

/*--- Variables ---*/

extern state_t game_state;

/*--- Functions ---*/

void state_init(void);

void state_setstage(int new_stage);
void state_setroom(int new_room);
void state_setcamera(int new_camera);

void state_unloadbackground(void);

#endif /* STATE_H */
