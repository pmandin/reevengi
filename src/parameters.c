/*
	Command line parameters

	Copyright (C) 2003	Patrice Mandin
	
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

/*--- Global variables ---*/

params_t params = {
	DEFAULT_VERBOSE,
	PACKAGE_NAME ".log",
	DEFAULT_BASEDIR,
	DEFAULT_GAMMA,
	VIEWMODE_BACKGROUND,
	DEFAULT_USE_OPENGL,
	DEFAULT_ASPECT_X,
	DEFAULT_ASPECT_Y,
	0,
	0
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

#ifdef ENABLE_SCRIPT_DISASM
	/*--- Check for script dump ---*/
	p = ParmPresent("-dumpscript", argc, argv);
	if (p) {
		params.dump_script = 1;
	}
#endif

	return 1;
}

void DisplayUsage(void)
{
	printf("---- Reevengi slide show ----\n");
	printf("----  by Patrice Mandin  ----\n");
	printf("Usage:\n");
	printf( "  [-basedir </path/to/gamedir>] (default=%s)\n"
		"  [-movie] (switch to movie player mode)\n"
		"  [-gamma <n>] (default=%.3f)\n"
		"  [-verbose <n>] (log verbosity, default=%d)\n"
		"  [-logfile <filename>] (default=" PACKAGE_NAME ".log)\n"
		"  [-opengl] (enable opengl mode)\n"
		"  [-aspect <x>:<y>] (set aspect ratio, default=%d:%d)\n"
		"  [-dither] (enable dithering in 8 bits mode)\n"
#ifdef ENABLE_SCRIPT_DISASM
		"  [-dumpscript] (enable script dump when loading room)\n"
#endif
		"  [-help] (print this message)\n",
		DEFAULT_BASEDIR,
		DEFAULT_GAMMA,
		DEFAULT_VERBOSE,
		DEFAULT_ASPECT_X,
		DEFAULT_ASPECT_Y
	);
}
