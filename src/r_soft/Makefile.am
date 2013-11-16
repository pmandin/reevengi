noinst_LIBRARIES = libr_soft.a

libr_soft_a_SOURCES = dirty_rects.c dither.c draw_sbuffer.c \
	matrix.c render_mask.c render_bitmap.c render_mesh.c render_texture.c \
	draw.c render.c draw_sbuffer8.c draw_sbuffer16.c draw_sbuffer24.c \
	draw_sbuffer32.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

EXTRA_DIST = dirty_rects.h dither.h draw.h draw_sbuffer.h \
	matrix.h render_mask.h render_bitmap.h render_mesh.h render_texture.h \
	render.h draw_sbuffer8.h draw_sbuffer16.h draw_sbuffer24.h \
	draw_sbuffer32.h \
	span_fill.inc.c span_gouraud.inc.c span_textured.inc.c \
	libr_soft.vcproj
