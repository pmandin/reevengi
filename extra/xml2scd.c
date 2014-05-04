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
#include <string.h>

#include <libxml/xmlversion.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>

/*--- Defines ---*/

#define DUMP_BUFFER_FINAL "strBuf"	/* char strBuf[256] */
#define DUMP_BUFFER_TMP	"tmpBuf"	/* char tmpBuf[256] */
#define DUMP_INST_PTR	"inst"		/* script_inst_t *inst */

/*--- Functions prototypes ---*/

xmlXPathObjectPtr xpathSearch (xmlDocPtr doc, xmlChar *xpath);

void generateDefines(xmlDocPtr doc);
void generateTypes(xmlDocPtr doc);
void generateTypeFields(xmlNodePtr node);
void generateDumps(xmlDocPtr doc);
void generateDumpFields(xmlNodePtr node, char *name_low, int *has_block_length);
void generateDumpFieldValues(xmlNodePtr node, char *field_value);

/*--- Functions ---*/

int main(int argc, char **argv)
{
	xmlDocPtr doc;

	if (argc<3) {
		fprintf(stderr, "Usage: %s /path/to/scdN.xml [--defines|--types|--dumps]\n", argv[0]);
		return 1;
	}

	LIBXML_TEST_VERSION
	xmlInitParser();

	doc = xmlParseFile(argv[1]);
	if (doc) {
		if (strcmp(argv[2],"--defines")==0) {
			generateDefines(doc);
		}
		if (strcmp(argv[2],"--types")==0) {
			generateTypes(doc);
		}
		if (strcmp(argv[2],"--dumps")==0) {
			generateDumps(doc);
		}
	} else {
		fprintf(stderr, "xml2scd: Can not read %s\n", argv[1]);
	}

	xmlCleanupParser();
	xmlMemoryDump();

	return 0;
}

xmlXPathObjectPtr xpathSearch (xmlDocPtr doc, xmlChar *xpath)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext(doc);
	result = xmlXPathEvalExpression(xpath, context);
	if(xmlXPathNodeSetIsEmpty(result->nodesetval))
	{
		return NULL;
	}
	xmlXPathFreeContext(context);

	return result;
}

void generateDefines(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlChar *inst_name, *inst_id;
	char *inst_name_up;
	xmlNodeSetPtr nodeset;
	int i, j;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/*[name()]");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
	}

	printf("/* Generated by xml2scd */\n");

	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
				
		node = nodeset->nodeTab[i];
		if (strcmp(node->name, "inst")!=0) {
			continue;
		}

		inst_name = xmlGetProp(node, "name");
		inst_id = xmlGetProp(node, "id");

		/* Convert to upper case for define */
		inst_name_up = strdup(inst_name);
		j=0;
		while (inst_name_up[j]) {
			inst_name_up[j] = toupper(inst_name_up[j]);
			++j;
		}

		printf("#define INST_%s\t%s\n", inst_name_up, inst_id);

		free(inst_name_up);
	}

	xmlXPathFreeObject(path);
}

void generateTypes(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlChar *inst_name, *inst_id;
	char *inst_name_low;
	xmlNodeSetPtr nodeset;
	int i, j;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/*[name()]");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
	}

	printf("/* Generated by xml2scd */\n");

	/* Each structure */
	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
				
		node = nodeset->nodeTab[i];
		if (strcmp(node->name, "inst")!=0) {
			continue;
		}

		inst_name = xmlGetProp(node, "name");

		/* Convert to lower case for structure */
		inst_name_low = strdup(inst_name);
		j=0;
		while (inst_name_low[j]) {
			inst_name_low[j] = tolower(inst_name_low[j]);
			++j;
		}

		printf("\n" "typedef struct {\n");
		generateTypeFields(node);		
		printf("} script_inst_%s_t;\n", inst_name_low);

		free(inst_name_low);
	}

	/* Union containing all instructions */
	printf("\n"
		"typedef union {\n"
		"\tUint8\topcode;\n");

	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
				
		node = nodeset->nodeTab[i];
		if (strcmp(node->name, "inst")!=0) {
			continue;
		}

		inst_name = xmlGetProp(node, "name");

		/* Convert to lower case for structure */
		inst_name_low = strdup(inst_name);
		j=0;
		while (inst_name_low[j]) {
			inst_name_low[j] = tolower(inst_name_low[j]);
			++j;
		}

		printf("\tscript_inst_%s_t\ti_%s;\n", inst_name_low, inst_name_low);

		free(inst_name_low);
	}

	printf("} script_inst_t;\n");

	xmlXPathFreeObject(path);
}

