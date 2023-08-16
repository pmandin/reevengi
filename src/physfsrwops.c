/*
 * This code provides a glue layer between PhysicsFS and Simple Directmedia
 *  Layer's (SDL) RWops i/o abstraction.
 *
 * License: this code is public domain. I make no warranty that it is useful,
 *  correct, harmless, or environmentally safe.
 *
 * This particular file may be used however you like, including copying it
 *  verbatim into a closed-source project, exploiting it commercially, and
 *  removing any trace of my name from the source (although I hope you won't
 *  do that). I welcome enhancements and corrections to this file, but I do
 *  not require you to send me patches if you make changes. This code has
 *  NO WARRANTY.
 *
 * Unless otherwise stated, the rest of PhysicsFS falls under the zlib license.
 *  Please see LICENSE.txt in the root of the source tree.
 *
 * SDL falls under the LGPL license. You can get SDL at http://www.libsdl.org/
 *
 *  This file was written by Ryan C. Gordon. (icculus@icculus.org).
 */

/*--- Includes ---*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>  /* used for SEEK_SET, SEEK_CUR, SEEK_END ... */
#include <SDL.h>

#include "physfsrwops.h"

/*--- Defines ---*/

#if SDL_VERSION_ATLEAST(2,0,0)
#define	REEVENGI_PHYSFS_SEEKTYPE	Sint64
#define	REEVENGI_PHYSFS_READTYPE	size_t
#define	REEVENGI_PHYSFS_WRITETYPE	size_t
#else
#define	REEVENGI_PHYSFS_SEEKTYPE	int
#define	REEVENGI_PHYSFS_READTYPE	int
#define	REEVENGI_PHYSFS_WRITETYPE	int
#endif

#if ((PHYSFS_VER_MAJOR==2) && (PHYSFS_VER_MINOR>=1)) || (PHYSFS_VER_MAJOR>2)
#define REEVENGI_PHYSFS_21	1
#else
#define REEVENGI_PHYSFS_21	0
#endif

static REEVENGI_PHYSFS_SEEKTYPE physfsrwops_seek(SDL_RWops *rw, REEVENGI_PHYSFS_SEEKTYPE offset, int whence)
{
    PHYSFS_File *handle = (PHYSFS_File *) rw->hidden.unknown.data1;
    REEVENGI_PHYSFS_SEEKTYPE pos = 0;

    if (whence == SEEK_SET)
    {
        pos = offset;
    } /* if */

    else if (whence == SEEK_CUR)
    {
        PHYSFS_sint64 current = PHYSFS_tell(handle);
        if (current == -1)
        {
            SDL_SetError("Can't find position in file: %s",
#if HAVE_PHYSFS_GETLASTERRORCODE
                         PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
                         PHYSFS_getLastError()
#endif
            );
            return(-1);
        } /* if */

        pos = (int) current;
        if ( ((PHYSFS_sint64) pos) != current )
        {
            SDL_SetError("Can't fit current file position in an int!");
            return(-1);
        } /* if */

        if (offset == 0)  /* this is a "tell" call. We're done. */
            return(pos);

        pos += offset;
    } /* else if */

    else if (whence == SEEK_END)
    {
        PHYSFS_sint64 len = PHYSFS_fileLength(handle);
        if (len == -1)
        {
            SDL_SetError("Can't find end of file: %s",
#if HAVE_PHYSFS_GETLASTERRORCODE
                         PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
                         PHYSFS_getLastError()
#endif
            );
            return(-1);
        } /* if */

        pos = (int) len;
        if ( ((PHYSFS_sint64) pos) != len )
        {
            SDL_SetError("Can't fit end-of-file position in an int!");
            return(-1);
        } /* if */

        pos += offset;
    } /* else if */

    else
    {
        SDL_SetError("Invalid 'whence' parameter.");
        return(-1);
    } /* else */

    if ( pos < 0 )
    {
        SDL_SetError("Attempt to seek past start of file.");
        return(-1);
    } /* if */

    if (!PHYSFS_seek(handle, (PHYSFS_uint64) pos))
    {
        SDL_SetError("PhysicsFS error: %s",
#if HAVE_PHYSFS_GETLASTERRORCODE
                     PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
                     PHYSFS_getLastError()
#endif
        );
        return(-1);
    } /* if */

    return(pos);
} /* physfsrwops_seek */


