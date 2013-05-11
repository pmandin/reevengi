noinst_LIBRARIES = libg_re2.a

libg_re2_a_SOURCES = emd.c depack_adt.c game_re2.c \
	game_re2_pc_demo.c game_re2_pc_game.c game_re2_ps1.c \
	rdt.c rdt_rid.c rdt_rvd.c rdt_pri.c rdt_msg.c \
	rdt_scd.c rdt_scd_dump.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h depack_adt.h game_re2.h \
	rdt.h rdt_rid.h rdt_rvd.h rdt_pri.h rdt_msg.h \
	rdt_scd.h rdt_scd_common.h rdt_scd_dump.h
