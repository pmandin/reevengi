/*
	Room script functions

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

/*--- Functions prototypes ---*/

static Uint8 *scriptInit(room_t *this, int num_script);
static int scriptGetInstLen(room_t *this, Uint8 *curInstPtr);
static void scriptExecInst(room_t *this);

static void scriptDump(room_t *this, int num_script);
static void scriptExec(room_t *this, int num_script);

static Uint8 *scriptNextInst(room_t *this);

/*--- Functions ---*/

void room_script_init(room_t *this)
{
	this->scriptInit = scriptInit;
	this->scriptGetInstLen = scriptGetInstLen;
	this->scriptExecInst = scriptExecInst;

	this->scriptDump = scriptDump;
	this->scriptExec = scriptExec;
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

		/*this->scriptPrintInst(this);*/
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

	inst_len = this->scriptGetInstLen(this, this->cur_inst);
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
