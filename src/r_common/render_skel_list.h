/*
	List for render models

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

#ifndef RENDER_SKEL_LIST_H
#define RENDER_SKEL_LIST_H 1

/*#include "render_skel.h"*/

/*--- External types ---*/

typedef struct render_skel_s render_skel_t;

/*--- Functions prototypes ---*/

/* Add a skel to the list */
void list_render_skel_add(render_skel_t *skel);

/* Remove skel from list */
void list_render_skel_remove(render_skel_t *skel);

/* Download all skels for video hardware */
void list_render_skel_download(void);

/* Shutdown list of skels */
void list_render_skel_shutdown(void);

#endif /* RENDER_SKEL_LIST_H */
