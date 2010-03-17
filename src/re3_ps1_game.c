/*
	RE3
	PS1
	Game

	Copyright (C) 2007	Patrice Mandin

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

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "filesystem.h"
#include "state.h"
#include "re3_ps1_game.h"
#include "background_bss.h"
#include "parameters.h"
#include "log.h"
#include "room_rdt2.h"
#include "render.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Types ---*/

typedef struct {
	Uint32 length;
	Uint32 count;
} ard_header_t;

typedef struct {
	Uint32 length;
	Uint32 unknown;
} ard_object_t;

/*--- Constant ---*/

static const char *re3ps1game_bg = "cd_data/stage%d/r%d%02x.bss";
static const char *re3ps1game_room = "cd_data/stage%d/r%d%02x.ard";
static const char *re3ps1game_font = "cd_data/etc/sele_ob%c.tim";

static const char *re3ps1game_movies[] = {
	"cd_data/zmovie/enda.str",
	"cd_data/zmovie/endb.str",
	"cd_data/zmovie/ins01.str",
	"cd_data/zmovie/ins02.str",
	"cd_data/zmovie/ins03.str",
	"cd_data/zmovie/ins04.str",
	"cd_data/zmovie/ins05.str",
	"cd_data/zmovie/ins06.str",
	"cd_data/zmovie/ins07.str",
	"cd_data/zmovie/ins08.str",
	"cd_data/zmovie/ins09.str",
	"cd_data/zmovie/opn.str",
	"cd_data/zmovie/roopne.str",
	NULL
};

/*
	offset Hex	offset Dec	name		offset Hex
	(bytes)		(2352 sectors)			(storage of sector number)

slus_00923
offset table at 131b2db0 (sector 136287) ?

	00364380	1512		em19.tim
	01ca30d0	12767		em5b.tim	0252e460
	02e95f30	20769		em10.tim	0db64540
	02eb2160	20818		em10.emd	13a01850
	02edadb0	20889		em54.tim	2566ea50
	02eedce0	20922		em54.emd
	02f027a0	20958		em59.tim	01fba5e0	05d1d430
	02f156d0	20991		em59.emd	131bb790
	031cdf40	22204		em15.emd
	03214950	22327		em1c.tim	25ac4a16
	03230b80	22376		em1c.emd	0426dab0
					em16.tim
	033af260	23042		em16.emd	2877b5cf
	033eee30	23153		em1e.tim	09fe3019
	03414c90	23219		em1e.emd	2674c4d0
	03682aa0	24302		em53.tim	0196c0fa
	036959d0	24335		em53.emd	02e3ff20
	038a9570	25261		em1f.tim	0be6aeaa
	038e19d0	25359		em1f.emd	0bbcfd60
	03de21f0	27589		em11.tim	020b3460
	03df5120	27622		em11.emd	047b4180
	03e3a8d0	27743		em14.tim
	03e4d800	27776		em14.emd	04cd05a0
	03e92fb0	27897		em15.tim	0469927c	07522e86
	03eea430	28049		em34.tim
	03f06660	28098		em34.emd	07c71cf0
	04103280	28984		em1d.tim
	041323e0	29066		em1d.emd	16a03840
	042aef30	29729		em20.tim
	042c1e60	29762		em20.emd	07021f80
	046565a0	31358		em23.tim
	046727d0	31407		em23.emd
	046c80c0	31556		em28.tim
	046daff0	31589		em28.emd
	049889d0	32783		em2d.tim
	04992600	32800		em2d.emd	03464180
	04e40320	34886		em58.tim
	04e53250	34919		em58.emd	23d55bb0
	051ecc40	36524		em2f.tim
	05208e70	36573		em2f.emd	07104130
	0563fdc0	38452		em13.emd	0469927c
							04701bc4
							07522e86
	05d90630	41713		em12.tim	131b9200
	05da3560	41746		em12.emd	05d06740
	05de8d10	41867		em17.tim	0c562420
	05e04f40	41916		em17.emd	020ac310
	063df170	44525		em52.tim
	063f20a0	44558		em52.emd
	06407490	44595		em55.tim
	0641a3c0	44628		em55.emd
	0642ee80	44664		em5a.tim
	06441db0	44697		em5a.emd
	06456870	44733		em5c.tim
	064697a0	44766		em5c.emd
	06f92ce0	49742		em21.tim
	06f9c8d0	49759		em21.emd
	08763450	60359		em2c.tim
	08776380	60392		em2c.emd
	0899f310	61355		em32.tim	0167a330
	089a8f40	61372		em32.emd
	08d38d00	62960		em19.emd
	08e7f8a0	63528		em27.tim
	08e894e0	63546		em27.emd
					em50.tim
	097d6970	67693		em50.emd
					em1a.tim
	0a1affc0	72084		em1a.emd
	0a5cb610	73915		em33.tim
	0a5f1470	73981		em33.emd	026334e0
	0a9a6750	75633		em51.tim
	0a9b9690	75667		em51.emd
	0aca22c0	76964		em5b.emd
					em70.tim
	0aca22c0	77409		em70.emd
					em71.tim
	0acca5e0	77034		em71.emd
	01e5a1c4			em62.tim (no 2352 sectors)
	0acca5e0	77034		em62.emd
	01ea9a38			em63.tim (no 2352 sectors)
	0acf2900	77104		em63.emd
	01ef939c			em64.tim (no 2352 sectors)
	0ad1ac20	77174		em64.emd
	01f488e0			em65.tim (no 2352 sectors)
	0ad42f40	77244		em65.emd
	01f98598			em66.tim (no 2352 sectors)
	0ad6b260	77314		em66.emd
	0c0c0e90	85933		em3a.tim
	0c0dd0d0	85983		em3a.emd
	0f2b02c0	108196		em26.tim
	0f2b9ef0	108213		em26.emd
					em36.tim
	0f3561f0	108485		em36.emd
	0f38cac0	108580		em37.tim
	0f3966f0	108597		em37.emd
	0f53b560	109330		em25.tim
	0f54e490	109363		em25.emd
	0faa8f20	111750		em40.tim
	0fab2b50	111767		em40.emd
	0ff1c320	113734		em35.tim
	0ff4b480	113816		em35.emd
					em3b.tim
	0ff84b40	113916		em3b.emd
	01e0a990			em5f.tim (no 2352 sectors)
	10769f40	117436		em5f.emd
	115ec930	123905		em13.tim	01f1bb40
							0298ce20
	1291d9a0	132461		em1b.tim
	12955dd0	132559		em1b.emd
	12bc0df0	133637		em22.tim
	13490fa0	137566		em56.tim
	134a3ed0	137599		em56.emd
	13a67ab0	140169		em24.tim
	13a7a9e0	140202		em24.emd
	13d92770	141581		em22.emd
	14a60ad0	147289		em30.tim
	14a86940	147356		em30.emd
	157021b0	152921		em18.tim	009b65e0
	15728010	152987		em18.emd
	1677e0a0	160270		em60.tim
	16790fd0	160303		em60.emd
	1861e6c0	173924		em61.tim
	186315f0	173957		em61.emd
	16c104b0	162307		em3f.tim
	16c233f0	162341		em3f.emd
	1715f790	164675		em38.tim
	171855f0	164741		em38.emd
	171d2e30	164875		em39.tim
	171dca70	164893		em39.emd
	174a9470	166141		em3e.tim
	174bc3a0	166174		em3e.emd
	18645780	173992		em67.tim
	186586b0	174025		em67.emd
					em57.tim
	1ce9b730	206241		em57.emd

					em2e.tim
					em2e.emd
					em5d.tim
					em5d.emd
					em5e.tim
					em5e.emd
*/

