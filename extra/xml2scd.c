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
void generateTypeUnions(xmlDocPtr doc);
void generateTypeUnionStructs(xmlNodePtr node, int gentype);
void generateTypeFields(xmlNodePtr node);

void generateEnums(xmlDocPtr doc);
void generateEnumValues(xmlNodePtr node);

void generateDumps(xmlDocPtr doc);
void generateDumpFields(xmlDocPtr doc, xmlNodePtr node, char *name_low, int *has_block_length);
void generateDumpFieldValues(xmlNodePtr node, char *field_value);
void generateDumpFieldEnums(xmlDocPtr doc, xmlNodePtr node, char *field_value);
void generateDumpFieldEnumValues(xmlNodePtr node, char *field_value);
void generateDumpFieldUnions(xmlDocPtr doc, xmlNodePtr node, char *name_low, char *field_value);
void generateDumpFieldUnionStructs(xmlDocPtr doc, xmlNodePtr node, char *name_low);

void generateLengths(xmlDocPtr doc);

void generateRewiki(xmlDocPtr doc);

/*--- Functions ---*/

int main(int argc, char **argv)
{
	xmlDocPtr doc;

	if (argc<3) {
		fprintf(stderr, "Usage: %s /path/to/scdN.xml [--defines|--types|--enums|--dumps|--lengths|--rewiki]\n", argv[0]);
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
		if (strcmp(argv[2],"--enums")==0) {
			generateEnums(doc);
		}
		if (strcmp(argv[2],"--dumps")==0) {
			generateDumps(doc);
		}
		if (strcmp(argv[2],"--lengths")==0) {
			generateLengths(doc);
		}
		if (strcmp(argv[2],"--rewiki")==0) {
			generateRewiki(doc);
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
	xmlNodeSetPtr nodeset;
	int i;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/inst");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
		return;
	}

	printf("/* Generated by xml2scd */\n");

	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
		xmlChar *inst_name, *inst_id;
		char *inst_name_up;
		int j;

		node = nodeset->nodeTab[i];

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

	printf("/* Generated by xml2scd */\n");

	generateTypeUnions(doc);

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/inst");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
		return;
	}

	/* Each structure */
	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
				
		node = nodeset->nodeTab[i];
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

void generateEnums(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlNodeSetPtr nodeset;
	int i, first_value=1;

	printf("/* Generated by xml2scd */\n");

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/enum");
	if (!path) {
		/*fprintf(stderr, "xml2scd: Path not found\n");*/
		return;
	}

	printf(	"\n"
		"/* Enums */\n"
		"\n"
		"typedef struct {\n"
		"\tUint8 id;\n"
		"\tconst char *name;\n"
		"} scd_enum_t;\n");

	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
		xmlChar *enum_name;
	
		node = nodeset->nodeTab[i];
		enum_name = xmlGetProp(node, "name");

		printf(	"\n"
			"static const scd_enum_t scd_enum_%s_names[]={\n", enum_name);
		generateEnumValues(node);
		printf("};\n");
	}

	xmlXPathFreeObject(path);
}

void generateEnumValues(xmlNodePtr node)
{
	xmlNodePtr child;
	int first_value = 1;

	for (child = node->children; child; child = child->next) {
		xmlChar *value_name, *value_id;

		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (strcmp(child->name, "value")!=0) {
			continue;
		}

		value_name = xmlGetProp(child, "name");
		value_id = xmlGetProp(child, "id");

		printf("\t%s{%s,\t\"%s\"}\n",
			first_value ? "" : ",",
			value_id, value_name);

		first_value = 0;
	}
}

void generateTypeUnions(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlNodeSetPtr nodeset;
	int i;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/union");
	if (!path) {
		/*fprintf(stderr, "xml2scd: Path not found\n");*/
		return;
	}

	printf("\n/* Unions */\n");

	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
		xmlChar *union_name;
	
		node = nodeset->nodeTab[i];
		union_name = xmlGetProp(node, "name");

		generateTypeUnionStructs(node, 0);

		printf(	"\n"
			"typedef union {\n");
		generateTypeUnionStructs(node, 1);
		printf("} %s_u;\n", union_name);
	}

	xmlXPathFreeObject(path);
}

void generateTypeUnionStructs(xmlNodePtr node, int gentype /* 0: complete, 1: only names */)
{
	xmlNodePtr child;
	xmlChar *union_name;
	xmlChar *struct_name;

	union_name = xmlGetProp(node, "name");
	if (!union_name) {
		return;
	}

	for (child = node->children; child; child = child->next) {
		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (strcmp(child->name, "struct")!=0) {
			continue;
		}

		struct_name = xmlGetProp(child, "name");

		if (gentype) {
			printf("\t%s_%s_t\t%s;\n", union_name, struct_name, struct_name);
		} else {
			printf(	"\n"
				"typedef struct {\n");
			generateTypeFields(child);		
			printf(	"} %s_%s_t;\n", union_name, struct_name);
		}
	}
}

