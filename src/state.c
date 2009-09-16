/*
	Game state

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

#include <stdlib.h>

#include <SDL.h>
#include <physfs.h>

#include "state.h"
#include "parameters.h"
#include "video.h"

/*--- Types ---*/

typedef struct {
	int version;	/* Num of version */
	char *filename;	/* Filename to detect it */
	char *name;	/* Name of version */
} game_detect_t;

/*--- Constants ---*/

const game_detect_t game_detect[]={
	{GAME_RE3_PC_GAME, "rofs2.dat", "Resident Evil 3: Nemesis, PC"},
	{GAME_RE3_PC_DEMO, "rofs1.dat", "Resident Evil 3: Nemesis Preview, PC"},

	{GAME_RE2_PC_GAME_LEON, "PL0/ZMOVIE/R108L.BIN", "Resident Evil 2 [LEON], PC"},
	{GAME_RE2_PC_GAME_CLAIRE, "PL1/ZMOVIE/R108C.BIN", "Resident Evil 2 [CLAIRE], PC"},
	{GAME_RE2_PC_DEMO_P, "Regist/LeonP.exe", "Resident Evil 2 Preview, PC"},
	{GAME_RE2_PC_DEMO_U, "regist/leonu.exe", "Resident Evil 2 Preview, PC"},

	{GAME_RE1_PC_GAME, "horr/usa/data/capcom.ptc", "Resident Evil, PC"},

	{GAME_RE3_PS1_GAME, "sles_025.28", "Resident Evil 3: Nemesis (UK), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.29", "Resident Evil 3: Nemesis (UK), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.30", "Resident Evil 3: Nemesis (FR), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.31", "Resident Evil 3: Nemesis (DE), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.32", "Resident Evil 3: Nemesis (ES), PS1"},
	{GAME_RE3_PS1_GAME, "sles_025.33", "Resident Evil 3: Nemesis (IT), PS1"},
	{GAME_RE3_PS1_GAME, "sles_026.98", "Resident Evil 3: Nemesis (UK), PS1"},
	{GAME_RE3_PS1_GAME, "slps_023.00", "BioHazard 3: Last Escape (JP), PS1"},
	{GAME_RE3_PS1_GAME, "slus_009.23", "Resident Evil 3: Nemesis (US), PS1"},
	{GAME_RE3_PS1_GAME, "slus_900.64", "Resident Evil 3: Nemesis Trial Edition (US), PS1"},

	{GAME_RE2_PS1_DEMO, "sced_003.60", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_DEMO, "sced_008.27", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_DEMO, "sled_009.77", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_DEMO, "slps_009.99", "BioHazard 2 Trial Edition (JP), PS1"},
	{GAME_RE2_PS1_DEMO, "slus_900.09", "Resident Evil 2 Preview (US), PS1"},
	{GAME_RE2_PS1_DEMO2, "sced_011.14", "Resident Evil 2 Preview (UK), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.72", "Resident Evil 2 [LEON] (UK), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.73", "Resident Evil 2 [LEON] (FR), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.74", "Resident Evil 2 [LEON] (DE), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "sles_009.75", "Resident Evil 2 [LEON] (IT), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slps_012.22", "BioHazard 2 [LEON] (JP), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slps_015.10", "BioHazard 2 Dual Shock [LEON] (JP), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slus_004.21", "Resident Evil 2 [LEON] (US), PS1"},
	{GAME_RE2_PS1_GAME_LEON, "slus_007.48", "Resident Evil 2 Dual Shock [LEON] (US), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.72", "Resident Evil 2 [CLAIRE] (UK), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.73", "Resident Evil 2 [CLAIRE] (FR), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.74", "Resident Evil 2 [CLAIRE] (DE), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "sles_109.75", "Resident Evil 2 [CLAIRE] (IT), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slps_012.23", "BioHazard 2 [CLAIRE] (JP), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slps_015.11", "BioHazard 2 Dual Shock [CLAIRE] (JP), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slus_005.92", "Resident Evil 2 [CLAIRE] (US), PS1"},
	{GAME_RE2_PS1_GAME_CLAIRE, "slus_007.56", "Resident Evil 2 Dual Shock [CLAIRE] (US), PS1"},

	{GAME_RE1_PS1_DEMO, "slpm_800.27", "BioHazard Trial Edition (JP), PS1"},
	{GAME_RE1_PS1_GAME, "ntsc.exe", "BioHazard 1.0 (JP), PS1"},
	{GAME_RE1_PS1_GAME, "sles_002.00", "Resident Evil (UK), PS1"},
	{GAME_RE1_PS1_GAME, "sles_002.27", "Resident Evil (FR), PS1"},
	{GAME_RE1_PS1_GAME, "sles_002.28", "Resident Evil (DE), PS1"},
	{GAME_RE1_PS1_GAME, "sles_009.69", "Resident Evil Director's Cut (UK), PS1"},
	{GAME_RE1_PS1_GAME, "sles_009.70", "Resident Evil Director's Cut (FR), PS1"},
	{GAME_RE1_PS1_GAME, "sles_009.71", "Resident Evil Director's Cut (DE), PS1"},
	{GAME_RE1_PS1_GAME, "slpm_867.70", "BioHazard 5th Anniversary LE (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slps_002.22", "BioHazard (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slps_009.98", "BioHazard Director's Cut (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slps_015.12", "BioHazard Director's Cut Dual Shock (JP), PS1"},
	{GAME_RE1_PS1_GAME, "slus_001.70", "Resident Evil (US), PS1"},
	{GAME_RE1_PS1_GAME, "slus_005.51", "Resident Evil Director's Cut (US), PS1"},
	{GAME_RE1_PS1_SHOCK, "slus_007.47", "Resident Evil Director's Cut Dual Shock (US), PS1"},

	{-1, "", ""}
};

/*--- Variables ---*/

state_t game_state;
static int num_game_detected = -1;

/*--- Functions prototypes ---*/

static void state_detect(void);
static void state_shutdown(void);

static void state_loadbackground(void);
static void state_unloadbackground(void);

static void state_loadroom(void);
static void state_unloadroom(void);

static model_t *state_loadmodel(int num_model);
static void state_download_textures(void);

static void state_unloadmodels(void);

/*--- Functions ---*/

void state_init(void)
{
	memset(&game_state, 0, sizeof(state_t));

	game_state.num_stage = 1;
	game_state.num_room = 0;
	game_state.num_camera = 0;

	game_state.movies_list = NULL;
	game_state.num_movie = 0;
	/*memset(game_state.cur_movie, 0, sizeof(game_state.cur_movie));*/
	game_state.cur_movie = NULL;

	state_detect();

	game_state.load_room = state_loadroom;
	game_state.load_background = state_loadbackground;
	game_state.load_model = state_loadmodel;
	game_state.download_textures = state_download_textures;
	game_state.shutdown = state_shutdown;
}

static void state_shutdown(void)
{
	state_unloadmodels();
	state_unloadbackground();
	state_unloadroom();

	if (game_state.priv_shutdown) {
		(*game_state.priv_shutdown)();
	}
}

const char *state_getGameName(void)
{
	if (num_game_detected!=-1) {
		return game_detect[num_game_detected].name;
	}

	return "Unknown version";
}

void state_setstage(int new_stage)
{
	game_state.num_stage = new_stage;
}

void state_setroom(int new_room)
{
	game_state.num_room = new_room;
}

void state_setcamera(int new_camera)
{
	game_state.num_camera = new_camera;
}

static void state_loadbackground(void)
{
	state_unloadbackground();

	if (game_state.priv_load_background) {
		(*game_state.priv_load_background)();
	}
}

void state_unloadbackground(void)
{
	if (game_state.back_surf) {
		video.destroySurface(game_state.back_surf);
		game_state.back_surf = NULL;
	}
}

static void state_loadroom(void)
{
	state_unloadroom();

	if (game_state.priv_load_room) {
		(*game_state.priv_load_room)();

		if (game_state.room) {
			room_map_init(game_state.room);
			/* Dump scripts if wanted */
			if (params.dump_script) {
				game_state.room->scriptDump(game_state.room, ROOM_SCRIPT_INIT);
				game_state.room->scriptDump(game_state.room, ROOM_SCRIPT_RUN);
			}
			game_state.room->scriptExec(game_state.room, ROOM_SCRIPT_INIT);
		}
	}
}

static void state_unloadroom(void)
{
	if (game_state.room) {
		game_state.room->shutdown(game_state.room);
		game_state.room = NULL;
	}
}

model_t *state_loadmodel(int num_model)
{
	int i;

	/* Search if model already loaded */
	for (i=0; i<game_state.model_list_count; i++) {
		if (game_state.model_list[i].num_model == num_model) {
			return game_state.model_list[i].model;
		}
	}

	/* Model not in list, reallocate list, and add it at the end */
	if (!game_state.priv_load_model) {
		return NULL;
	}

	++game_state.model_list_count;

	game_state.model_list = (model_item_t *) realloc(game_state.model_list, game_state.model_list_count*sizeof(model_item_t));

	game_state.model_list[game_state.model_list_count-1].num_model = num_model;
	game_state.model_list[game_state.model_list_count-1].model = game_state.priv_load_model(num_model);

	return game_state.model_list[game_state.model_list_count-1].model;
}

/* Force reupload textures */
static void state_download_textures(void)
{
	int i;

	for (i=0; i<game_state.model_list_count; i++) {
		model_t *model = game_state.model_list[i].model;
		if (model) {
			render_texture_t *texture = model->texture;
			if (texture) {
				texture->download(texture);
			}
		}
	}
}

static void state_unloadmodels(void)
{
	int i;

	for (i=0; i<game_state.model_list_count; i++) {
		model_t *model = game_state.model_list[i].model;
		if (model) {
			model->shutdown(model);
			game_state.model_list[i].num_model = -1;
			game_state.model_list[i].model = NULL;
		}
	}
	if (game_state.model_list) {
		free(game_state.model_list);
		game_state.model_list = NULL;
		game_state.model_list_count = 0;
	}
}

void state_newmovie(void)
{
	char **movie = game_state.movies_list;
	int i;

	for (i=0; movie[i]; i++) {
		if (i==game_state.num_movie) {
			/*sprintf(game_state.cur_movie, "%s/%s", params.basedir, movie[i]);*/
			game_state.cur_movie = movie[i];
			break;
		}
	}
}

int state_getnummovies(void)
{
	char **movie = game_state.movies_list;
	int i;

	for (i=0; movie[i]; i++) {
	}
	return i;
}

/* Detect some game version */

int state_game_file_exists(char *filename)
{
	char *filenamedir;
	int detected = 0;
	
	filenamedir = malloc(strlen(params.basedir)+strlen(filename)+4);
	if (filenamedir) {
		PHYSFS_file	*curfile;

		sprintf(filenamedir, "%s/%s", params.basedir, filename);

		curfile = PHYSFS_openRead(filename);
		if (curfile) {
			char dummy;

			if (PHYSFS_read(curfile, &dummy, 1, 1)>0) {
				detected = 1;
			}

			PHYSFS_close(curfile);
		}

		free(filenamedir);
	}

	return detected;
}

static void state_detect(void)
{
	int i=0;

	game_state.version = GAME_UNKNOWN;

	while (game_detect[i].version != -1) {
		if (state_game_file_exists(game_detect[i].filename)) {
			num_game_detected = i;
			game_state.version = game_detect[i].version;
			break;
		}
		i++;
	}
}
