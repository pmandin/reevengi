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

#ifndef PARAMETERS_H
#define PARAMETERS_H

/*--- Defines ---*/

#define VIEWMODE_BACKGROUND	0
#define VIEWMODE_MOVIE		1

/*--- Types ---*/

typedef struct {
	int verbose;		/* Verbose level */
	const char *log_file;	/* Log file */
	const char *basedir;	/* Base directory for files */
	float gamma;		/* Gamma level */
	int viewmode;		/* Viewer mode */
	int use_opengl;		/* Enable OpenGL */
	int aspect_x;		/* Aspect ratio */
	int aspect_y;
	int aspect_user;	/* User gave us an aspect ratio */
} params_t;

/*--- Variables ---*/

extern params_t params;

/*--- Functions prototypes ---*/

extern void DisplayUsage(void);
extern int CheckParm(int argc,char **argv);

#endif /* PARAMETERS_H */