void generateTypeFields(xmlNodePtr node)
{
	xmlNodePtr child;
	xmlChar *field_name, *field_type, *field_array;

	for (child = node->children; child; child = child->next) {
		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (strcmp(child->name, "field")!=0) {
			continue;
		}

		field_name = xmlGetProp(child, "name");
		field_type = xmlGetProp(child, "type");
		field_array = xmlGetProp(child, "array");

		if (strcmp(field_type, "union")==0) {
			printf("\t%s_u\t%s;\n", field_name, field_name);
		} else {
			if (field_array) {
				printf("\t%s\t%s[%s];\n", field_type, field_name, field_array);
			} else {
				printf("\t%s\t%s;\n", field_type, field_name);
			}
		}
	}
}

void generateDumps(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlChar *inst_name, *inst_id;
	char *inst_name_up, *inst_name_low;
	xmlNodeSetPtr nodeset;
	int i, j;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/inst");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
	}

	printf("/* Generated by xml2scd */\n\n");
	printf("switch(%s->opcode) {\n", DUMP_INST_PTR);

	/* Each instruction */
	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
		int has_block_length = 0;

		node = nodeset->nodeTab[i];

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

		printf("\tcase INST_%s:\n", inst_name_up);
		printf("\t\t{\n"
			"\t\t\tstrcat(%s, \"%s%s\");\n",
			DUMP_BUFFER_FINAL,
			(strcmp(&inst_id[2], inst_name_low) == 0) ? "INST_" : "",
			inst_name);
		generateDumpFields(doc, node, inst_name_low, &has_block_length);
		printf("\t\t\tstrcat(%s, \"\\n\");\n",
			DUMP_BUFFER_FINAL);

		/* Process length of block */
		if (has_block_length) {
			printf("\t\t\tblock_len = %s->i_%s.block_length;\n",
				DUMP_INST_PTR,
				inst_name_low);
			printf("\t\t\tblock_ptr = (script_inst_t *) (&((Uint8 *) %s)[sizeof(script_inst_%s_t)]);\n",
				DUMP_INST_PTR,
				inst_name_low);
		}

		printf("\t\t}\n"
			"\t\tbreak;\n");

		free(inst_name_up);
		free(inst_name_low);
	}

	printf(	"\tdefault:\n");
	printf(	"\t\tsprintf(%s, \"Unknown opcode 0x%%02x\\n\", %s->opcode);\n",
		DUMP_BUFFER_TMP,
		DUMP_INST_PTR);
	printf(	"\t\tstrcat(%s, %s);\n",
		DUMP_BUFFER_FINAL,
		DUMP_BUFFER_TMP);
	printf(	"\t\tbreak;\n"
		"}\n");

	xmlXPathFreeObject(path);
}