void generateTypeFields(xmlNodePtr node)
{
	xmlNodePtr child;
	xmlChar *field_name, *field_type;

	for (child = node->children; child; child = child->next) {
		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		field_name = xmlGetProp(child, "name");
		field_type = xmlGetProp(child, "type");

		printf("\t%s\t%s;\n", field_type, field_name);
	}
}

void generateDumps(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlChar *inst_name, *inst_id;
	char *inst_name_up, *inst_name_low;
	xmlNodeSetPtr nodeset;
	int i, j;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/*[name()]");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
	}

	printf("/* Generated by xml2scd */\n");

	/* Each instruction */
	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
		int has_block_length = 0;

		node = nodeset->nodeTab[i];
		if (strcmp(node->name, "inst")!=0) {
			continue;
		}

		inst_name = xmlGetProp(node, "name");
		inst_id = xmlGetProp(node, "id");

		/* Convert to upper case for define */
		inst_name_up = strdup(inst_name);
		j=0;
		while (inst_name_up[j]) {
			inst_name_up[j] = toupper(inst_name_up[j]);
			++j;
		}

		/* Convert to lower case for structure */
		inst_name_low = strdup(inst_name);
		j=0;
		while (inst_name_low[j]) {
			inst_name_low[j] = tolower(inst_name_low[j]);
			++j;
		}

		printf("case INST_%s:\n", inst_name_up);
		printf("\t{\n"
			"\t\tstrcat(%s, \"%s%s\");\n",
			DUMP_BUFFER_FINAL,
			(strcmp(&inst_id[2], inst_name_low) == 0) ? "INST_" : "",
			inst_name_up);
		generateDumpFields(node, inst_name_low, &has_block_length);

		/* Process length of block */
		if (has_block_length) {
			printf("\t\tblock_len = %s->i_%s.block_length;\n",
				DUMP_INST_PTR,
				inst_name_low);
			printf("\t\tblock_ptr = (script_inst_t *) (&((Uint8 *) %s)[sizeof(script_inst_%s_t)]);\n",
				DUMP_INST_PTR,
				inst_name_low);
		}

		printf("\t}\n"
			"\tbreak;\n");

		free(inst_name_up);
		free(inst_name_low);
	}

	xmlXPathFreeObject(path);
}

void generateDumpFields(xmlNodePtr node, char *name_low, int *has_block_length)
{
	xmlNodePtr child;
	xmlChar *field_name, *field_type;
	char swapValue[32];

	for (child = node->children; child; child = child->next) {
		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		field_name = xmlGetProp(child, "name");
		if ((strcmp(field_name, "opcode")==0)
		    || (strcmp(field_name, "dummy")==0)) {
			continue;
		}
		if (strcmp(field_name, "block_length")==0) {
			*has_block_length = 1;
			continue;
		}
		field_type = xmlGetProp(child, "type");

		sprintf(swapValue, "%s->i_%s.%s", DUMP_INST_PTR, name_low, field_name);
		if ((strcmp(field_type, "Uint16")==0) || (strcmp(field_type, "Sint16")==0)) {
			sprintf(swapValue, "SDL_SwapLE16(%s->i_%s.%s)", DUMP_INST_PTR, name_low, field_name);
		}
		if ((strcmp(field_type, "Uint32")==0) || (strcmp(field_type, "Sint32")==0)) {
			sprintf(swapValue, "SDL_SwapLE32(%s->i_%s.%s)", DUMP_INST_PTR, name_low, field_name);
		}

		printf("\t\tsprintf(%s, \" %s=%%d\", %s);\n",
			DUMP_BUFFER_TMP,
			field_name,
			swapValue);

		printf("\t\tstrcat(strBuf, tmpBuf);\n");

		generateDumpFieldValues(child, swapValue);
	}
}

void generateDumpFieldValues(xmlNodePtr node, char *field_value)
{
	xmlNodePtr child;
	xmlChar *value_name, *value_id;
	int first_value = 1;

	for (child = node->children; child; child = child->next) {
		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (strcmp(child->name, "value")!=0) {
			continue;
		}

		value_name = xmlGetProp(child, "name");
		value_id = xmlGetProp(child, "id");

		if (first_value) {
			printf("\t\tswitch (%s) {\n",
				field_value);
			first_value=0;
		}

		printf("\t\t\tcase %s:\tsprintf(%s, \" (%s)\");\n",
			value_id,
			DUMP_BUFFER_TMP,
			value_name);
	}

	if (!first_value) {
		printf("\t\t\tdefault:\tbreak;\n"
			"\t\t}\n");
		printf("\t\tstrcat(%s, %s);\n",
			DUMP_BUFFER_FINAL,
			DUMP_BUFFER_TMP);
	}
}
