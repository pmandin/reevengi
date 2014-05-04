noinst_LIBRARIES = libg_re2.a

libg_re2_a_SOURCES = emd.c adt.c game_re2.c \
	game_re2_pc_demo.c game_re2_pc_game.c game_re2_ps1.c \
	rdt.c rdt_rid.c rdt_rvd.c rdt_pri.c rdt_msg.c \
	rdt_scd.c rdt_scd_dump.c rdt_sca.c ems.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

XML2SCD = $(top_builddir)/extra/xml2scd

SCD_XML = scd2.xml

rdt_scd_defs.gen.h:	$(SCD_XML) $(XML2SCD)
	$(XML2SCD) $< --defines > $@

header_files = emd.h adt.h game_re2.h game_re2_pc_game.h \
	rdt.h rdt_rid.h rdt_rvd.h rdt_pri.h rdt_msg.h \
	rdt_scd.h rdt_scd_common.h rdt_scd_dump.h rdt_sca.h \
	rdt_esp.h rdt_lit.h ems.h

generated_files = rdt_scd_defs.gen.h

EXTRA_DIST = $(header_files) $(generated_files) \
	libg_re2.vcproj $(SCD_XML)
