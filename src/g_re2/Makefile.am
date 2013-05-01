noinst_LIBRARIES = libg_re2.a

libg_re2_a_SOURCES = emd.c game_re2.c \
	game_re2_pc_demo.c game_re2_pc_game.c game_re2_ps1.c \
	room_rdt2.c \
	room_rdt2_script.c room_rdt2_script_dump.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h game_re2.h \
	room_rdt2.h \
	room_rdt2_script.h room_rdt2_script_common.h room_rdt2_script_dump.h
