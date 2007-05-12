#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(reevengi, 0.1, pmandin@caramail.com)
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

# Checks for libraries.

# Math
if test "x$ac_cv_func_pow" = "xno"; then
	LIBS="$LIBS -lm"
fi

# SDL
AM_PATH_SDL( 1.2.0 , :, AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))

# SDL_image
AC_CHECK_LIB(SDL_image, IMG_isJPG, have_sdlimage_lib=yes, have_sdlimage_lib=no)
if test "x$have_sdlimage_lib" = "xyes"; then
	SDL_LIBS="$SDL_LIBS -lSDL_image"
	AC_DEFINE(ENABLE_SDLIMAGE, 1, [Define if you have SDL_image])
fi

# Outputs
AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT
