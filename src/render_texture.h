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

/* Texture must be POT sized */
#define RENDER_TEXTURE_MUST_POT (1<<0)

/* Texture may use a cached version for rescale/dithering */
#define RENDER_TEXTURE_CACHEABLE (1<<1)

/*--- Types ---*/

typedef struct render_texture_s render_texture_t;

struct render_texture_s {
	void (*shutdown)(render_texture_t *this);

	/* Send/remove texture from video card */
	void (*upload)(render_texture_t *this, int num_pal);
	void (*download)(render_texture_t *this);

	void (*prepare_resize)(render_texture_t *this, int *w, int *h);
	void (*resize)(render_texture_t *this, int w, int h);

	void (*load_from_tim)(render_texture_t *this, void *tim_ptr);
	void (*load_from_surf)(render_texture_t *this, SDL_Surface *surf);

	int w, h;		/* Dimension of image zone */
	int bpp, pitch;		/* Bytes per pixel, Line length */
	int pitchw, pitchh;	/* Dimension of bounding zone */
	Uint8 num_palettes, paletted;
	Uint8 must_pot, cacheable;
	
	Uint32 palettes[MAX_TEX_PALETTE][256];	/* N palettes max per texture */
	Uint8 *pixels;			/* Textures are paletted, so 8 bits */

	SDL_PixelFormat format;

	/* Cache for rescaled version */
	SDL_Surface *scaled;
};

/*--- Functions prototypes ---*/

/* Create a texture */
render_texture_t *render_texture_create(int flags);

#endif /* RENDER_TEXTURE_H */
