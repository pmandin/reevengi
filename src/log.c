/*
	Messages log

	Copyright (C) 2008	Patrice Mandin
	
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

#include <stdarg.h>
#include <stdio.h>

#include <SDL.h>

#include "parameters.h"

/*--- Variables ---*/

static int firsttime=1;
static int createfile=1;

static int use_ticks = 0;
static Uint32 ticks = 0;

/*--- Functions ---*/

void logMsg(int level, const char *fmt, ...)
{
	FILE *output;
	va_list ap;

	if (params.verbose<level) {
		return;
	}

	if (use_ticks) {
		ticks = SDL_GetTicks();
	}

	/* Print on stdout */
	printf("[%9.3f] ", ticks/1000.0f);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	/* Write to log file ? */
	output = fopen(params.log_file, createfile ? "w" : "a+");
	createfile = 0;
	if (!output) {
		if (firsttime) {
			fprintf(stderr, "Can not open log file %s\n", params.log_file);
			firsttime=0;
		}
		return;
	}

	fprintf(output, "[%9.3f] ", ticks/1000.0f);
	va_start(ap, fmt);
	vfprintf(output, fmt, ap);
	va_end(ap);

	fclose(output);
}

void logEnableTicks(void)
{
	use_ticks = 1;
}
