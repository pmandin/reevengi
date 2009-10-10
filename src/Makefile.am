bin_PROGRAMS = reevengi

AM_CFLAGS = $(SDL_CFLAGS) $(AVCODEC_CFLAGS) $(AVFORMAT_CFLAGS) $(PHYSFS_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS) $(AVCODEC_CFLAGS) $(AVFORMAT_CFLAGS) $(PHYSFS_CFLAGS)

LIBS = $(SDL_LIBS) $(SDLIMAGE_LIBS) $(PHYSFS_LIBS) $(ZLIB_LIBS) $(AVCODEC_LIBS) \
	$(AVFORMAT_LIBS)

reevengi_SOURCES = background_bss.c background_tim.c clock.c \
	depack_adt.c depack_mdec.c depack_pak.c depack_vlc.c \
	dirty_rects.c dither.c draw_sbuffer.c draw_simple.c dyngl.c \
	filesystem.c idctfst.c log.c main.c matrix.c \
	model_emd.c model_emd2.c model_emd3.c \
	parameters.c physfsrwops.c \
	re1_pc_game.c re1_ps1.c \
	re2_pc_demo.c re2_pc_game.c re2_ps1.c \
	re3_pc.c re3_ps1_game.c \
	render_bitmap.c render_bitmap_opengl.c \
	render.c render_opengl.c \
	render_mesh.c render_mesh_opengl.c \
	render_skel.c render_skel_list.c \
	render_texture.c render_texture_opengl.c \
	render_texture_list.c \
	room.c room_rdt.c room_rdt_script.c \
	room_rdt2.c room_rdt2_script.c room_rdt3_script.c \
	state.c video.c video_opengl.c \
	view_background.c view_movie.c

reevengi_headers = background_bss.h background_tim.h clock.h \
	depack_adt.h depack_mdec.h depack_pak.h depack_vlc.h \
	dirty_rects.h dither.h draw.h draw_sbuffer.h draw_simple.h dyngl.h \
	filesystem.h idctfst.h log.h matrix.h \
	model_emd.h model_emd2.h model_emd3.h \
	parameters.h physfsrwops.h \
	re1_pc_game.h re1_ps1.h \
	re2_pc_demo.h re2_pc_game.h re2_ps1.h \
	re3_pc.h re3_ps1_game.h \
	render.h \
	render_mesh.h render_mesh_opengl.h \
	render_skel.h render_skel_list.h \
	render_texture.h render_texture_opengl.h \
	render_texture_list.h \
	room.h room_rdt.h room_rdt_script.h \
	room_rdt2.h room_rdt2_script.h room_rdt3_script.h \
	state.h video.h \
	view_background.h view_movie.h

reevengi_generated = dyngl_gl.c dyngl_gl.h dyngl_glext.c dyngl_glext.h

EXTRA_DIST = $(reevengi_headers) $(reevengi_generated)

