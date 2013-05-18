noinst_LIBRARIES = libr_common.a

libr_common_a_SOURCES = render_skel_list.c render_texture_list.c \
	render_text.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = render_skel_list.h render_texture_list.h \
	render_text.h
