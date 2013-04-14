noinst_LIBRARIES = libg_re2.a

libg_re2_a_SOURCES = emd.c \
	re2_pc_demo.c re2_pc_game.c re2_ps1.c \
	room_rdt2.c \
	room_rdt2_script.c room_rdt2_script_dump.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h \
	re2_pc_demo.h re2_pc_game.h re2_ps1.h
	room_rdt2.h \
	room_rdt2_script.h room_rdt2_script_common.h room_rdt2_script_dump.h
