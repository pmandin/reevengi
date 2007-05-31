#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(reevengi, 0.3, pmandin@caramail.com)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([autotools])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
#AC_FUNC_MALLOC
AC_HEADER_STDC
AC_CHECK_FUNCS([memset])

# Checks for target

SYS_SDLIMAGE_ADDLIBS=""
case "$target" in
    *-*-mint*)
	SYS_SDLIMAGE_ADDLIBS="-ljpeg -ltiff -lpng"
        ;;
esac	

# Checks for libraries.

# Math

if test "x$ac_cv_func_pow" = "xno"; then
	LIBS="$LIBS -lm"
fi

# SDL

AM_PATH_SDL( 1.2.0 , :, AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))

# Zlib

CHECK_ZLIB()

# SDL_image

TMP_CFLAGS=$CFLAGS
TMP_CPPFLAGS=$CPPFLAGS
CFLAGS="$CFLAGS $SDL_CFLAGS"
CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
AC_CHECK_HEADER(SDL_image.h,
	have_sdlimage_h=yes, havesdlimage_h=no)
AC_CHECK_LIB(SDL_image, IMG_isJPG,
	have_sdlimage_lib=yes, have_sdlimage_lib=no,
	$SDL_LIBS $SYS_SDLIMAGE_ADDLIBS)
if test "x$have_sdlimage_h" = "xyes" -a "x$have_sdlimage_lib" = "xyes"; then
	SDLIMAGE_LIBS="-lSDL_image $SYS_SDLIMAGE_ADDLIBS $ZLIB_LIBS"
	AC_DEFINE(ENABLE_SDLIMAGE, 1, [Define if you have SDL_image])
else
	AC_MSG_ERROR([*** SDL_image not found!])
fi
CFLAGS=$TMP_CFLAGS
CPPFLAGS=$TMP_CPPFLAGS

# physfs

TMP_LIBS=$LIBS
LIBS="$LIBS $ZLIB_LIBS"
AC_CHECK_HEADER(physfs.h, have_physfs_h=yes, have_physfs_h=no)
AC_CHECK_LIB(physfs, PHYSFS_init, have_physfs_lib=yes)
if test "x$have_physfs_h" = "xyes" -a "x$have_physfs_lib" = "xyes"; then
	PHYSFS_LIBS="-lphysfs"
else
	AC_MSG_ERROR([*** PhysicsFS not found!])
fi
LIBS=$TMP_LIBS

# Outputs
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)
AC_SUBST(SDLIMAGE_LIBS)
AC_SUBST(PHYSFS_LIBS)

AC_CONFIG_FILES([
Makefile
diff/Makefile
src/Makefile
])
AC_OUTPUT
