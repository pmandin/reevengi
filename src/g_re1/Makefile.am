noinst_LIBRARIES = libg_re1.a

libg_re1_a_SOURCES = emd.c pak.c game_re1.c game_re1_pc.c \
	game_re1_ps1.c rdt.c rdt_rvd.c rdt_rid.c rdt_pri.c rdt_scd.c \
	rdt_scd_dump.c rdt_sca.c rdt_msg.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

EXTRA_DIST = emd.h pak.h game_re1.h rdt.h rdt_rvd.h rdt_rid.h \
	rdt_pri.h rdt_scd.h rdt_scd_common.h rdt_scd_dump.h \
	rdt_sca.h rdt_msg.h rdt_evt.h \
	libg_re1.vcproj

rdt_scd_dump.c: rdt_scd_defs.gen.h rdt_scd_types.gen.h \
	rdt_scd_dumps.gen.c

rdt_scd_defs.gen.h:	scd1.xml
	$(top_builddir)/extra/xml2scd $< --defines > $@

rdt_scd_types.gen.h:	scd1.xml
	$(top_builddir)/extra/xml2scd $< --types > $@

rdt_scd_dumps.gen.c:	scd1.xml
	$(top_builddir)/extra/xml2scd $< --dumps > $@
