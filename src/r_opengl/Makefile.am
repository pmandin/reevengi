noinst_LIBRARIES = libr_opengl.a

libr_opengl_a_SOURCES = dyngl.c render_bitmap.c render_mask.c \
	render_mesh_opengl.c render_opengl.c render_skel_opengl.c \
	render_texture_opengl.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

#INCLUDES = -I$(top_srcdir)/src

reevengi_generated = dyngl_gl.c dyngl_gl.h dyngl_glext.c dyngl_glext.h

EXTRA_DIST = $(reevengi_generated) dyngl.h render_mask.h \
	render_mesh_opengl.h render_skel_opengl.h render_texture_opengl.h \
	render_bitmap.h