static REEVENGI_PHYSFS_READTYPE physfsrwops_read(SDL_RWops *rw, void *ptr, REEVENGI_PHYSFS_READTYPE size, REEVENGI_PHYSFS_READTYPE maxnum)
{
    PHYSFS_File *handle = (PHYSFS_File *) rw->hidden.unknown.data1;
#if (REEVENGI_PHYSFS_21 == 1)
    PHYSFS_sint64 rc = PHYSFS_readBytes(handle, ptr, size * maxnum);
#else
    PHYSFS_sint64 rc = PHYSFS_read(handle, ptr, size, maxnum);
#endif
    if (rc != maxnum)
    {
        if (!PHYSFS_eof(handle)) { /* not EOF? Must be an error. */
            SDL_SetError("PhysicsFS error: %s",
#if HAVE_PHYSFS_GETLASTERRORCODE
                     PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
                     PHYSFS_getLastError()
#endif
            );
        }
    } /* if */

    return((REEVENGI_PHYSFS_READTYPE) rc);
} /* physfsrwops_read */


static REEVENGI_PHYSFS_WRITETYPE physfsrwops_write(SDL_RWops *rw, const void *ptr, REEVENGI_PHYSFS_WRITETYPE size, REEVENGI_PHYSFS_WRITETYPE num)
{
    PHYSFS_File *handle = (PHYSFS_File *) rw->hidden.unknown.data1;
#if (REEVENGI_PHYSFS_21 == 1)
    PHYSFS_sint64 rc = PHYSFS_writeBytes(handle, ptr, size * num);
#else
    PHYSFS_sint64 rc = PHYSFS_write(handle, ptr, size, num);
#endif
	if (rc != num) {
        SDL_SetError("PhysicsFS error: %s",
#if HAVE_PHYSFS_GETLASTERRORCODE
                     PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
                     PHYSFS_getLastError()
#endif
        );
    }

    return((REEVENGI_PHYSFS_WRITETYPE) rc);
} /* physfsrwops_write */


static int physfsrwops_close(SDL_RWops *rw)
{
    PHYSFS_File *handle = (PHYSFS_File *) rw->hidden.unknown.data1;
    if (!PHYSFS_close(handle))
    {
        SDL_SetError("PhysicsFS error: %s",
#if HAVE_PHYSFS_GETLASTERRORCODE
                     PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
                     PHYSFS_getLastError()
#endif
        );
        return(-1);
    } /* if */

    SDL_FreeRW(rw);
    return(0);
} /* physfsrwops_close */


static SDL_RWops *create_rwops(PHYSFS_File *handle)
{
    SDL_RWops *retval = NULL;

    if (handle == NULL) {
        SDL_SetError("PhysicsFS error: %s",
#if HAVE_PHYSFS_GETLASTERRORCODE
                     PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#else
                     PHYSFS_getLastError()
#endif
        );
    } else
    {
        retval = SDL_AllocRW();
        if (retval != NULL)
        {
            retval->seek  = physfsrwops_seek;
            retval->read  = physfsrwops_read;
            retval->write = physfsrwops_write;
            retval->close = physfsrwops_close;
            retval->hidden.unknown.data1 = handle;
        } /* if */
    } /* else */

    return(retval);
} /* create_rwops */


SDL_RWops *PHYSFSRWOPS_makeRWops(PHYSFS_File *handle)
{
    SDL_RWops *retval = NULL;
    if (handle == NULL)
        SDL_SetError("NULL pointer passed to PHYSFSRWOPS_makeRWops().");
    else
        retval = create_rwops(handle);

    return(retval);
} /* PHYSFSRWOPS_makeRWops */


SDL_RWops *PHYSFSRWOPS_openRead(const char *fname)
{
    return(create_rwops(PHYSFS_openRead(fname)));
} /* PHYSFSRWOPS_openRead */


SDL_RWops *PHYSFSRWOPS_openWrite(const char *fname)
{
    return(create_rwops(PHYSFS_openWrite(fname)));
} /* PHYSFSRWOPS_openWrite */


SDL_RWops *PHYSFSRWOPS_openAppend(const char *fname)
{
    return(create_rwops(PHYSFS_openAppend(fname)));
} /* PHYSFSRWOPS_openAppend */


/* end of physfsrwops.c ... */

