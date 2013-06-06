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
#include "room_collision.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void dtor(room_t *this);

static char *getFilename(room_t *this, int stage, int room, int camera);
static void loadFile(room_t *this, int stage, int room, int camera);

static void load(room_t *this, int stage, int room, int camera);
static void init(room_t *this);

static void load_background(room_t *this, int stage, int room, int camera);
static void load_bgmask(room_t *this, int stage, int room, int camera);
static void changeCamera(room_t *this, int num_stage, int num_room, int num_camera);

static void unload(room_t *this);
static void unload_background(room_t *this);
static void unload_bgmask(room_t *this);

static int getNumCameras(room_t *this);
static void getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static void initMasks(room_t *this, int num_camera);
static void drawMasks(room_t *this, int num_camera);

static void getText(room_t *this, int lang, int num_text, char *buffer, int bufferLen);

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

	this->getFilename = getFilename;
	this->loadFile = loadFile;

	this->load = load;
	this->init = init;

	this->load_background = load_background;
	this->load_bgmask = load_bgmask;
	this->changeCamera = changeCamera;

	this->getNumCameras = getNumCameras;
	this->getCamera = getCamera;

	room_camswitch_init(this);

	this->initMasks = initMasks;
	this->drawMasks = drawMasks;

	this->getText = getText;

	room_script_init(this);
	room_door_init(this);
	room_map_init(this);
	room_collision_init(this);

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

static char *getFilename(room_t *this, int stage, int room, int camera)
{
	return strdup("");
}

static void loadFile(room_t *this, int stage, int room, int camera)
{
	PHYSFS_sint64 length;
	void *file;
	char *filename;
	int retval = 0;
	
	filename = this->getFilename(this, stage, room, camera);
	if (!filename) {
		return;
	}

	logMsg(1, "room: Loading %s ...\n", filename);

	file = FS_Load(filename, &length);
	if (file) {
		this->file = file;
		this->file_length = length;
		this->init(this);

		logMsg(2, "room: %d cameras angles, %d camera switches, %d boundaries\n",
			this->num_cameras, this->getNumCamSwitches(this), this->getNumBoundaries(this));

		retval = 1;
	}

	logMsg(1, "room: %s loading %s ...\n",
		retval ? "Done" : "Failed",
		filename);

	free(filename);
}

static void load(room_t *this, int stage, int room, int camera)
{
	unload(this);

	this->loadFile(this, stage, room, camera);

	room_map_init_data(this);

	/* Dump scripts if wanted */
	if (params.dump_script) {
		this->scriptDump(this, ROOM_SCRIPT_INIT);
		this->scriptDump(this, ROOM_SCRIPT_RUN);
	}
	this->scriptExec(this, ROOM_SCRIPT_INIT);
}

static void init(room_t *this)
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

static void changeCamera(room_t *this, int num_stage, int num_room, int num_camera)
{
	this->load_background(this, num_stage, num_room, num_camera);

	this->load_bgmask(this, num_stage, num_room, num_camera);
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

static void getText(room_t *this, int lang, int num_text, char *buffer, int bufferLen)
{
}
