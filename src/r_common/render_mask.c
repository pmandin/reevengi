/*
	Draw background image mask

	Copyright (C) 2010	Patrice Mandin

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <SDL.h>

#include "render_mask.h"

/*--- Functions prototypes ---*/

static void shutdown(render_mask_t *this);

static void addZone(render_mask_t *this,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth);

static void finishedZones(render_mask_t *this);

static void drawMask(render_mask_t *this);

/*--- Functions ---*/

render_mask_t *render_mask_create(render_texture_t *texture)
{
	return NULL;
}

void render_mask_init(render_mask_t *this, render_texture_t *texture)
{
	assert(this);

	this->shutdown = shutdown;
	this->addZone = addZone;
	this->finishedZones = finishedZones;
	this->drawMask = drawMask;

	/* Texture freed outside */
	this->texture = texture;
}

static void shutdown(render_mask_t *this)
{
	if (!this) {
		return;
	}

	free(this);
}

static void addZone(render_mask_t *this,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth)
{
}

static void finishedZones(render_mask_t *this)
{
}

static void drawMask(render_mask_t *this)
{
}
