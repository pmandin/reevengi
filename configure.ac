#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(reevengi, 0.18, patmandin@gmail.com)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([autotools])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

PKG_PROG_PKG_CONFIG
m4_ifndef([PKG_CHECK_MODULES], [AC_DEFUN([PKG_CHECK_MODULES], [true])])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AC_PROG_RANLIB

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
#AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset pow])

case "$host" in
	m68k*)
		AC_CHECK_DEFINE([__m68k__], [],
			[CFLAGS="$CFLAGS -D__m68k__"
			CCASFLAGS="$CCASFLAGS -D__m68k__"]
		)
		;;
	*)
		;;
esac	

# Checks for target

SYS_SDLIMAGE_ADDLIBS=""
case "$host" in
	*-*-mingw* | *-*-cygwin*)
		;;
	*-*-mint*)
		SYS_SDLIMAGE_ADDLIBS="-ltiff -ljpeg -lpng"
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

if test "x$PKG_CONFIG" != "x"; then
	PKG_CHECK_MODULES([SDL_IMAGE], [SDL_image], [have_sdlimage=yes], [have_sdlimage=no])
	if [[ "x$have_sdlimage" = "xyes" ]]; then
		AC_DEFINE([HAVE_SDLIMAGE], 1, [Define if SDL_image enabled])
	fi
else
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
		SDL_IMAGE_LIBS="-lSDL_image $SYS_SDLIMAGE_ADDLIBS $ZLIB_LIBS -lm"
		SDL_IMAGE_CFLAGS=""
		AC_DEFINE(HAVE_SDLIMAGE, 1, [Define if you have SDL_image])
	else
		AC_MSG_WARN([*** SDL_image not found!])
	fi
	CFLAGS=$TMP_CFLAGS
	CPPFLAGS=$TMP_CPPFLAGS
fi

# physfs

TMP_LIBS=$LIBS
TMP_CFLAGS=$CFLAGS
LIBS="$LIBS $ZLIB_LIBS"
AC_ARG_WITH([physfs-prefix],
	[  --with-physfs-prefix=PFX Prefix where Physfs is installed (optional)],
	[WITH_PHYSFS_PFX="$withval"],
	[WITH_PHYSFS_PFX=""])
if test "x$WITH_PHYSFS_PFX"  != "x"; then
	CFLAGS="$CFLAGS -I${WITH_PHYSFS_PFX}/include"
	LIBS="$LIBS -L${WITH_PHYSFS_PFX}/lib"
fi
AC_CHECK_HEADER(physfs.h, have_physfs_h=yes, have_physfs_h=no)
AC_CHECK_LIB(physfs, PHYSFS_init, have_physfs_lib=yes)
if test "x$have_physfs_h" = "xyes" -a "x$have_physfs_lib" = "xyes"; then
	PHYSFS_CFLAGS="-I${WITH_PHYSFS_PFX}/include"
	PHYSFS_LIBS="-L${WITH_PHYSFS_PFX}/lib -lphysfs"
else
	AC_MSG_ERROR([*** PhysicsFS not found!])
fi
LIBS=$TMP_LIBS
CFLAGS=$TMP_CFLAGS

# OpenGL renderer

AC_ARG_ENABLE(opengl,
	[  --enable-opengl         Enable OpenGL (default=yes)],
	[WANT_OPENGL=$withval], [WANT_OPENGL=yes])
GLU_LIBS=
if test "x$WANT_OPENGL" = "xyes"; then
	AC_DEFINE(ENABLE_OPENGL, 1, [Define if you want OpenGL renderer])
fi

# Talloc

#if test "x$PKG_CONFIG" != "x"; then
#	PKG_CHECK_MODULES([LIBTALLOC], [talloc])
#fi

# Cdio

#if test "x$PKG_CONFIG" != "x"; then
#	PKG_CHECK_MODULES([LIBCDIO], [libcdio])
#fi

# Movies using FFmpeg

AC_ARG_ENABLE(movies,
	[  --enable-movies         Enable movie player using FFmpeg (default=yes)],
	[WANT_MOVIES=$withval], [WANT_MOVIES=yes])
if test "x$WANT_MOVIES" = "xyes"; then
	PKG_CHECK_MODULES([AVCODEC], [libavcodec])
	PKG_CHECK_MODULES([AVFORMAT], [libavformat])

	AC_DEFINE(ENABLE_MOVIES, 1, [Define if you want movie player])
fi

# Game script disassembly

AC_ARG_ENABLE(script_disasm,
	[  --enable-script-disasm  Enable script disassembly (default=yes)],
	[WANT_SCRIPT_DISASM=$withval], [WANT_SCRIPT_DISASM=yes])
if test "x$WANT_SCRIPT_DISASM" = "xyes"; then
	AC_DEFINE(ENABLE_SCRIPT_DISASM, 1, [Define if you want script disassembly])
fi

# Debugging stuff

AC_ARG_ENABLE(assert,
	[  --disable-assert        Disable debugging with assert (default=yes)],
	, [enable_assert="no"])
if test "x$enable_assert" = "xno"; then
	AC_DEFINE(NDEBUG, 1, [Define if you do not want debugging with assert])
fi

# Outputs
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)
AC_SUBST(SDL_IMAGE_CFLAGS)
AC_SUBST(SDL_IMAGE_LIBS)
AC_SUBST(PHYSFS_CFLAGS)
AC_SUBST(PHYSFS_LIBS)

AC_CONFIG_FILES([
Makefile
diff/Makefile
src/Makefile
src/g_common/Makefile
src/g_re1/Makefile
src/g_re2/Makefile
src/g_re3/Makefile
src/r_common/Makefile
src/r_opengl/Makefile
src/r_soft/Makefile
vs2005/Makefile
vs2005/libg_common/Makefile
vs2005/libg_re1/Makefile
vs2005/libg_re2/Makefile
vs2005/libg_re3/Makefile
vs2005/libr_common/Makefile
vs2005/libr_soft/Makefile
vs2005/libr_opengl/Makefile
])
AC_OUTPUT
