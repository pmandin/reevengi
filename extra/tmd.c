#include <stdio.h>
#include <fcntl.h>

/* Load file in mem from filename, return buffer, update length */

char *loadFile(char *filename, int *length)
{
	int handle;
	char *buffer;

	/* Load file */
	handle = open(filename, O_RDONLY);
	if (handle<0) {
		fprintf(stderr, "Unable to open %s\n", filename);	
		return NULL;
	}

	*length = lseek(handle, 0, SEEK_END);
	lseek(handle, 0, SEEK_SET); 	

	buffer = (char *)malloc(*length);
	if (buffer==NULL) {
		fprintf(stderr, "Unable to allocate %d bytes\n", length);
		return NULL;
	}

	read(handle, buffer, *length);
	close(handle);

	return buffer;
}

typedef struct {
	unsigned long	id;
	unsigned long	dummy;
	unsigned long	count;
} tmd_header_t;

typedef struct {
	unsigned long vertex_offset;
	unsigned long vertex_count;
	unsigned long normal_offset;
	unsigned long normal_count;
	unsigned long primitive_offset;
	unsigned long primitive_count;
	unsigned long log_scale;
} tmd_object_t;

typedef struct {
	short x;
	short y;
	short z;
	short zero;
} tmd_vertex_t;

typedef struct {
	unsigned long id;	/* 0x34000609 */
	unsigned long prim1;	/* xxrrggbb ? */
	unsigned long prim2;	/* xxxxuuvv ? */
	unsigned long prim3;
	unsigned short v0;
	unsigned short n0;
	unsigned short v1;
	unsigned short n1;
	unsigned short v2;
	unsigned short n2;
} tmd_primitive_t;

void parseVertex(tmd_vertex_t *tmdVertex, int count)
{
	int i;
	
	for (i=0; i<count; i++) {
		printf("  x=%d,y=%d,z=%d\n", tmdVertex[i].x, tmdVertex[i].y, tmdVertex[i].z);
	}
}

void parsePrimitive(tmd_primitive_t *tmdPrimitive, int count)
{
	int i;
	
	for (i=0; i<count; i++) {
		printf("  i:0x%08x,0x%08x,0x%08x,0x%08x\n",
			tmdPrimitive[i].id,
			tmdPrimitive[i].prim1,
			tmdPrimitive[i].prim2,
			tmdPrimitive[i].prim3);
		printf("  v:%d,%d,%d\n",
			tmdPrimitive[i].v0,
			tmdPrimitive[i].v1,
			tmdPrimitive[i].v2);
		printf("  n:%d,%d,%d\n",
			tmdPrimitive[i].n0,
			tmdPrimitive[i].n1,
			tmdPrimitive[i].n2);
	}
}

void parseObject(unsigned char *tmd, tmd_object_t *tmdObject)
{
	printf(" vertex:    offset=0x%08x, count=%d\n",
		tmdObject->vertex_offset, tmdObject->vertex_count);
	parseVertex((tmd_vertex_t *) &tmd[tmdObject->vertex_offset + sizeof(tmd_header_t)], tmdObject->vertex_count);

	printf(" normal:    offset=0x%08x, count=%d\n",
		tmdObject->normal_offset, tmdObject->normal_count);
	parseVertex((tmd_vertex_t *) &tmd[tmdObject->normal_offset + sizeof(tmd_header_t)], tmdObject->normal_count);

	printf(" primitive: offset=0x%08x, count=%d\n",
		tmdObject->primitive_offset, tmdObject->primitive_count);
	parsePrimitive((tmd_primitive_t *) &tmd[tmdObject->primitive_offset + sizeof(tmd_header_t)], tmdObject->primitive_count);
}

void parseTmd(unsigned char *tmd, int length)
{
	int i;
	tmd_header_t *tmdHeader = (tmd_header_t *) tmd;
	printf("id=0x%08x, objects=%d\n", tmdHeader->id, tmdHeader->count);

	for (i=0; i<tmdHeader->count; i++) {
		printf("object %d:\n", i);
		parseObject(tmd, (tmd_object_t *) &tmd[sizeof(tmd_header_t)+sizeof(tmd_object_t)*i]);
	}
}

int main(int argc, char **argv)
{
	int length;
	unsigned char *fileInMem;
/*
	if (argc<2) {
		return 1;
	}
*/
	fileInMem = loadFile("/windows/Program Files/ResidentEvil/usa/players/ws202.tmd", &length);
	/*fileInMem = loadFile(argv[1], &length);*/
	if (fileInMem==NULL) {
		return 1;
	}

	parseTmd(fileInMem, length);

	free(fileInMem);

	return 0;
}
