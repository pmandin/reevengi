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

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
#AC_FUNC_MALLOC
AC_HEADER_STDC
AC_CHECK_FUNCS([memset pow])

# Checks for target

SYS_SDLIMAGE_ADDLIBS=""
WANT_GAMMA="yes"
case "$host" in
	*-*-mingw* | *-*-cygwin*)
		WANT_GAMMA="no"
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
	SDLIMAGE_LIBS="-lSDL_image $SYS_SDLIMAGE_ADDLIBS $ZLIB_LIBS -lm"
	AC_DEFINE(ENABLE_SDLIMAGE, 1, [Define if you have SDL_image])
else
	AC_MSG_WARN([*** SDL_image not found!])
fi
CFLAGS=$TMP_CFLAGS
CPPFLAGS=$TMP_CPPFLAGS

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

# Gamma setting
# Some SDL versions have a bug using windib driver that makes it crash

if test "x$WANT_GAMMA" = "xyes"; then
	AC_DEFINE(ENABLE_GAMMA, 1, [Define if you want gamma setting])
fi

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
AC_SUBST(SDLIMAGE_LIBS)
AC_SUBST(PHYSFS_CFLAGS)
AC_SUBST(PHYSFS_LIBS)

AC_CONFIG_FILES([
Makefile
diff/Makefile
src/Makefile
vs2005/Makefile
])
AC_OUTPUT
