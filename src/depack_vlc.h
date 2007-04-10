/*
	VLC file depacker
*/

#ifndef DEPACK_VLC_H
#define DEPACK_VLC_H

#include <SDL.h>

void vlc_depack(SDL_RWops *src, Uint8 **dstPointer, int *dstLength);

#endif /* DEPACK_VLC_H */
