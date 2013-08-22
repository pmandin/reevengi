noinst_LIBRARIES = libr_soft.a

libr_soft_a_SOURCES = dirty_rects.c dither.c draw_sbuffer.c draw_simple.c \
	matrix.c render_mask.c render_bitmap.c render_mesh.c render_texture.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

#INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = dirty_rects.h dither.h draw.h draw_sbuffer.h draw_simple.h \
	matrix.h render_mask.h render_bitmap.h render_mesh.h render_texture.h
