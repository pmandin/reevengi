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

#ifndef CLOCK_H
#define CLOCK_H 1

#include <SDL.h>

/*--- Functions prototypes ---*/

/* Init clock */
void clockInit(void);

/* Get clock time */
Uint32 clockGet(void);

/* Pause clock */
void clockPause(void);

/* Unpause clock */
void clockUnpause(void);

/* Get last paused duration */
Uint32 clockPausedTime(void);

#endif /* CLOCK_H */
