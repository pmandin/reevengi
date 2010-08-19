/*
	Command line parameters

	Copyright (C) 2003-2010	Patrice Mandin
	
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

/*--- Includes ---*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parameters.h"

/*--- Defines ---*/

#define DEFAULT_BASEDIR "."
#define DEFAULT_VERBOSE 0
#define DEFAULT_GAMMA 1.0f
#define DEFAULT_USE_OPENGL 0
#define DEFAULT_ASPECT_X 4
#define DEFAULT_ASPECT_Y 3
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240
#define DEFAULT_BPP 16
#define DEFAULT_STAGE 1
#define DEFAULT_ROOM 0
#define DEFAULT_CAMERA 0

/*--- Global variables ---*/

params_t params = {
	.verbose = DEFAULT_VERBOSE,
	.log_file = PACKAGE_NAME ".log",
	.basedir = DEFAULT_BASEDIR,
	.gamma = DEFAULT_GAMMA,
	.viewmode = VIEWMODE_BACKGROUND,
	.use_opengl = DEFAULT_USE_OPENGL,
	.aspect_x = DEFAULT_ASPECT_X,
	.aspect_y = DEFAULT_ASPECT_Y,
	.dithering = 0,
	.linear = 0,
	.dump_script = 0,
	.width = 0,
	.height = 0,
	.bpp = 0,
	.fps = 0,
	.stage = DEFAULT_STAGE,
	.room = DEFAULT_ROOM,
	.camera = DEFAULT_CAMERA
};

/*---- Variables ---*/

/*--- Functions ---*/

static int ParmPresent(char *param, int argc, char **argv)
{
	int	i;

	for (i=1; i<argc; i++)
		if ( !strcmp(param, argv[i]) )
			return i;

	return 0;
}

int CheckParm(int argc,char **argv)
{
	int p;

	/*--- Check for help ---*/
	p = ParmPresent("-help", argc, argv);
	if (p) {
		return 0;
	}

	/*--- Check for verbose mode ---*/
	p = ParmPresent("-verbose", argc, argv);
	if (p && p < argc-1) {
		params.verbose = atoi(argv[p+1]);
	}

	/*--- Check for log filename ---*/
	p = ParmPresent("-logfile", argc, argv);
	if (p && p < argc-1) {
		params.log_file = argv[p+1];
	}

	/*--- Check for gamma ---*/
	p = ParmPresent("-gamma", argc, argv);
	if (p && p < argc-1) {
		params.gamma = atof(argv[p+1]);
	}

	/*--- Check for base directory ---*/
	p = ParmPresent("-basedir", argc, argv);
	if (p && p < argc-1) {
		params.basedir = argv[p+1];
	}

	/*--- Check for movie mode ---*/
	p = ParmPresent("-movie", argc, argv);
	if (p) {
		params.viewmode = VIEWMODE_MOVIE;
	}

	/*--- Check for OpenGL ---*/
	p = ParmPresent("-opengl", argc, argv);
	if (p) {
		params.use_opengl = 1;
	}

	/*--- Check for aspect ratio ---*/
	p = ParmPresent("-aspect", argc, argv);
	if (p && p < argc-1) {
		if (sscanf(argv[p+1], "%d:%d", &params.aspect_x, &params.aspect_y) != 2) {
			/* Reput default values if failed */
			params.aspect_x = DEFAULT_ASPECT_X;
			params.aspect_y = DEFAULT_ASPECT_Y;
		} else {
			params.aspect_user = 1;
		}
	}

	/*--- Check for dithering ---*/
	p = ParmPresent("-dither", argc, argv);
	if (p) {
		params.dithering = 1;
	}

	/*--- Check for dithering ---*/
	p = ParmPresent("-linear", argc, argv);
	if (p) {
		params.linear = 1;
	}

	/*--- Check for video mode ---*/
	p = ParmPresent("-width", argc, argv);
	if (p && p < argc-1) {
		params.width = atoi(argv[p+1]);
	}

	p = ParmPresent("-height", argc, argv);
	if (p) {
		params.height = atoi(argv[p+1]);
	}

	p = ParmPresent("-bpp", argc, argv);
	if (p) {
		params.bpp = atoi(argv[p+1]);
		if (params.bpp<8) {
			params.bpp = 8;
		}
	}

#ifdef ENABLE_SCRIPT_DISASM
	/*--- Check for script dump ---*/
	p = ParmPresent("-dumpscript", argc, argv);
	if (p) {
		params.dump_script = 1;
	}
#endif

	/*--- Check for fps ---*/
	p = ParmPresent("-fps", argc, argv);
	if (p) {
		params.fps = 1;
	}

	/*--- Check for stage/room/camera ---*/
	p = ParmPresent("-stage", argc, argv);
	if (p && p < argc-1) {
		params.stage = atoi(argv[p+1]);
	}

	p = ParmPresent("-room", argc, argv);
	if (p) {
		params.room = atoi(argv[p+1]);
	}

	p = ParmPresent("-camera", argc, argv);
	if (p) {
		params.camera = atoi(argv[p+1]);
	}

	return 1;
}

void DisplayUsage(void)
{
	printf("---- Reevengi slide show ----\n");
	printf("----  by Patrice Mandin  ----\n");
	printf("Usage:\n");
	printf("  [-basedir </path/to/gamedir>] (default=%s)\n", DEFAULT_BASEDIR);
	printf("  [-movie] (switch to movie player mode)\n");
	printf("  [-gamma <n>] (default=%.3f)\n", DEFAULT_GAMMA);
	printf("  [-verbose <n>] (log verbosity, default=%d)\n", DEFAULT_VERBOSE);
	printf("  [-logfile <filename>] (default=%s.log)\n", PACKAGE_NAME);
	printf("  [-opengl] (enable opengl mode)\n");
	printf("  [-aspect <x>:<y>] (set aspect ratio, default=%d:%d)\n", DEFAULT_ASPECT_X, DEFAULT_ASPECT_Y);
	printf("  [-dither] (enable dithering in 8 bits mode)\n");
	printf("  [-linear] (enable bilinear filtering)\n");
	printf("  [-width <w>] (width of video mode, default=%d)\n", DEFAULT_WIDTH);
	printf("  [-height <h>] (height of video mode, default=%d)\n", DEFAULT_HEIGHT);
	printf("  [-bpp <b>] (bits per pixel for video mode, default=%d)\n", DEFAULT_BPP);
	printf("  [-fps] (enable fps display)\n");
	printf("  [-stage <n>] (stage, default=%d)\n", DEFAULT_STAGE);
	printf("  [-room <n>] (room, default=%d)\n", DEFAULT_ROOM);
	printf("  [-camera <n>] (camera, default=%d)\n", DEFAULT_CAMERA);
#ifdef ENABLE_SCRIPT_DISASM
	printf("  [-dumpscript] (enable script dump when loading room)\n");
#endif
	printf("  [-help] (print this message)\n");
}
