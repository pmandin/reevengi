noinst_LIBRARIES = libg_re1.a

libg_re1_a_SOURCES = emd.c game_re1.c game_re1_pc.c game_re1_ps1.c \
	room_rdt.c \
	room_rdt_script.c room_rdt_script_dump.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h game_re1.h game_re1_pc.h game_re1_ps1.h \
	room_rdt.h \
	room_rdt_script.h room_rdt_script_common.h room_rdt_script_dump.h
