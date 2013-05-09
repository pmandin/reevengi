noinst_LIBRARIES = libg_re3.a

libg_re3_a_SOURCES = emd.c game_re3.c game_re3_pc.c game_re3_ps1_game.c \
	rdt_scd.c rdt_scd_dump.c

AM_CFLAGS = $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = emd.h game_re3.h \
	rdt_scd.h rdt_scd_common.h rdt_scd_dump.h