void generateDumpFields(xmlDocPtr doc, xmlNodePtr node, char *name_low, int *has_block_length)
{
	xmlNodePtr child;
	xmlChar *field_name, *field_type, *field_base, *field_array;
	char swapValue[64];
	int base, size, i, array_size;

	for (child = node->children; child; child = child->next) {
		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (strcmp(child->name, "field")!=0) {
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
		base = 16;
		field_base = xmlGetProp(child, "base");
		if (field_base) {
			base = atoi(field_base);
		}
		array_size = 1;
		field_array = xmlGetProp(child, "array");
		if (field_array) {
			array_size = atoi(field_array);
		}

		sprintf(swapValue, "%s->i_%s.%s", DUMP_INST_PTR, name_low, field_name);
		size=2;
		if ((strcmp(field_type, "Uint16")==0) || (strcmp(field_type, "Sint16")==0)) {
			sprintf(swapValue, "SDL_SwapLE16(%s->i_%s.%s)", DUMP_INST_PTR, name_low, field_name);
			size=4;
		}
		if ((strcmp(field_type, "Uint32")==0) || (strcmp(field_type, "Sint32")==0)) {
			sprintf(swapValue, "SDL_SwapLE32(%s->i_%s.%s)", DUMP_INST_PTR, name_low, field_name);
			size=8;
		}

		if (strcmp(field_type, "union")==0) {
			xmlChar *switch_name;
			
			switch_name = xmlGetProp(child, "switch");
			if (switch_name) {
				sprintf(swapValue, "%s->i_%s.%s", DUMP_INST_PTR, name_low, switch_name);
				generateDumpFieldUnions(doc, child, name_low, swapValue);
			}
			continue;
		}

		if (array_size==1) {
			if (base==10) {
				printf("\t\t\tsprintf(%s, \" %s=%%d\", %s);\n",
					DUMP_BUFFER_TMP,
					field_name,
					swapValue);
			} else {
				printf("\t\t\tsprintf(%s, \" %s=0x%%0%dx\", %s);\n",
					DUMP_BUFFER_TMP,
					field_name,
					size,
					swapValue);
			}

			printf("\t\t\tstrcat(strBuf, tmpBuf);\n");

			if (xmlGetProp(child, "enum")) {
				generateDumpFieldEnums(doc, child, swapValue);
			} else {
				generateDumpFieldValues(child, swapValue);
			}
		} else {
			sprintf(swapValue, "%s->i_%s.%s[i]", DUMP_INST_PTR, name_low, field_name);
			if ((strcmp(field_type, "Uint16")==0) || (strcmp(field_type, "Sint16")==0)) {
				sprintf(swapValue, "SDL_SwapLE16(%s->i_%s.%s[i])", DUMP_INST_PTR, name_low, field_name);
			}
			if ((strcmp(field_type, "Uint32")==0) || (strcmp(field_type, "Sint32")==0)) {
				sprintf(swapValue, "SDL_SwapLE32(%s->i_%s.%s[i])", DUMP_INST_PTR, name_low, field_name);
			}

			printf(	"\t\t\t{\n"
				"\t\t\t\tint i;\n\n"
				"\t\t\t\tfor(i=0;i<%d;i++) {\n", array_size);

			if (base==10) {
				printf("\t\t\t\t\tsprintf(%s, \" %s[%%d]=%%d\", i, %s);\n",
					DUMP_BUFFER_TMP,
					field_name,
					swapValue);
			} else {
				printf("\t\t\t\t\tsprintf(%s, \" %s[%%d]=0x%%0%dx\", i, %s);\n",
					DUMP_BUFFER_TMP,
					field_name,
					size,
					swapValue);
			}

			printf("\t\t\t\t\tstrcat(strBuf, tmpBuf);\n");

			if (xmlGetProp(child, "enum")) {
				generateDumpFieldEnums(doc, child, swapValue);
			} else {
				generateDumpFieldValues(child, swapValue);
			}

			printf(	"\t\t\t\t}\n"
				"\t\t\t}\n");
		}
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
			printf("\t\t\tswitch (%s) {\n",
				field_value);
			first_value=0;
		}

		printf("\t\t\t\tcase %s:\tsprintf(%s, \" (%s)\");\tbreak;\n",
			value_id,
			DUMP_BUFFER_TMP,
			value_name);
	}

	if (!first_value) {
		printf(	"\t\t\t\tdefault:\tsprintf(%s, \" (---)\");\tbreak;\n"
			"\t\t\t}\n"
			"\t\t\tstrcat(%s, %s);\n",
			DUMP_BUFFER_TMP,
			DUMP_BUFFER_FINAL,
			DUMP_BUFFER_TMP);
	}
}

void generateDumpFieldEnums(xmlDocPtr doc, xmlNodePtr node, char *field_value)
{
	xmlChar *node_enum_name;

	node_enum_name = xmlGetProp(node, "enum");
	if (!node_enum_name) {
		return;
	}

	printf(	"\t\t\t{\n"
		"\t\t\t\tint i;\n"
		"\n"
		"\t\t\t\tfor (i=0; i<sizeof(scd_enum_%s_names)/sizeof(scd_enum_t); i++) {\n",
		node_enum_name);
	printf(	"\t\t\t\t\tif (%s == scd_enum_%s_names[i].id) {\n",
		field_value,
		node_enum_name);
	printf("\t\t\t\t\t\tsprintf(%s, \" (%%s)\", scd_enum_%s_names[i].name);\n",
		DUMP_BUFFER_TMP,
		node_enum_name);
	printf(	"\t\t\t\t\t\tstrcat(%s, %s);\n",
		DUMP_BUFFER_FINAL,
		DUMP_BUFFER_TMP);
	printf(	"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t\t}\n"
		"\t\t\t\t}\n"
		"\t\t\t}\n");
}

void generateDumpFieldEnumValues(xmlNodePtr node, char *field_value)
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
			printf("\t\t\tswitch (%s) {\n",
				field_value);
			first_value=0;
		}

		printf("\t\t\t\tcase %s:\tsprintf(%s, \" (%s)\");\tbreak;\n",
			value_id,
			DUMP_BUFFER_TMP,
			value_name);
	}

	if (!first_value) {
		printf(	"\t\t\t\tdefault:\tsprintf(%s, \" (---)\");\tbreak;\n"
			"\t\t\t}\n"
			"\t\t\tstrcat(%s, %s);\n",
			DUMP_BUFFER_TMP,
			DUMP_BUFFER_FINAL,
			DUMP_BUFFER_TMP);
	}
}

