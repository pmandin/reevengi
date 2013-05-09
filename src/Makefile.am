SUBDIRS = g_common g_re1 g_re2 g_re3 r_opengl r_soft

bin_PROGRAMS = reevengi

AM_CFLAGS = $(SDL_CFLAGS) $(AVCODEC_CFLAGS) $(AVFORMAT_CFLAGS) $(PHYSFS_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS) $(AVCODEC_CFLAGS) $(AVFORMAT_CFLAGS) $(PHYSFS_CFLAGS)

LIBS = $(SDL_LIBS) $(SDL_IMAGE_LIBS) $(PHYSFS_LIBS) $(ZLIB_LIBS) $(AVCODEC_LIBS) \
	$(AVFORMAT_LIBS)

reevengi_SOURCES = background_bss.c background_tim.c clock.c \
	depack_adt.c depack_mdec.c depack_pak.c depack_vlc.c depack_sld.c \
	filesystem.c idctfst.c log.c main.c \
	parameters.c physfsrwops.c \
	render_bitmap.c render_bitmap_opengl.c \
	render.c render_opengl.c render_text.c \
	render_mesh.c render_mesh_opengl.c \
	render_skel.c render_skel_opengl.c render_skel_list.c \
	render_texture.c render_texture_opengl.c \
	render_texture_list.c \
	render_mask.c render_mask_opengl.c \
	video.c video_opengl.c \
	view_background.c view_movie.c

reevengi_headers = background_bss.h background_tim.h clock.h \
	depack_adt.h depack_mdec.h depack_pak.h depack_vlc.h depack_sld.h \
	filesystem.h idctfst.h log.h \
	parameters.h physfsrwops.h \
	render.h render_bitmap.h render_text.h \
	render_mesh.h render_mesh_opengl.h \
	render_skel.h render_skel_opengl.h render_skel_list.h \
	render_texture.h render_texture_opengl.h \
	render_texture_list.h \
	render_mask.h render_mask_opengl.h \
	video.h \
	view_background.h view_movie.h

reevengi_LDADD = g_common/libg_common.a g_re1/libg_re1.a g_re2/libg_re2.a \
	g_re3/libg_re3.a r_opengl/libr_opengl.a r_soft/libr_soft.a

EXTRA_DIST = $(reevengi_headers)
