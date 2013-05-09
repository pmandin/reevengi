noinst_LIBRARIES = libg_common.a

libg_common_a_SOURCES = game.c fs_ignorecase.c menu.c player.c room.c \
	room_map.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = game.h fs_ignorecase.h menu.h player.h room.h room_map.h
