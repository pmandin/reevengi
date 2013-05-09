/*
	Room description
	RE2 RDT manager

	Copyright (C) 2009-2013	Patrice Mandin

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

#include <SDL.h>

#include "../log.h"

#include "../g_common/room.h"

#include "rdt.h"

/*--- Constants ---*/

static const char txt2asc[0x60]={
	' ','.','?','?', '?','(',')','?', '?','?','?','?', '0','1','2','3',
	'4','5','6','7', '8','9',':','?', ',','"','!','?', '?','A','B','C',
	'D','E','F','G', 'H','I','J','K', 'L','M','N','O', 'P','Q','R','S',
	'T','U','V','W', 'X','Y','Z','[', '/',']','\'','-', '_','a','b','c',
	'd','e','f','g', 'h','i','j','k', 'l','m','n','o', 'p','q','r','s',
	't','u','v','w', 'x','y','z','?', '?','?','?','?', '?','?','?','?'
};

static const char *txtcolor[6]={
	"white", "green", "red", "grey", "blue", "black"
};

/*--- Types ---*/

typedef struct {
	Uint8 id[8];
} rdt_anim_list_t;

typedef struct {
	Uint16 num_frames;
	Uint16 num_sprites;
	Uint8 w,h;
	Uint16 unknown;
} rdt_anim_header_t;

typedef struct {
	Uint8 sprite;
	Uint8 unknown[7];
} rdt_anim_step_t;

typedef struct {
	Uint8 x,y;
	Sint8 offset_x,offset_y;
} rdt_anim_sprite_t;

typedef struct {
	Uint16 offsets[8];
} rdt_anim_unknown0_t;

typedef struct {
	Uint8 unknown[0x38];
} rdt_anim_unknown1_t;

typedef struct {
	Uint32 length;
} rdt_anim_end_t;

/*--- Functions prototypes ---*/

static void rdt2_displayTexts(room_t *this, int num_lang);

/*--- Functions ---*/

void rdt2_init(room_t *this)
{
	rdt2_header_t *rdt_header = (rdt2_header_t *) this->file;

	if (this->file_length > 4) {
		this->num_cameras = this->getNumCameras(this);
#if 0
		this->num_camswitches = rdt2_getNumCamswitches(this);
		this->num_boundaries = rdt2_getNumBoundaries(this);

		this->getCamera = rdt2_getCamera;
		this->getCamswitch = rdt2_getCamswitch;
		this->getBoundary = rdt2_getBoundary;

		this->drawMasks = rdt2_drawMasks;

/*		switch(game_state.version) {
			case GAME_RE3_PS1_GAME:
			case GAME_RE3_PC_GAME:
			case GAME_RE3_PC_DEMO:
				room_rdt3_scriptInit(this);
				break;
			default:*/
				room_rdt2_scriptInit(this);
/*				break;
		}*/
#endif
	}

/*	logMsg(2, "%d cameras angles, %d camera switches, %d boundaries\n",
		this->num_cameras, this->num_camswitches, this->num_boundaries);*/

	/* Display texts */
	rdt2_displayTexts(this, 0);	/* language 1 */
	rdt2_displayTexts(this, 1);	/* language 2 */
}

static void rdt2_displayTexts(room_t *this, int num_lang)
{
	rdt2_header_t *rdt_header = (rdt2_header_t *) this->file;
	int room_lang = (num_lang==0) ? RDT2_OFFSET_TEXT_LANG1 : RDT2_OFFSET_TEXT_LANG2;
	Uint32 offset;
	Uint16 *txtOffsets, txtCount;
	int i;
	char tmpBuf[512];

	logMsg(1, "Language %d\n", num_lang);

	offset = SDL_SwapLE32(rdt_header->offsets[room_lang]);
	if (offset == 0) {
		logMsg(1, " No texts to display\n");
		return;
	}

	txtOffsets = (Uint16 *) &((Uint8 *) this->file)[offset];

	txtCount = SDL_SwapLE16(txtOffsets[0]) >> 1;
	for (i=0; i<txtCount; i++) {
		room_rdt2_getText(this, num_lang, i, tmpBuf, sizeof(tmpBuf));
		logMsg(1, " Text[0x%02x]: %s\n", i, tmpBuf);
	}
}

void room_rdt2_getText(room_t *this, int lang, int num_text, char *buffer, int bufferLen)
{
	rdt2_header_t *rdt_header = (rdt2_header_t *) this->file;
	int room_lang = (lang==0) ? RDT2_OFFSET_TEXT_LANG1 : RDT2_OFFSET_TEXT_LANG2;
	Uint32 offset;
	Uint16 *txtOffsets, txtCount;
	Uint8 *txtPtr;
	int i = 0;
	char strBuf[32];

	memset(buffer, 0, sizeof(bufferLen));
	
	offset = SDL_SwapLE32(rdt_header->offsets[room_lang]);
	if (offset == 0) {
		return;
	}

	txtOffsets = (Uint16 *) &((Uint8 *) this->file)[offset];

	txtCount = SDL_SwapLE16(txtOffsets[0]) >> 1;
	if (num_text>=txtCount) {
		return;
	}

	txtPtr = &((Uint8 *) this->file)[offset + SDL_SwapLE16(txtOffsets[num_text])];

	while ((txtPtr[i] != 0xfe) && (i<bufferLen-1)) {
		switch(txtPtr[i]) {
			case 0xf3:
				strncat(buffer, "[0xf3]", bufferLen-1);
				break;
			case 0xf8:
				/* Item */
				sprintf(strBuf, "<item id=\"%d\">", txtPtr[i+1]);
				strncat(buffer, strBuf, bufferLen-1);
				i++;
				break;
			case 0xf9:
				/* Text color */
				if (txtPtr[i+1]<6) {
					sprintf(strBuf, "<text color=\"%s\">", txtcolor[txtPtr[i+1]]);
				} else {
					sprintf(strBuf, "<text color=\"0x%02x\">", txtPtr[i+1]);
				}
				strncat(buffer, strBuf, bufferLen-1);
				i++;
				break;
			case 0xfa:
				switch (txtPtr[i+1]) {
					case 0:
						sprintf(strBuf, "<p>");
						break;
					case 1:
						sprintf(strBuf, "</p>");
						break;
					default:
						sprintf(strBuf, "[0xfa][0x%02x]", txtPtr[i+1]);
						break;
				}
				strncat(buffer, strBuf, bufferLen-1);
				i++;
				break;
			case 0xfb:
				/* Yes/No question */
				strncat(buffer, "[Yes/No]", bufferLen-1);
				break;
			case 0xfc:
				/* Carriage return */
				strncat(buffer, "<br>", bufferLen-1);
				break;
			case 0xfd:
				/* Wait player input */
				sprintf(strBuf, "[0xfd][0x%02x]", txtPtr[i+1]);
				strncat(buffer, strBuf, bufferLen-1);
				i++;
				break;
			default:
				if (txtPtr[i]<0x60) {
					sprintf(strBuf, "%c", txt2asc[txtPtr[i]]);
				} else if (txtPtr[i]==0x79) {
					sprintf(strBuf, ".");
				} else {
					sprintf(strBuf, "[0x%02x]", txtPtr[i]);
				}
				strncat(buffer, strBuf, bufferLen-1);
				break;
		}
		i++;
	}
}
