/*
	Textures for 3D objects

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

#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H 1

/*--- Defines ---*/

#define MAX_TEX_PALETTE 8

/*--- Types ---*/

typedef struct {
	int w, h;		/* Dimension of image zone */
	int pitchw, pitchh;	/* Dimension of bouding zone */
	int pitch;
	int paletted;
	Uint32 palettes[256][MAX_TEX_PALETTE];	/* N palettes max per texture */
	Uint8 *pixels;			/* Textures are paletted, so 8 bits */
} render_texture_t;

/*--- Functions prototypes ---*/

/* Load texture from a TIM image file as pointer */
render_texture_t *render_texture_load_from_tim(void *tim_ptr);

/* Free texture data */
void render_texture_shutdown(render_texture_t *texture);

#endif /* RENDER_TEXTURE_H */
