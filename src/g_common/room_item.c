/*
	Room item

	Copyright (C) 2009-2013	Patrice Mandin

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
#include <SDL.h>

#include "../log.h"
#include "../parameters.h"
#include "../filesystem.h"

#include "room.h"
#include "room_item.h"

/*--- Functions prototypes ---*/

static void addItem(room_t *this, room_item_t *item);

/*--- Functions ---*/

void room_item_init(room_t *this)
{
	this->addItem = addItem;
}

static void addItem(room_t *this, room_item_t *item)
{
	this->items = realloc(this->items, (this->num_items+1) * sizeof(room_item_t));
	if (!this->items) {
		logMsg(0, "room_item: Can not allocate memory for item\n");
		return;
	}

	memcpy(&this->items[this->num_items], item, sizeof(room_item_t));
	logMsg(1, "room_item: Adding item %d (x=%d,y=%d,%dx%d)\n", this->num_items,
		item->x,item->y,item->w,item->h);

	++this->num_items;
}
