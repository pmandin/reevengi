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

#ifndef ROOM_ITEM_H
#define ROOM_ITEM_H 1

/*--- Types ---*/

typedef struct room_item_s room_item_t;

struct room_item_s {
	Sint16 x,y,w,h;

	int num_item;
};

/*--- Functions ---*/

void room_item_init(room_t *this);

#endif /* ROOM_ITEM_H */
