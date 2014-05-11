noinst_LIBRARIES = libg_re1.a

libg_re1_a_SOURCES = emd.c pak.c game_re1.c game_re1_pc.c \
	game_re1_ps1.c rdt.c rdt_rvd.c rdt_rid.c rdt_pri.c rdt_scd.c \
	rdt_scd_dump.c rdt_sca.c rdt_msg.c

AM_CFLAGS = $(SDL_CFLAGS)
AM_CXXFLAGS = $(SDL_CFLAGS)

XML2SCD = $(top_builddir)/extra/xml2scd

SCD_XML = scd1.xml

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

header_files = emd.h pak.h game_re1.h rdt.h rdt_rvd.h rdt_rid.h \
	rdt_pri.h rdt_scd.h rdt_scd_dump.h \
	rdt_sca.h rdt_msg.h rdt_evt.h

generated_files = rdt_scd_defs.gen.h rdt_scd_types.gen.h rdt_scd_dumps.gen.c \
	rdt_scd_lengths.gen.c rdt_scd_enums.gen.c

EXTRA_DIST = $(header_files) $(generated_files) \
	libg_re1.vcproj $(SCD_XML)
	
