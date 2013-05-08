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
#include "../parameters.h"

#include "room.h"
#include "game.h"

/*--- Types ---*/

/*--- Constants ---*/

/*--- Global variables ---*/

/*--- Variables ---*/

/*--- Functions prototypes ---*/

static void dtor(room_t *this);

static void load(room_t *this, int stage, int room, int camera);
static void init(room_t *this);

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

static Uint8 *scriptInit(room_t *this, int num_script);
static int scriptGetInstLen(room_t *this, Uint8 *curInstPtr);
static void scriptExecInst(room_t *this);
static void scriptPrintInst(room_t *this);

static void scriptDump(room_t *this, int num_script);
static void scriptExec(room_t *this, int num_script);
static Uint8 *scriptNextInst(room_t *this);

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
	this->init = init;

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

	this->scriptInit = scriptInit;
	this->scriptGetInstLen = scriptGetInstLen;
	this->scriptExecInst = scriptExecInst;
	this->scriptPrintInst = scriptPrintInst;

	this->scriptDump = scriptDump;
	this->scriptExec = scriptExec;
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

static void init(room_t *this)
{
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

static Uint8 *scriptInit(room_t *this, int num_script)
{
	return NULL;
}

static int scriptGetInstLen(room_t *this, Uint8 *curInstPtr)
{
	return 0;
}

static void scriptExecInst(room_t *this)
{
}

static void scriptPrintInst(room_t *this)
{
}

static void scriptDump(room_t *this, int num_script)
{
	Uint8 *inst;
	char strBuf[1024];

	inst = this->scriptInit(this, num_script);
	while (inst) {
		if (params.verbose>=2) {
			int i, inst_len;
			char tmpBuf[16];

			inst_len = this->scriptGetInstLen(this, inst);
			if (inst_len==0) {
				inst_len = 16;
			}

			memset(strBuf, 0, sizeof(strBuf));
			sprintf(tmpBuf, "0x%08x:", this->cur_inst_offset);
			strcat(strBuf, tmpBuf);
			for (i=0; i<inst_len; i++) {
				sprintf(tmpBuf, " 0x%02x", this->cur_inst[i]);
				strcat(strBuf, tmpBuf);
			}
			strcat(strBuf, "\n");
			logMsg(2, strBuf);
		}

		this->scriptPrintInst(this);
		inst = scriptNextInst(this);
	}
}

static void scriptExec(room_t *this, int num_script)
{
	Uint8 *inst;

	inst = this->scriptInit(this, num_script);
	while (inst) {
		this->scriptExecInst(this);
		inst = scriptNextInst(this);
	}
}

static Uint8 *scriptNextInst(room_t *this)
{
	int inst_len;
	Uint8 *cur_inst;

	if (!this) {
		return NULL;
	}
	if (!this->cur_inst) {
		return NULL;
	}

	inst_len = this->scriptGetInstLen(this, cur_inst);
	if (inst_len == 0) {
		return NULL;
	}

	this->cur_inst_offset += inst_len;	
	if (this->script_length>0) {
		if (this->cur_inst_offset>= this->script_length) {
			logMsg(1, "End of script reached\n");
			return NULL;
		}
	}

	cur_inst = this->cur_inst;

	this->cur_inst = &cur_inst[inst_len];
	return this->cur_inst;
}
