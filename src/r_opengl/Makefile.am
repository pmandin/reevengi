noinst_LIBRARIES = libr_opengl.a

libr_opengl_a_SOURCES = dyngl.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

INCLUDES = -I$(top_srcdir)/src

reevengi_generated = dyngl_gl.c dyngl_gl.h dyngl_glext.c dyngl_glext.h

EXTRA_DIST = dyngl.h $(reevengi_generated)
