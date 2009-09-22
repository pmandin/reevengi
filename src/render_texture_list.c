/*
	List for render textures

	Copyright (C) 2008	Patrice Mandin

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

#include "render_texture.h"

/*--- Variables ---*/

static render_texture_t **render_texture_list = NULL;
static int render_texture_list_size = 0;

/*--- Functions ---*/

void list_render_texture_add(render_texture_t *texture)
{
	int i;
	render_texture_t **new_list;

	/* Check not already present */
	for (i=0; i<render_texture_list_size; i++) {
		if (render_texture_list[i] == texture) {
			return;
		}
	}

	/* Try to add at empty place */
	for (i=0; i<render_texture_list_size; i++) {
		if (!render_texture_list[i]) {
			render_texture_list[i] = texture;
			return;
		}
	}

	/* Allocate room for new texture */
	new_list = (render_texture_t **) realloc(render_texture_list,
		(render_texture_list_size+1) * sizeof(render_texture_t *));
	if (!new_list) {
		/* Failed */
		fprintf(stderr, "Failed allocating memory for render_texture_list\n");
		return;
	}

	render_texture_list = new_list;
	render_texture_list[render_texture_list_size++] = texture;
}

void list_render_texture_remove(render_texture_t *texture)
{
	int i;

	if (!render_texture_list) {
		return;
	}

	for (i=0; i<render_texture_list_size; i++) {
		if (render_texture_list[i] == texture) {
			render_texture_list[i] = NULL;
			return;
		}
	}
}

void list_render_texture_download(void)
{
	int i;

	if (!render_texture_list) {
		return;
	}

	for (i=0; i<render_texture_list_size; i++) {
		render_texture_t *texture = render_texture_list[i];
		if (texture) {
			texture->download(texture);
		}
	}
}

void list_render_texture_shutdown(void)
{
	if (render_texture_list) {
		free(render_texture_list);
		render_texture_list = NULL;
	}
}
