/*
	Game clock

	Copyright (C) 2009	Patrice Mandin

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

#include <SDL.h>

/*--- Variables ---*/

static Uint32 running_time, start_pause, end_pause;
static int paused;

/*--- Functions ---*/

void clockInit(void)
{
	running_time = paused = 0;
	start_pause = end_pause = SDL_GetTicks();
}

Uint32 clockGet(void)
{
	if (paused) {
		return running_time;
	}

	return running_time + SDL_GetTicks() - end_pause;
}

void clockPause(void)
{
	if (!paused) {
		paused = 1;
		start_pause = SDL_GetTicks();
		/* end_pause is end of previous pause */
		running_time += start_pause - end_pause;
	}
}

void clockUnpause(void)
{
	if (paused) {
		paused = 0;
		end_pause = SDL_GetTicks();
	}
}

Uint32 clockPausedTime(void)
{
	if (paused) {
		return 0;
	}

	return end_pause - start_pause;
}
