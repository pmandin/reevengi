noinst_LIBRARIES = libg_re3.a

libg_re3_a_SOURCES = emd.c sld.c game_re3.c game_re3_pc.c \
	game_re3_ps1_game.c rdt_scd.c rdt_scd_dump.c rdt_sca.c \
	rdt.c ard.c

AM_CFLAGS = $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS) $(PHYSFS_CFLAGS) $(LIBXML_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS) $(PHYSFS_CFLAGS) $(LIBXML_CFLAGS)

XML2SCD = $(top_builddir)/extra/xml2scd

SCD_XML = scd3.xml

rdt_scd_dump.c: rdt_scd_defs.gen.h rdt_scd_types.gen.h \
	rdt_scd_dumps.gen.c rdt_scd_enums.gen.c

rdt_scd.c:	rdt_scd_defs.gen.h rdt_scd_types.gen.h \
	rdt_scd_lengths.gen.c

rdt_scd_defs.gen.h:	$(SCD_XML) $(XML2SCD)
	$(XML2SCD) $< --defines > $@

rdt_scd_types.gen.h:	$(SCD_XML) $(XML2SCD)
	$(XML2SCD) $< --types > $@

rdt_scd_dumps.gen.c:	$(SCD_XML) $(XML2SCD)
	$(XML2SCD) $< --dumps > $@

rdt_scd_lengths.gen.c:	$(SCD_XML) $(XML2SCD)
	$(XML2SCD) $< --lengths > $@

rdt_scd_enums.gen.c:	$(SCD_XML) $(XML2SCD)
	$(XML2SCD) $< --enums > $@

header_files = emd.h sld.h game_re3.h \
	rdt_scd.h rdt_scd_dump.h rdt_sca.h \
	rdt.h ard.h cd_raw.c

generated_files = rdt_scd_defs.gen.h rdt_scd_types.gen.h \
	rdt_scd_dumps.gen.c rdt_scd_lengths.gen.c \
	rdt_scd_enums.gen.c

EXTRA_DIST = $(header_files) $(generated_files) \
	libg_re3.vcproj xml_libg_re3.vcproj $(SCD_XML)
