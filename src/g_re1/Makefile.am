noinst_LIBRARIES = libg_re1.a

libg_re1_a_SOURCES = emd.c pak.c game_re1.c game_re1_pc.c \
	game_re1_ps1.c rdt.c rdt_rvd.c rdt_rid.c rdt_pri.c rdt_scd.c \
	rdt_scd_dump.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h pak.h game_re1.h rdt.h rdt_rvd.h rdt_rid.h \
	rdt_pri.c rdt_scd.h rdt_scd_common.h rdt_scd_dump.h
