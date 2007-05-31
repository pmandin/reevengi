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
	unsigned long dummy[7];
	unsigned long offset0;
} object7_t;

typedef struct {
	unsigned long offset0;
	unsigned long offset1;
} object10_t;

typedef struct {
	char dummy0;
	char numObject7;
	char numObject10;
	char dummy1[5];
	unsigned long offsets[22];
} rdt_t;

object7_t	*obj7Ptr;
object10_t	*obj10Ptr;

void *getObjectPtr(void *source, unsigned long offset)
{
	if (offset==0) {
		return NULL;
	}
	return &((unsigned char *) source)[offset];
}

void parseRoomDescription(rdt_t *rdt, int length)
{
	int i;

	printf("length: %d\n", length);

	for (i=0; i<17; i++) {
		printf("object list: pointer %d: 0x%08x\n", i, getObjectPtr((unsigned char *)rdt, rdt->offsets[i]));
	}

	object7_t *obj7 = getObjectPtr((unsigned char *)rdt, rdt->offsets[7]);
	for (i=0; i<rdt->numObject7; i++) {
		printf("object 7: pointer %d: 0x%08x (0x%08x)\n", i,
			obj7[i].offset0, getObjectPtr((unsigned char *)rdt, obj7[i].offset0)
		);
	}

	object10_t *obj10 = getObjectPtr((unsigned char *)rdt, rdt->offsets[10]);
	for (i=0; i<rdt->numObject10; i++) {
		printf("object 10: pointer %d: 0x%08x (0x%08x), 0x%08x (0x%08x)\n", i,
			obj10[i].offset0, getObjectPtr((unsigned char *)rdt, obj10[i].offset0),
			obj10[i].offset1, getObjectPtr((unsigned char *)rdt, obj10[i].offset1)
		);
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
	fileInMem = loadFile("/windows_d/ResidentEvil2-Demo2/pl0/rdu/room1000.rdt", &length);
	/*fileInMem = loadFile(argv[1], &length);*/
	if (fileInMem==NULL) {
		return 1;
	}

	parseRoomDescription((rdt_t *)fileInMem, length);

	free(fileInMem);

	return 0;
}