/*--- Variables ---*/

static int game_lang = 'u';

/*--- Functions prototypes ---*/

static void re3ps1game_shutdown(void);

static void re3ps1game_loadbackground(void);

static void re3ps1game_loadroom(void);
static int re3ps1game_loadroom_ard(const char *filename);

static void load_font(void);
static void get_char(int ascii, int *x, int *y, int *w, int *h);

/*--- Functions ---*/

void re3ps1game_init(state_t *game_state)
{
	game_state->priv_load_background = re3ps1game_loadbackground;
	game_state->priv_load_room = re3ps1game_loadroom;
	game_state->priv_shutdown = re3ps1game_shutdown;

	game_state->movies_list = (char **) re3ps1game_movies;

	if (state_game_file_exists("cd_data/etc/sele_obf.tim")) {
		game_lang = 'f';
	}

	game_state->load_font = load_font;
	game_state->get_char = get_char;
}

static void re3ps1game_shutdown(void)
{
}

static void re3ps1game_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_bg, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "bss: Start loading %s ...\n", filepath);

	logMsg(1, "bss: %s loading %s ...\n",
		background_bss_load(filepath, CHUNK_SIZE, 0) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static void re3ps1game_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_room, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "ard: Start loading %s ...\n", filepath);

	logMsg(1, "ard: %s loading %s ...\n",
		re3ps1game_loadroom_ard(filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int re3ps1game_loadroom_ard(const char *filename)
{
	PHYSFS_sint64 length;
	Uint8 *ard_file;
	ard_object_t *ard_object;
	int i, count;
	Uint32 offset, len;
	void *file;

	ard_file = (Uint8 *) FS_Load(filename, &length);
	if (!ard_file) {
		return 0;
	}

	count = ((ard_header_t *) ard_file)->count;
	count = SDL_SwapLE32(count);
/*
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		logMsg(1, "ard: object %d at offset 0x%08x\n", i,offset);
		len = SDL_SwapLE32(ard_object->length);

		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}
*/
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		len = SDL_SwapLE32(ard_object->length);
		if (i==8) {
			/* Stop on embedded RDT file */
			break;
		}
		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}

	file = malloc(len);
	if (!file) {
		free(ard_file);
		return 0;
	}

	logMsg(3, "ard: Loading embedded RDT file from offset 0x%08x\n", offset);
	memcpy(file, &ard_file[offset], len);

	game_state.room = room_create(file, len);
	if (!game_state.room) {
		free(file);
		free(ard_file);
		return 0;
	}

	room_rdt2_init(game_state.room);

	free(ard_file);
	return 1;
}

static void load_font(void)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
	const char *filename = re3ps1game_font;

	filepath = malloc(strlen(filename)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, filename, game_lang);

	logMsg(1, "Loading font from %s...\n", filepath);

	font_file = FS_Load(filepath, &length);
	if (font_file) {
		game_state.font = render.createTexture(0);
		if (game_state.font) {
			game_state.font->load_from_tim(game_state.font, font_file);
			retval = 1;
		}

		free(font_file);
	}

	logMsg(1, "Loading font from %s... %s\n", filepath, retval ? "Done" : "Failed");

	free(filepath);
}

static void get_char(int ascii, int *x, int *y, int *w, int *h)
{
	*x = *y = 0;
	*w = 8;
	*h = 10;

	if ((ascii<=32) || (ascii>=96+27)) {
		return;
	}

	ascii -= 32;
	*x = 128+ ((ascii & 15)<<3);
	*y = 176+ ((ascii>>4)*10);
}
