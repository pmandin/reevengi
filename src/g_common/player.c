/*
	Player data

	Copyright (C) 2007-2013	Patrice Mandin

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

#include <string.h>

#include "player.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/


/*--- Functions prototypes ---*/

static void player_shutdown(player_t *this);

/*--- Functions ---*/

void player_init(player_t *this)
{
	memset(this, 0, sizeof(player_t));

	this->init = player_init;
	this->shutdown = player_shutdown;

#ifdef ENABLE_DEBUG_POS
	this->x = 13148.0f;
	this->y = -2466.0f;
	this->z = 3367.0f;
	this->a = (157.0f * 4096.0f) / 360.0f;
#endif
}

static void player_shutdown(player_t *this)
{
}
