bin_PROGRAMS = reevengi

AM_CFLAGS = $(SDL_CFLAGS)

LIBS = $(SDL_LIBS)

reevengi_SOURCES = main.c file.c state.c re1_ps1_demo.c re2_pc_demo.c \
	depack_adt.c parameters.c depack_vlc.c depack_mdec.c idctfst.c \
	re2_ps1_demo.c re3_ps1_game.c depack_pak.c re1_pc_game.c

reevengi_headers = file.h state.h re1_ps1_demo.h re2_pc_demo.h depack_adt.h \
	parameters.h depack_vlc.h depack_mdec.h idctfst.h re2_ps1_demo.h \
	re3_ps1_game.h depack_pak.h re1_pc_game.h

EXTRA_DIST = $(reevengi_headers)
