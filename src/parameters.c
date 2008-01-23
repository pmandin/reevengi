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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parameters.h"

/*--- Defines ---*/

#define DEFAULT_BASEDIR "."
#define DEFAULT_VERBOSE 0
#define DEFAULT_GAMMA 1.0
#define DEFAULT_USE_OPENGL 0

/*--- Global variables ---*/

/* Directory of the game */
unsigned char *basedir=DEFAULT_BASEDIR;

/* Verbose mode */
int verbose=DEFAULT_VERBOSE;

/* Gamma level */
float gamma = DEFAULT_GAMMA;

/* Viewer mode */
int viewmode = VIEWMODE_BACKGROUND;

/* Enable OpenGL */
int use_opengl = DEFAULT_USE_OPENGL;

/*---- Variables ---*/

/*--- Functions ---*/

static int ParmPresent(char *param, int argc, char **argv)
{
	int	i;

	for (i=1; i<argc; i++)
		if ( !strcasecmp(param, argv[i]) )
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
		verbose = atoi(argv[p+1]);
	}

	/*--- Check for gamma ---*/
	p = ParmPresent("-gamma", argc, argv);
	if (p && p < argc-1) {
		gamma = atof(argv[p+1]);
	}

	/*--- Check for base directory ---*/
	p = ParmPresent("-basedir", argc, argv);
	if (p && p < argc-1) {
		basedir = argv[p+1];
	}

	/*--- Check for movie mode ---*/
	p = ParmPresent("-movie", argc, argv);
	if (p) {
		viewmode = VIEWMODE_MOVIE;
	}

	/*--- Check for OpenGL ---*/
	p = ParmPresent("-opengl", argc, argv);
	if (p) {
		use_opengl = 1;
	}

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
		"  [-verbose <n>] (default=%d)\n"
		"  [-help] (print this message)\n",
		DEFAULT_BASEDIR,
		DEFAULT_GAMMA,
		DEFAULT_VERBOSE
	);
}
