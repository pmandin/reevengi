bin_PROGRAMS = reevengi

AM_CFLAGS = $(SDL_CFLAGS) $(AVCODEC_CFLAGS) $(AVFORMAT_CFLAGS) $(PHYSFS_CFLAGS)

LIBS = $(SDL_LIBS) $(SDLIMAGE_LIBS) $(PHYSFS_LIBS) $(ZLIB_LIBS) $(AVCODEC_LIBS) \
	$(AVFORMAT_LIBS) $(GLU_LIBS)

reevengi_SOURCES = main.c state.c re1_ps1.c re2_pc_demo.c re2_pc_game.c \
	depack_adt.c parameters.c depack_vlc.c depack_mdec.c idctfst.c \
	re2_ps1.c re3_ps1_game.c depack_pak.c re1_pc_game.c \
	background_bss.c background_tim.c re3_pc.c \
	filesystem.c physfsrwops.c view_background.c view_movie.c \
	dyngl.c video.c video_opengl.c dirty_rects.c video_surface.c \
	video_surface_opengl.c model_emd.c log.c

reevengi_headers = state.h re1_ps1.h re2_pc_demo.h depack_adt.h \
	parameters.h depack_vlc.h depack_mdec.h idctfst.h re2_ps1.h \
	re3_ps1_game.h depack_pak.h re1_pc_game.h background_bss.h \
	background_tim.h re3_pc.h filesystem.h re2_pc_game.h \
	physfsrwops.h view_background.h view_movie.h \
	dyngl.h video.h dirty_rects.h video_surface.h video_surface_opengl.h \
	model_emd.h log.h

reevengi_generated = dyngl_gl.c dyngl_gl.h dyngl_glext.c dyngl_glext.h

EXTRA_DIST = $(reevengi_headers) $(reevengi_generated)

