/*
	List for render textures

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

#ifndef RENDER_TEXTURE_LIST_H
#define RENDER_TEXTURE_LIST_H 1

/*--- External types ---*/

struct render_texture_s;

/*--- Functions prototypes ---*/

/* Add a texture to the list */
void list_render_texture_add(struct render_texture_s *texture);

/* Remove texture from list */
void list_render_texture_remove(struct render_texture_s *texture);

/* Download all textures for video hardware */
void list_render_texture_download(void);

/* Shutdown list of textures */
void list_render_texture_shutdown(void);

#endif /* RENDER_TEXTURE_LIST_H */
