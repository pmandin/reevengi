noinst_PROGRAMS = xml2scd

xml2scd_SOURCES = xml2scd.c

xml2scd.o:	$(xml2scd_SOURCES)
	$(CC_FOR_BUILD) -g $(CFLAGS_FOR_BUILD) $(LIBXML_CFLAGS) -c $< -o $@

xml2scd:	xml2scd.o
	$(CCLD_FOR_BUILD) -g $(LDFLAGS_FOR_BUILD) $(LIBXML_LIBS) -o $@ $<
