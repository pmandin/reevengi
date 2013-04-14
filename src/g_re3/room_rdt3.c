/*
	Room description
	RE3 RDT manager

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

#include "room.h"
#include "state.h"
#include "room_rdt3.h"
#include "room_rdt3_script.h"
#include "log.h"
#include "video.h"
#include "render.h"
#include "parameters.h"

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
	Uint16 unk0;
	Uint16 const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	Sint32 camera_from_x;
	Sint32 camera_from_y;
	Sint32 camera_from_z;
	Sint32 camera_to_x;
	Sint32 camera_to_y;
	Sint32 camera_to_z;
	Uint32 masks_offset;
} rdt_camera_pos_t;

typedef struct {
	Uint16 const0; /* 0xff01 */
	Uint8 from,to;
	Sint16 x1,y1; /* Coordinates to use to calc when player crosses switch zone */
	Sint16 x2,y2;
	Sint16 x3,y3;
	Sint16 x4,y4;
} rdt_camera_switch_t;

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

typedef struct {
	Uint16 num_offset;
	Uint16 num_masks;
} rdt_mask_header_t;

typedef struct {
	Uint16 count;
	Uint16 unknown;
	Uint16 dst_x, dst_y;
} rdt_mask_offset_t;

typedef struct {
	Uint8 src_x, src_y;
	Uint8 dst_x, dst_y;
	Uint16 depth, size;
} rdt_mask_square_t;

typedef struct {
	Uint8 src_x, src_y;
	Uint8 dst_x, dst_y;
	Uint16 depth, zero;
	Uint16 width, height;
} rdt_mask_rect_t;

/*--- Functions prototypes ---*/

static void rdt3_getCamera(room_t *this, int num_camera, room_camera_t *room_camera);

static int rdt3_getNumCamswitches(room_t *this);
static void rdt3_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch);

static int rdt3_getNumBoundaries(room_t *this);
static void rdt3_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_camswitch);

static void rdt3_displayTexts(room_t *this, int num_lang);

static void rdt3_initMasks(room_t *this, int num_camera);
static void rdt3_drawMasks(room_t *this, int num_camera);

/*--- Functions ---*/

void room_rdt3_init(room_t *this)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;

	if (this->file_length > 4) {
		this->num_cameras = rdt_header->num_cameras;
		this->num_camswitches = rdt3_getNumCamswitches(this);
		this->num_boundaries = rdt3_getNumBoundaries(this);

		this->getCamera = rdt3_getCamera;
		this->getCamswitch = rdt3_getCamswitch;
		this->getBoundary = rdt3_getBoundary;

		this->drawMasks = rdt3_drawMasks;

		room_rdt3_scriptInit(this);
	}

	logMsg(2, "%d cameras angles, %d camera switches, %d boundaries\n",
		this->num_cameras, this->num_camswitches, this->num_boundaries);

	/* Display texts */
	rdt3_displayTexts(this, 0);	/* language 1 */
	rdt3_displayTexts(this, 1);	/* language 2 */
}

static void rdt3_getCamera(room_t *this, int num_camera, room_camera_t *room_camera)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	Uint32 offset;
	rdt_camera_pos_t *cam_array;
	
	offset = SDL_SwapLE32(rdt_header->offsets[RDT3_OFFSET_CAMERAS]);
	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[offset];

	room_camera->from_x = SDL_SwapLE32(cam_array[num_camera].camera_from_x);
	room_camera->from_y = SDL_SwapLE32(cam_array[num_camera].camera_from_y);
	room_camera->from_z = SDL_SwapLE32(cam_array[num_camera].camera_from_z);
	room_camera->to_x = SDL_SwapLE32(cam_array[num_camera].camera_to_x);
	room_camera->to_y = SDL_SwapLE32(cam_array[num_camera].camera_to_y);
	room_camera->to_z = SDL_SwapLE32(cam_array[num_camera].camera_to_z);
}

static int rdt3_getNumCamswitches(room_t *this)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	Uint32 offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_switches = 0, prev_from = -1;

	offset = SDL_SwapLE32(rdt_header->offsets[RDT3_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary=0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			/* boundary, not a switch */
		} else {
			num_switches++;
		}

		++i;
	}

	return num_switches;
}

static void rdt3_getCamswitch(room_t *this, int num_camswitch, room_camswitch_t *room_camswitch)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	Uint32 offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	offset = SDL_SwapLE32(rdt_header->offsets[RDT3_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary = 0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			/* boundary, not a switch */
		} else {
			if (j==num_camswitch) {
				break;
			}

			++j;
		}

		++i;
	}

	room_camswitch->from = camswitch_array[i].from;
	room_camswitch->to = camswitch_array[i].to;
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}

static int rdt3_getNumBoundaries(room_t *this)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	Uint32 offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, num_boundaries = 0, prev_from = -1;

	offset = SDL_SwapLE32(rdt_header->offsets[RDT3_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary=0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			++num_boundaries;
		} else {
			/* switch, not a boundary */
		}

		++i;
	}

	return num_boundaries;
}

