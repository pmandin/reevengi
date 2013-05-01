noinst_LIBRARIES = libg_re3.a

libg_re3_a_SOURCES = emd.c game_re3.c game_re3_pc.c game_re3_ps1_game.c \
	room_rdt3.c \
	room_rdt3_script.c room_rdt3_script_dump.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h game_re3.h \
	room_rdt3.h \
	room_rdt3_script.h room_rdt3_script_common.h room_rdt3_script_dump.h
