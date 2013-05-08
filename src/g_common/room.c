/*
	Room data

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

#include "room.h"
#include "game.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void dtor(room_t *this);

static void load(room_t *this, int stage, int room, int camera);
static void load_background(room_t *this, int stage, int room, int camera);
static void load_bgmask(room_t *this, int stage, int room, int camera);

static void unload(room_t *this);
static void unload_background(room_t *this);
static void unload_bgmask(room_t *this);

static int getNumCameras(room_t *this);
static void getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static int getNumCamSwitches(room_t *this);
static void getCamSwitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

static int getNumBoundaries(room_t *this);
static void getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_boundary);

static void initMasks(room_t *this, int num_camera);
static void drawMasks(room_t *this, int num_camera);

/*--- Functions ---*/

room_t *room_ctor(void)
{
	room_t *this;

	logMsg(2, "room: ctor\n");

	this = (room_t *) calloc(1, sizeof(room_t));
	if (!this) {
		return NULL;
	}

	this->dtor = dtor;

	this->load = load;
	this->load_background = load_background;
	this->load_bgmask = load_bgmask;

	this->getNumCameras = getNumCameras;
	this->getCamera = getCamera;

	this->getNumCamSwitches = getNumCamSwitches;
	this->getCamSwitch = getCamSwitch;

	this->getNumBoundaries = getNumBoundaries;
	this->getBoundary = getBoundary;

	this->initMasks = initMasks;
	this->drawMasks = drawMasks;
}

static void dtor(room_t *this)
{
	logMsg(2, "room: dtor\n");

	unload_background(this);
	unload_bgmask(this);
	unload(this);

	free(this);
}

static void load(room_t *this, int stage, int room, int camera)
{
	unload(this);
}

static void load_background(room_t *this, int stage, int room, int camera)
{
	unloadbackground(this);
}

static void load_bgmask(room_t *this, int stage, int room, int camera)
{
	unload_bgmask(this);
}

static void unload(room_t *this)
{
	logMsg(2, "room: unload\n");

	if (this->file) {
		free(this->file);
		this->file=NULL;
	}
}

static void unload_background(room_t *this)
{
	logMsg(2, "room: unloadbackground\n");

	if (this->background) {
		free(this->background);
		this->background=NULL;
	}
}

static void unload_bgmask(room_t *this)
{
	logMsg(2, "room: unloadbgmask\n");

	if (this->bg_mask) {
		free(this->bg_mask);
		this->bg_mask=NULL;
	}
	if (this->rdr_mask) {
		free(this->rdr_mask);
		this->rdr_mask=NULL;
	}
}

static int getNumCameras(room_t *this)
{
	return 0;
}

static void getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
}

static int getNumCamSwitches(room_t *this)
{
	return 0;
}

static void getCamSwitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
}

static int getNumBoundaries(room_t *this)
{
	return 0;
}

static void getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_boundary)
{
}

static void initMasks(room_t *this, int num_camera)
{
}

static void drawMasks(room_t *this, int num_camera)
{
}
