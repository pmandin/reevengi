noinst_LIBRARIES = libg_re1.a

libg_re1_a_SOURCES = emd.c game_re1.c game_re1_pc.c game_re1_ps1.c \
	room_rdt_script.c room_rdt_script_dump.c \
	rdt_rvd.c rdt_rid.c rdt_pri.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h game_re1.h \
	room_rdt_script.h room_rdt_script_common.h room_rdt_script_dump.h \
	rdt_rvd.h rdt_rid.h rdt_pri.c
