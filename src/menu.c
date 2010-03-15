/*
	Debug menu

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

#include <SDL.h>

#include "render_texture.h"
#include "render_skel.h"
#include "state.h"

#include "video.h"
#include "render.h"

/*--- Defines ---*/

#define START_X	16
#define START_Y 16

/*--- Constants ---*/

static char *title=PACKAGE_STRING;

/*--- Functions prototypes ---*/

static void menu_text_print(const char *str, int x, int y);

/*--- Functions ---*/

void menu_render(void)
{
	int i;

	if (!game_state.font) {
		return;
	}

	render.set_dithering(0);
	render.set_texture(0, game_state.font);
	render.set_blending(1);
	render.set_useDirtyRects(0);
	render.bitmap.setMasking(0);
	render.bitmap.setScaler(render.texture->w,render.texture->h,
		render.texture->w,render.texture->h);
	render.bitmap.setDepth(0, 0.0f);

	menu_text_print(title,START_X,START_Y);
}

static void menu_text_print(const char *str, int x, int y)
{
	Uint8 c;
	int sx,sy,sw,sh;
	int dx=video.viewport.x+x,dy=video.viewport.y+y,dw=8,dh=8; /* dirtied zone */

	while (c=*str++) {
		if (c>32) {
			game_state.get_char(c, &sx, &sy, &sw, &sh);

			render.bitmap.clipSource(sx,sy, sw,sh);
			render.bitmap.clipDest(video.viewport.x+x,video.viewport.y+y, sw,sh);
			render.bitmap.drawImage(&video);
		}

		x+= sw;
		dw += sw;
		dh = sh;
	}

	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		dx,dy,dw,dh);
}