static void rdt3_getBoundary(room_t *this, int num_boundary, room_camswitch_t *room_camswitch)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	Uint32 offset;
	rdt_camera_switch_t *camswitch_array;
	int i=0, j=0, prev_from=-1;

	offset = SDL_SwapLE32(rdt_header->offsets[RDT3_OFFSET_CAM_SWITCHES]);
	camswitch_array = (rdt_camera_switch_t *) &((Uint8 *) this->file)[offset];

	while (SDL_SwapLE16(camswitch_array[i].const0) != 0xffff) {
		int boundary = 0;

		if (prev_from != camswitch_array[i].from) {
			prev_from = camswitch_array[i].from;
			boundary = 1;
		}
		if (boundary && (camswitch_array[i].to==0)) {
			if (j==num_boundary) {
				break;
			}

			++j;
		} else {
			/* switch, not a boundary */
		}

		++i;
	}

	room_camswitch->from = camswitch_array[i].from;
	room_camswitch->to = camswitch_array[i].to;
	room_camswitch->x[0] = SDL_SwapLE16(camswitch_array[i].x1);
	room_camswitch->y[0] = SDL_SwapLE16(camswitch_array[i].y1);
	room_camswitch->x[1] = SDL_SwapLE16(camswitch_array[i].x2);
	room_camswitch->y[1] = SDL_SwapLE16(camswitch_array[i].y2);
	room_camswitch->x[2] = SDL_SwapLE16(camswitch_array[i].x3);
	room_camswitch->y[2] = SDL_SwapLE16(camswitch_array[i].y3);
	room_camswitch->x[3] = SDL_SwapLE16(camswitch_array[i].x4);
	room_camswitch->y[3] = SDL_SwapLE16(camswitch_array[i].y4);
}

static void rdt3_displayTexts(room_t *this, int num_lang)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	int room_lang = (num_lang==0) ? RDT3_OFFSET_TEXT_LANG1 : RDT3_OFFSET_TEXT_LANG2;
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
		room_rdt3_getText(this, num_lang, i, tmpBuf, sizeof(tmpBuf));
		logMsg(1, " Text[0x%02x]: %s\n", i, tmpBuf);
	}
}

void room_rdt3_getText(room_t *this, int lang, int num_text, char *buffer, int bufferLen)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	int room_lang = (lang==0) ? RDT3_OFFSET_TEXT_LANG1 : RDT3_OFFSET_TEXT_LANG2;
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

static void rdt3_initMasks(room_t *this, int num_camera)
{
	rdt3_header_t *rdt_header = (rdt3_header_t *) this->file;
	Uint32 offset;
	rdt_camera_pos_t *cam_array;
	rdt_mask_header_t *mask_hdr;
	rdt_mask_offset_t *mask_offsets;
	int num_offset, count_offsets;
	render_mask_t *rdr_mask;

	if (num_camera>=this->num_cameras) {
		return;
	}

	if (game_state.bg_mask==NULL) {
		return;
	}

	offset = SDL_SwapLE32(rdt_header->offsets[RDT3_OFFSET_CAMERAS]);
	cam_array = (rdt_camera_pos_t *) &((Uint8 *) this->file)[offset];

	offset = SDL_SwapLE32(cam_array[num_camera].masks_offset);
	if (offset == 0xffffffffUL) {
		return;
	}

	game_state.rdr_mask = render.render_mask_create(game_state.bg_mask);
	if (!game_state.rdr_mask) {
		return;
	}
	rdr_mask = game_state.rdr_mask;

	mask_hdr = (rdt_mask_header_t *) &((Uint8 *) this->file)[offset];
	offset += sizeof(rdt_mask_header_t);

	mask_offsets = (rdt_mask_offset_t *) &((Uint8 *) this->file)[offset];
	count_offsets = (Sint16) SDL_SwapLE16(mask_hdr->num_offset);
	if (count_offsets < 0) {
		return;
	}

	offset += sizeof(rdt_mask_offset_t) * count_offsets;

	for (num_offset=0; num_offset<SDL_SwapLE16(mask_hdr->num_offset); num_offset++) {
		int num_mask;
		
		for (num_mask=0; num_mask<SDL_SwapLE16(mask_offsets->count); num_mask++) {
			rdt_mask_square_t *square_mask;
			int src_x, src_y, width, height, depth;
			int dst_x = SDL_SwapLE16(mask_offsets->dst_x);
			int dst_y = SDL_SwapLE16(mask_offsets->dst_y);

			square_mask = (rdt_mask_square_t *) &((Uint8 *) this->file)[offset];
			if (square_mask->size == 0) {
				/* Rect mask */
				rdt_mask_rect_t *rect_mask = (rdt_mask_rect_t *) square_mask;

				src_x = rect_mask->src_x;
				src_y = rect_mask->src_y;
				dst_x += rect_mask->dst_x;
				dst_y += rect_mask->dst_y;
				width = SDL_SwapLE16(rect_mask->width);
				height = SDL_SwapLE16(rect_mask->height);
				depth = SDL_SwapLE16(rect_mask->depth);

				offset += sizeof(rdt_mask_rect_t);
			} else {
				/* Square mask */

				src_x = square_mask->src_x;
				src_y = square_mask->src_y;
				dst_x += square_mask->dst_x;
				dst_y += square_mask->dst_y;
				width = height = SDL_SwapLE16(square_mask->size);
				depth = SDL_SwapLE16(square_mask->depth);

				offset += sizeof(rdt_mask_square_t);
			}

			rdr_mask->addZone(rdr_mask,
				src_x,src_y, width,height,
				dst_x,dst_y, 32*depth);
		}

		mask_offsets++;
	}

	rdr_mask->finishedZones(rdr_mask);
}

static void rdt3_drawMasks(room_t *this, int num_camera)
{
	render_mask_t *rdr_mask;

	if (!game_state.rdr_mask) {
		rdt3_initMasks(this, num_camera);
	}
	rdr_mask = game_state.rdr_mask;
	if (!rdr_mask) {
		return;
	}

	rdr_mask->drawMask(rdr_mask);
}
