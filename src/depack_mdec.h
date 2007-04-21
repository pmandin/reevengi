/*
	MDEC depacker
*/

#ifndef DEPACK_MDEC_H
#define DEPACK_MDEC_H

#include <SDL.h>

void depack_mdec(SDL_RWops *src, Uint8 **dstPointer, int *dstLength,
	int width, int height);

#endif /* DEPACK_MDEC_H */
