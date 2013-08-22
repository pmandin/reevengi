noinst_LIBRARIES = libr_opengl.a

libr_opengl_a_SOURCES = dyngl.c render_bitmap.c render_mask.c \
	render_mesh.c render_opengl.c render_skel.c \
	render_texture.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

#INCLUDES = -I$(top_srcdir)/src

reevengi_generated = dyngl_gl.c dyngl_gl.h dyngl_glext.c dyngl_glext.h

EXTRA_DIST = $(reevengi_generated) dyngl.h render_mask.h \
	render_mesh.h render_skel.h render_texture.h \
	render_bitmap.h
