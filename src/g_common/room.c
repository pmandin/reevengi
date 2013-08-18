/*
	Room

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

#include "room.h"
#include "game.h"

#include "room_script.h"
#include "room_camswitch.h"
#include "room_map.h"
#include "room_door.h"
#include "room_item.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void dtor(room_t *this);

static char *getFilename(room_t *this);
static void loadFile(room_t *this);
static void postLoad(room_t *this);

static void load_background(room_t *this, int stage, int room, int camera);
static void load_bgmask(room_t *this, int stage, int room, int camera);
static void setCamera(room_t *this, int num_camera);

static void unload(room_t *this);
static void unload_background(room_t *this);
static void unload_bgmask(room_t *this);

static int getNumCameras(room_t *this);
static void getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static void initMasks(room_t *this, int num_camera);
static void drawMasks(room_t *this, int num_camera);

static void displayTexts(room_t *this, int num_lang);
static void getText(room_t *this, int lang, int num_text, char *buffer, int bufferLen);

static int getNumCollisions(room_t *this);
static void drawMapCollision(room_t *this, int num_collision);
static int checkCollision(room_t *this, int num_collision, float x, float y);
static int checkCollisions(room_t *this, float x, float y);

/*--- Functions ---*/

room_t *room_ctor(game_t *game, int num_stage, int num_room)
{
	room_t *this;

	logMsg(2, "room: ctor\n");

	this = (room_t *) calloc(1, sizeof(room_t));
	if (!this) {
		return NULL;
	}

	this->dtor = dtor;

	this->getFilename = getFilename;
	this->loadFile = loadFile;
	this->postLoad = postLoad;

	this->load_background = load_background;
	this->load_bgmask = load_bgmask;
	this->setCamera = setCamera;

	this->getNumCameras = getNumCameras;
	this->getCamera = getCamera;

	this->initMasks = initMasks;
	this->drawMasks = drawMasks;

	this->displayTexts = displayTexts;
	this->getText = getText;

	this->getNumCollisions = getNumCollisions;
	this->drawMapCollision = drawMapCollision;
	this->checkCollision = checkCollision;
	this->checkCollisions = checkCollisions;

	room_camswitch_init(this);
	room_script_init(this);
	room_door_init(this);
	room_map_init(this);
	room_item_init(this);

	this->num_stage = num_stage;
	this->num_room = num_room;

	return this;
}

static void dtor(room_t *this)
{
	logMsg(2, "room: dtor\n");

	unload_background(this);
	unload_bgmask(this);
	unload(this);

	free(this);
}

static char *getFilename(room_t *this)
{
	return NULL;
}

static void loadFile(room_t *this)
{
	PHYSFS_sint64 length;
	void *file;
	char *filename;
	int retval = 0;

	/*logMsg(1, "room: stage %d, room %d\n", this->num_stage, this->num_room);*/

	filename = this->getFilename(this);
	if (!filename) {
		return;
	}

	logMsg(1, "room: Loading %s ...\n", filename);

	file = FS_Load(filename, &length);
	if (file) {
		if (length>=8) {
			this->file = file;
			this->file_length = length;

			retval = 1;
		} else {
			free(file);
		}
	}

	logMsg(1, "room: %s loading %s ...\n",
		retval ? "Done" : "Failed",
		filename);

	free(filename);
}

static void postLoad(room_t *this)
{
}

static void load_background(room_t *this, int stage, int room, int camera)
{
	unload_background(this);
}

static void load_bgmask(room_t *this, int stage, int room, int camera)
{
	unload_bgmask(this);
}

static void unload(room_t *this)
{
	logMsg(2, "room: unload\n");

	if (this->doors) {
		free(this->doors);
		this->doors = NULL;

		this->num_doors=0;
	}

	if (this->items) {
		free(this->items);
		this->items = NULL;

		this->num_items=0;
	}

	if (this->file) {
		free(this->file);
		this->file=NULL;
		this->file_length=0;
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

static void setCamera(room_t *this, int num_camera)
{
	this->load_background(this, this->num_stage, this->num_room, num_camera);
	this->load_bgmask(this, this->num_stage, this->num_room, num_camera);

	this->initMasks(this, num_camera);
}

static int getNumCameras(room_t *this)
{
	return 0;
}

static void getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
}

static void initMasks(room_t *this, int num_camera)
{
}

static void drawMasks(room_t *this, int num_camera)
{
}

static void displayTexts(room_t *this, int num_lang)
{
}

static void getText(room_t *this, int lang, int num_text, char *buffer, int bufferLen)
{
}

static int getNumCollisions(room_t *this)
{
	return 0;
}

static void drawMapCollision(room_t *this, int num_collision)
{
}

static int checkCollision(room_t *this, int num_collision, float x, float y)
{
	return 0;
}

static int checkCollisions(room_t *this, float x, float y)
{
	int i;

	for (i=0; i<this->getNumCollisions(this); i++) {
		if (this->checkCollision(this, i, x,y)) {
			return 1;
		}
	}

	return 0;
}
