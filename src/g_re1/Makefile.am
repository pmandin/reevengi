noinst_LIBRARIES = libg_re1.a

libg_re1_a_SOURCES = emd.c \
	re1_pc_game.c re1_ps1.c \
	room_rdt.c \
	room_rdt_script.c room_rdt_script_dump.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h \
	re1_pc_game.ch re1_ps1.h \
	room_rdt.h \
	room_rdt_script.h room_rdt_script_common.h room_rdt_script_dump.h
