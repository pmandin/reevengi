noinst_PROGRAMS = xml2scd$(EXEEXT)

xml2scd_SOURCES = xml2scd.c

xml2scd.o:	$(xml2scd_SOURCES)
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LIBXML_CFLAGS) -c $< -o $@

xml2scd$(EXEEXT):	xml2scd.o
	$(CCLD_FOR_BUILD) $(LDFLAGS_FOR_BUILD) -o $@ $<  $(LIBXML_LIBS) $(ZLIB_LIBS_FOR_BUILD)

EXTRA_DIST = xml2scd.vcproj Makefile.nmake
