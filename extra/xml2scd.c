/*
	XML2SCD
	Convert XML data to .h and .c files to process SCD stuff

	Copyright (C) 2014	Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>

#include <libxml/xmlversion.h>
#include <libxml/xmlreader.h>

/*--- Functions prototypes ---*/

void generateHeaders(xmlDocPtr doc);

/*--- Functions ---*/

int main(int argc, char **argv)
{
	xmlDocPtr doc;

	if (argc<3) {
		fprintf(stderr, "Usage: %s /path/to/scdN.xml [--headers]\n", argv[0]);
		return 1;
	}

	LIBXML_TEST_VERSION
	xmlInitParser();

	doc = xmlParseFile(argv[1]);
	if (doc) {
		if (strcmp(argv[2],"--headers")==0) {
			generateHeaders(doc);
		}
	} else {
		fprintf(stderr, "xml2scd: Can not read %s\n", argv[1]);
	}

	xmlCleanupParser();
	xmlMemoryDump();

	return 0;
}

void generateHeaders(xmlDocPtr doc)
{
}

