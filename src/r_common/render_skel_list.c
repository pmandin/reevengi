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

#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>

#include "render.h"
#include "render_skel.h"

/*--- Variables ---*/

static render_skel_t **render_skel_list = NULL;
static int render_skel_list_size = 0;

/*--- Functions ---*/

void list_render_skel_add(render_skel_t *skel)
{
	int i;
	render_skel_t **new_list;

	/* Check not already present */
	for (i=0; i<render_skel_list_size; i++) {
		if (render_skel_list[i] == skel) {
			return;
		}
	}

	/* Try to add at empty place */
	for (i=0; i<render_skel_list_size; i++) {
		if (!render_skel_list[i]) {
			render_skel_list[i] = skel;
			return;
		}
	}

	/* Allocate room for new skel */
	new_list = (render_skel_t **) realloc(render_skel_list,
		(render_skel_list_size+1) * sizeof(render_skel_t *));
	if (!new_list) {
		/* Failed */
		fprintf(stderr, "Failed allocating memory for render_skel_list\n");
		return;
	}

	render_skel_list = new_list;
	render_skel_list[render_skel_list_size++] = skel;
}

void list_render_skel_remove(render_skel_t *skel)
{
	int i;

	if (!render_skel_list) {
		return;
	}

	for (i=0; i<render_skel_list_size; i++) {
		if (render_skel_list[i] == skel) {
			render_skel_list[i] = NULL;
			return;
		}
	}
}

void list_render_skel_download(void)
{
	int i;

	if (!render_skel_list) {
		return;
	}

	for (i=0; i<render_skel_list_size; i++) {
		render_skel_t *skel = render_skel_list[i];
		if (skel) {
			skel->download(skel);
		}
	}
}

void list_render_skel_shutdown(void)
{
	if (render_skel_list) {
		free(render_skel_list);
		render_skel_list = NULL;
	}
}