void generateDumpFieldUnions(xmlDocPtr doc, xmlNodePtr node, char *name_low, char *field_value)
{
	xmlXPathObjectPtr path;
	xmlNodeSetPtr nodeset;
	xmlChar *union_name;
	int i, first_value=1;
	char swapValue[64];

	union_name = xmlGetProp(node, "name");
	if (!union_name) {
		return;
	}

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/union");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
		return;
	}

	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr child;
		xmlChar *child_name;
		
		child = nodeset->nodeTab[i];

		child_name = xmlGetProp(child, "name");
		if (strcmp(child_name, union_name)!=0) {
			continue;
		}		

		sprintf(swapValue, "%s.%s", name_low, child_name);

		printf(	"\t\t\tswitch(%s) {\n", field_value);
		generateDumpFieldUnionStructs(doc, child, swapValue);
		printf(	"\t\t\t\tdefault:\n"
			"\t\t\t\t\tbreak;\n"
			"\t\t\t}\n");
	}

	xmlXPathFreeObject(path);
}

void generateDumpFieldUnionStructs(xmlDocPtr doc, xmlNodePtr node, char *name_low)
{
	xmlNodePtr child;
	int has_block_length = 0;
	char swapValue[64];

	for (child = node->children; child; child = child->next) {
		xmlChar *struct_name, *struct_id;

		if (child->type != XML_ELEMENT_NODE) {
			continue;
		}

		if (strcmp(child->name, "struct")!=0) {
			continue;
		}

		struct_name = xmlGetProp(child, "name");
		struct_id = xmlGetProp(child, "id");
		if (!struct_name || !struct_id) {
			continue;
		}

		sprintf(swapValue, "%s.%s", name_low, struct_name);

		printf(	"\t\t\t\tcase %s:\n"
			"\t\t\t\t\t{\n", struct_id);
		generateDumpFields(doc, child, swapValue, &has_block_length);
		printf(	"\t\t\t\t\t}\n"
			"\t\t\t\t\tbreak;\n");
	}
}

void generateLengths(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlChar *inst_name, *var_length;
	char *inst_name_up, *inst_name_low;
	xmlNodeSetPtr nodeset;
	int i, j;
	int first_value=1;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/inst");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
	}

	printf("/* Generated by xml2scd */\n");

	/* Array containing all instructions */
	printf("static const script_inst_len_t inst_length[]={\n");

	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
				
		node = nodeset->nodeTab[i];

		inst_name = xmlGetProp(node, "name");
		var_length = xmlGetProp(node, "variable_length");
		if (var_length) {
			continue;
		}

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

		printf("\t%s{INST_%s,\tsizeof(script_inst_%s_t)}\n",
			first_value ? "" : ",",
			inst_name_up, inst_name_low);

		free(inst_name_up);
		free(inst_name_low);

		first_value = 0;
	}

	printf("};\n");

	xmlXPathFreeObject(path);
}

void generateRewiki(xmlDocPtr doc)
{
	xmlXPathObjectPtr path;
	xmlChar *inst_name, *inst_id;
	char *inst_name_up, *inst_name_low;
	xmlNodeSetPtr nodeset;
	int i, j;

	path = (xmlXPathObjectPtr) xpathSearch(doc, "/scd/inst");
	if (!path) {
		fprintf(stderr, "xml2scd: Path not found\n");
	}

	printf(	"{|\n"
		"!Byte!!Instruction!!Length!!Description\n");

	/* Each instruction */
	nodeset = path->nodesetval;
	for (i=0; i < nodeset->nodeNr; i++) {
		xmlNodePtr node;
				
		node = nodeset->nodeTab[i];

		inst_name = xmlGetProp(node, "name");
		inst_id = xmlGetProp(node, "id");

		/* Convert to lower case for structure */
		inst_name_low = strdup(inst_name);
		j=0;
		while (inst_name_low[j]) {
			inst_name_low[j] = tolower(inst_name_low[j]);
			++j;
		}

		printf(	"|-\n"
			"|%s||%s||-||%s\n", inst_id, inst_name, "description");

		printf(	"<pre><nowiki>\n"
			"typedef struct {\n");
		generateTypeFields(node);		
		printf(	"} script_inst_%s_t;\n"
			"</nowiki></pre>\n",
			inst_name_low);

		free(inst_name_low);
	}

	printf("|}\n");

	xmlXPathFreeObject(path);
}
