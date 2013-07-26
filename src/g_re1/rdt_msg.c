/*
	RE1 MSG
	Text messages

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

#include "../g_common/room.h"

#include "rdt.h"
#include "rdt_msg.h"

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

/*--- Functions prototypes ---*/

static int getTextBlockLength(room_t *this);

/*--- Functions ---*/

void rdt1_msg_getText(room_t *this, int lang, int num_text, char *buffer, int bufferLen)
{
	rdt1_header_t *rdt_header;
	int room_lang = RDT1_OFFSET_TEXT /*(lang==0) ? RDT2_OFFSET_TEXT_LANG1 : RDT2_OFFSET_TEXT_LANG2*/;
	Uint32 offset;
	Uint16 *txtOffsets, txtCount;
	Uint8 *txtPtr;
	int i = 0, txtLen;
	char strBuf[32];

	memset(buffer, 0, sizeof(bufferLen));

	rdt_header = (rdt1_header_t *) this->file;
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

	txtLen=getTextBlockLength(this)
	   - SDL_SwapLE16(txtOffsets[num_text]);
	if (num_text<txtCount-1) {
		txtLen = SDL_SwapLE16(txtOffsets[num_text+1])
			- SDL_SwapLE16(txtOffsets[num_text]);
	}

/*	sprintf(strBuf, "(0x%08x %d) ", offset + SDL_SwapLE16(txtOffsets[num_text]), txtLen);
	strncat(buffer, strBuf, bufferLen-1);*/

/*	while ((txtPtr[i] != 0xfe) && (i<bufferLen-1)) {*/
	while ((i<txtLen) && (i<bufferLen-1)) {
#if 0
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
#else
				if (/*(txtPtr[i]>0x04) &&*/ (txtPtr[i]<0x60)) {
					sprintf(strBuf, "%c", txt2asc[txtPtr[i]]);
				} else {		
					sprintf(strBuf, "[0x%02x]", txtPtr[i]);
				}
				strncat(buffer, strBuf, bufferLen-1);
#endif
		i++;
	}

}

/* Find length of block message in RDT file, by finding the smallest offset
   after the MSG block */

static int getTextBlockLength(room_t *this)
{
	rdt1_header_t *rdt_header;
	int i;
	Uint32 offset, cur_offset, next_offset = this->file_length;

	rdt_header = (rdt1_header_t *) this->file;
	offset = SDL_SwapLE32(rdt_header->offsets[RDT1_OFFSET_TEXT]);

	for (i=0; i<19; i++) {
		cur_offset = SDL_SwapLE32(rdt_header->offsets[i]);
		if (cur_offset<=offset) {
			continue;
		}
		if (cur_offset<next_offset) {
			next_offset=cur_offset;
		}
	}

	return next_offset-offset;
}
