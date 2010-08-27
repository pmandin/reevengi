/*
	Draw background image mask
	OpenGL renderer

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

#include "video.h"
#include "render.h"
#include "render_mask.h"

#ifdef ENABLE_OPENGL

#include "render_mask_opengl.h"

/*--- Functions prototypes ---*/

static void shutdown(render_mask_t *this);

static void addZone(render_mask_t *this,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth);

static void finishedZones(render_mask_t *this);

static void drawMask(render_mask_t *this);

/*--- Functions ---*/

render_mask_t *render_mask_opengl_create(render_texture_t *texture)
{
	render_mask_gl_t *gl_mask;
	render_mask_t *mask;

	mask = render_mask_soft_create(texture);
	if (!mask) {
		return NULL;
	}

	gl_mask = realloc(mask, sizeof(render_mask_gl_t));
	if (!gl_mask) {
		fprintf(stderr, "Can not allocate memory for render_mask\n");
		return NULL;
	}

	mask = (render_mask_t *) gl_mask;

	mask->shutdown = shutdown;
	mask->addZone = addZone;
	mask->finishedZones = finishedZones;
	mask->drawMask = drawMask;

	gl_mask->num_zones = 0;
	gl_mask->zones = NULL;

	return mask;
}

static void shutdown(render_mask_t *this)
{
	render_mask_gl_t *gl_mask = (render_mask_gl_t *) this;

	if (!this) {
		return;
	}

	if (gl_mask->zones) {
		free(gl_mask->zones);
		gl_mask->zones = NULL;
	}
	free(this);
}

static void addZone(render_mask_t *this,
	int srcX, int srcY, int w,int h,
	int dstX, int dstY, int depth)
{
	render_mask_gl_t *gl_mask;
	render_mask_gl_zone_t *zones, *new_zone;

	assert(this);
	gl_mask = (render_mask_gl_t *) this;

	zones = realloc(gl_mask->zones, (gl_mask->num_zones+1) * sizeof(render_mask_gl_zone_t));
	if (!zones) {
		fprintf(stderr, "render_mask_opengl: Failed to allocate memory for zones\n");
		return;
	}

	new_zone = &zones[gl_mask->num_zones];
	new_zone->srcx = srcX;
	new_zone->srcy = srcY;
	new_zone->width = w;
	new_zone->height = h;
	new_zone->dstx = dstX;
	new_zone->dsty = dstY;
	new_zone->depth = (float) depth;

	gl_mask->zones = zones;
	++gl_mask->num_zones;
}

static void finishedZones(render_mask_t *this)
{
}

static void drawMask(render_mask_t *this)
{
	int i;
	render_mask_gl_t *gl_mask;
	render_texture_t *tex;

	assert(this);
	assert(this->texture);
	gl_mask = (render_mask_gl_t *) this;
	tex = this->texture;

	render.set_dithering(0);
	render.set_useDirtyRects(0);
	render.set_texture(0, tex);
	render.bitmap.setMasking(1);
	render.bitmap.setScaler(
		tex->w, tex->h,
		(tex->w*video.viewport.w)/RENDER_MASK_WIDTH,
		(tex->h*video.viewport.h)/RENDER_MASK_HEIGHT);
	render.set_blending(1);

	for (i=0; i<gl_mask->num_zones; i++) {
		render_mask_gl_zone_t *zone = &(gl_mask->zones[i]);

		int x1 = (zone->dstx*video.viewport.w)/RENDER_MASK_WIDTH;
		int y1 = (zone->dsty*video.viewport.h)/RENDER_MASK_HEIGHT;
		int x2 = ((zone->dstx+zone->width)*video.viewport.w)/RENDER_MASK_WIDTH;
		int y2 = ((zone->dsty+zone->height)*video.viewport.h)/RENDER_MASK_HEIGHT;

		render.bitmap.clipSource(
			zone->srcx, zone->srcy,
			zone->width, zone->height);
		render.bitmap.clipDest(
			video.viewport.x+x1,
			video.viewport.y+y1,
			x2-x1,y2-y1);
		render.bitmap.setDepth(1, zone->depth);
		render.bitmap.drawImage();
	}

	render.bitmap.setDepth(0, 0.0f);
	render.bitmap.setMasking(0);
	render.set_blending(0);
	render.set_dithering(0);
}

#else /* ENABLE_OPENGL */

render_mask_t *render_mask_opengl_create(render_texture_t *texture)
{
	return NULL;
}

#endif /* ENABLE_OPENGL */
