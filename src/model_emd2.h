/*
	Load EMD model
	Resident Evil 2

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

#ifndef MODEL_EMD2
#define MODEL_EMD2 1

#include "video.h"

/*--- Functions ---*/

int model_emd2_load(const char *filename);
void model_emd2_close(void);
void model_emd2_draw(void);

#endif /* MODEL_EMD2 */
