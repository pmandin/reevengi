/*
	Room
	Cameras switches and boundaries

	Copyright (C) 2007-2013	Patrice Mandin

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

#include <string.h>
#include <SDL.h>

#include "../log.h"
#include "../parameters.h"
#include "../filesystem.h"
#include "../r_common/render.h"

#include "room.h"
#include "room_camswitch.h"
#include "game.h"

/*--- Functions prototypes ---*/

static int getNumCamSwitches(room_t *this);
static void getCamSwitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);
static int checkCamSwitch(room_t *this, int num_camera, float x, float y);
static void drawCamSwitches(room_t *this);

static int getNumBoundaries(room_t *this);
static void getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_boundary);
static int checkBoundary(room_t *this, int num_camera, float x, float y);
static void drawBoundaries(room_t *this);

/*--- Functions ---*/

void room_camswitch_init(room_t *this)
{
	this->getNumCamSwitches = getNumCamSwitches;
	this->getCamSwitch = getCamSwitch;
	this->checkCamSwitch = checkCamSwitch;
	this->drawCamSwitches = drawCamSwitches;

	this->getNumBoundaries = getNumBoundaries;
	this->getBoundary = getBoundary;
	this->checkBoundary = checkBoundary;
	this->drawBoundaries = drawBoundaries;
}

static int getNumCamSwitches(room_t *this)
{
	return 0;
}

static void getCamSwitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
}

static int checkCamSwitch(room_t *this, int num_camera, float x, float y)
{
	int i,j;

	if (!this) {
		return -1;
	}

	for (i=0; i<this->getNumCamSwitches(this); i++) {
		room_camswitch_t room_camswitch;
		int is_inside = 1;

		this->getCamSwitch(this, i, &room_camswitch);

		if (room_camswitch.from != num_camera) {
			continue;
		}

		for (j=0; j<4; j++) {
			float dx1,dy1,dx2,dy2;

			dx1 = room_camswitch.x[(j+1) & 3] - room_camswitch.x[j];
			dy1 = room_camswitch.y[(j+1) & 3] - room_camswitch.y[j];

			dx2 = x - room_camswitch.x[j];
			dy2 = y - room_camswitch.y[j];

			if (dx1*dy2-dy1*dx2 >= 0) {
				is_inside = 0;
			}
		}

		if (is_inside) {
			return room_camswitch.to;
		}
	}

	return -1;
}

static void drawCamSwitches(room_t *this)
{
	int i, j;

	for (i=0; i<this->getNumCamSwitches(this); i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[4];

		this->getCamSwitch(this, i, &room_camswitch);

		if (room_camswitch.from != game->num_camera) {
			continue;
		}

		for (j=0; j<4; j++) {
			v[j].x = room_camswitch.x[j];
			v[j].y = 0.0f;
			v[j].z = room_camswitch.y[j];
		}

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
	}
}

static int getNumBoundaries(room_t *this)
{
	return 0;
}

static void getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_boundary)
{
}

static int checkBoundary(room_t *this, int num_camera, float x, float y)
{
	int i,j;

	if (!this) {
		return 0;
	}

	for (i=0; i<this->getNumBoundaries(this); i++) {
		room_camswitch_t room_camswitch;
		int is_inside = 1;

		this->getBoundary(this, i, &room_camswitch);

		if (room_camswitch.from != num_camera) {
			continue;
		}

		for (j=0; j<4; j++) {
			float dx1,dy1,dx2,dy2;

			dx1 = room_camswitch.x[(j+1) & 3] - room_camswitch.x[j];
			dy1 = room_camswitch.y[(j+1) & 3] - room_camswitch.y[j];

			dx2 = x - room_camswitch.x[j];
			dy2 = y - room_camswitch.y[j];

			if (dx1*dy2-dy1*dx2 >= 0) {
				is_inside = 0;
			}
		}

		if (!is_inside) {
			return 1;
		}
	}

	return 0;
}

static void drawBoundaries(room_t *this)
{
	int i, j;

	for (i=0; i<this->getNumBoundaries(this); i++) {
		room_camswitch_t room_camswitch;
		vertex_t v[4];

		this->getBoundary(this, i, &room_camswitch);

		if (room_camswitch.from != game->num_camera) {
			continue;
		}

		for (j=0; j<4; j++) {
			v[j].x = room_camswitch.x[j];
			v[j].y = 0.0f;
			v[j].z = room_camswitch.y[j];
		}

		render.quad_wf(&v[0], &v[1], &v[2], &v[3]);
	}
}
