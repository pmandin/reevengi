/*
	Load EMD model
	Resident Evil

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

#ifndef MODEL_EMD
#define MODEL_EMD 1

#include <SDL.h>

#include "model.h"

/*--- Functions ---*/

model_t *model_emd_load(SDL_RWops *src_emd, SDL_RWops *src_tim);

#endif /* MODEL_EMD */
