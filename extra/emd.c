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

void *getObjectPtr(void *source, unsigned long offset)
{
	if (offset==0) {
		return NULL;
	}
	return &((unsigned char *) source)[offset];
}

void printObject0(unsigned long *objPointer)
{
	printf("  0x%08x\n", objPointer[0]);
}

typedef struct {
	unsigned short count;
	unsigned short offset;
} emd1_t;

void printObject1(void *objPointer)
{
	int i, j, emd1Count;
	emd1_t *emd1 = objPointer;

	emd1Count = emd1[0].offset / sizeof(emd1_t);	
	printf("  %d objects\n", emd1Count);
	for (i=0;i<emd1Count;i++) {
		printf("  count=0x%04x, offset=0x%04x\n",
			emd1[i].count, emd1[i].offset
		);
		unsigned long *emd1Index = (unsigned long *)getObjectPtr(objPointer, emd1[i].offset);
		for (j=0; j<emd1[i].count; j++) {
			printf("   0x%04x: 0x%08x\n", j, emd1Index[j]);
		}
	}
}

void printObject2(void *objPointer)
{
	int i, j, emd2Count;
	unsigned char *emd2 = objPointer;
	emd2 = &emd2[8];

	emd2Count = 0x361;
	printf("  %d objects\n", emd2Count);

	long minv = 0x7fffffff, maxv = 0x80000000;
	for (i=0;i<emd2Count;i++) {
		short *emd2Vertex = (short *)emd2;
		printf("   %04x:\n", i);
/*
		for (j=0;j<5;j++) {
			printf("    "
				"0x%04x,0x%04x,0x%04x,0x%04x,"
				"0x%04x,0x%04x,0x%04x,0x%04x\n",
				emd2Vertex[j*8+0], emd2Vertex[j*8+1],
				emd2Vertex[j*8+2], emd2Vertex[j*8+3],
				emd2Vertex[j*8+4], emd2Vertex[j*8+5],
				emd2Vertex[j*8+6], emd2Vertex[j*8+7]
			);
		}
*/
			long value = (long) emd2Vertex[0];
			if (minv> value) {
				minv = value;
			}
			if (maxv< value) {
				maxv = value;
			}
		emd2 += 0x50;
	}
	printf("min/max: 0x%08x, 0x%08x\n", minv, maxv);
}

void printVertex7(void *objPointer)
{
	unsigned long offset = ((unsigned long*) objPointer)[0];

	unsigned char *tmpPtr = (unsigned char *) objPointer;
	tmpPtr += offset;

	unsigned short *vertexPtr = (unsigned short *) tmpPtr;
	printf("    vertex 0x%04x,0x%04x,0x%04x\n", vertexPtr[0], vertexPtr[1], vertexPtr[2]);
	printf("    normal 0x%04x,0x%04x,0x%04x\n", vertexPtr[3], vertexPtr[4], vertexPtr[5]);
}

void printVertex70(void *objPointer, unsigned long count, unsigned long offset)
{
	int i;
	printf("    s70: count:0x%08x, offset:0x%08x\n", count, offset);

	unsigned char *tmpPtr = (unsigned char *) objPointer;
	tmpPtr += offset;

	for (i=0;i<count;i++) {
		printf("    s70: %d,%d,%d\n",
			((short *)tmpPtr)[0],
			((short *)tmpPtr)[1],
			((short *)tmpPtr)[2]
		);
		tmpPtr += 8;
	}
}

void printVertex71(void *objPointer, unsigned long count, unsigned long offset)
{
	int i;
	printf("    s71: count:0x%08x, offset:0x%08x\n", count, offset);

	unsigned char *tmpPtr = (unsigned char *) objPointer;
	tmpPtr += offset /*+ numPrim*12*/;

	int total = 0;
	for (i=0;i<count;i++) {
		printf("    s71: 0x%08x,0x%08x,0x%08x\n",
			((unsigned long *)tmpPtr)[0],
			((unsigned long *)tmpPtr)[1],
			((unsigned long *)tmpPtr)[2]
		);
		tmpPtr += 12;
		total |= (((unsigned long *)tmpPtr)[1]>>16) & 3;
	}
	printf("    s71: total: 0x%08x\n", total);
}

void printVertex72(void *objPointer, unsigned long count, unsigned long offset)
{
	int i;
	printf("    s72: count:0x%08x, offset:0x%08x\n", count, offset);

	unsigned char *tmpPtr = (unsigned char *) objPointer;
	tmpPtr += offset;

	int total = 0;
	for (i=0;i<count;i++) {
		printf("    s72: 0x%08x,0x%08x,0x%08x,0x%08x\n",
			((unsigned long *)tmpPtr)[0],
			((unsigned long *)tmpPtr)[1],
			((unsigned long *)tmpPtr)[2],
			((unsigned long *)tmpPtr)[3]
		);
		tmpPtr += 16;
		total |= (((unsigned long *)tmpPtr)[1]>>16) & 3;
	}
	printf("    s72: total: 0x%08x\n", total);
}

void printVertex73(void *objPointer, unsigned long count, unsigned long offset, int flag)
{
	int i;
	printf("    s73: count:0x%08x, offset:0x%08x\n", count, offset);

	unsigned char *tmpPtr = (unsigned char *) objPointer;
	tmpPtr += offset;
	if (flag) {
		tmpPtr += 12;
	}

	int maxIndex = 0;
	for (i=0;i<count;i++) {
		int j;
		unsigned short *indexPtr = (unsigned short *) tmpPtr;
		printf("    s73: 0x%04x: ",i);
		for (j=0; j<6; j++) {
			printf(" 0x%04x", indexPtr[j]);
			if (maxIndex<indexPtr[j]) {
				maxIndex = indexPtr[j];
			}
		}
		printf("\n");
		tmpPtr += 12;
	}
	printf("    s73: maxIndex: 0x%04x\n", maxIndex);
}

void printVertex74(void *objPointer, unsigned long count, unsigned long offset, int flag)
{
	int i;
	printf("    s74: count:0x%08x, offset:0x%08x\n", count, offset);

	unsigned char *tmpPtr = (unsigned char *) objPointer;
	tmpPtr += offset;
	if (flag) {
		tmpPtr += 16;
	}

	int maxIndex = 0;
	for (i=0;i<count;i++) {
		int j;
		unsigned short *indexPtr = (unsigned short *) tmpPtr;
		printf("    s74: 0x%04x: ",i);
		for (j=0; j<8; j++) {
			printf(" 0x%04x", indexPtr[j]);
			if (maxIndex<indexPtr[j]) {
				maxIndex = indexPtr[j];
			}
		}
		printf("\n");
		tmpPtr += 16;
	}
	printf("    s74: maxIndex: 0x%04x\n", maxIndex);
}

void printObject7(void *objPointer)
{
	unsigned long *obj7 = (unsigned long *) objPointer;
	unsigned long *obj7base = obj7;
	printf("  length: 0x%08x\n", obj7[0]);

	unsigned long count = obj7[2];
	printf("  count: 0x%08x\n", count);

	obj7 += 3;
	int i;

	int totalSize = 0;
	for (i=0; i<count/2; i++) {
		printf("  %d:\n", i);
		printf("   0x00: 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x\n",
			obj7[0],obj7[1],obj7[2],obj7[3],obj7[4],obj7[5],obj7[6]
		);
		printf("   0x1c: 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x\n",
			obj7[7],obj7[8],obj7[9],obj7[10],obj7[11],obj7[12],obj7[13]
		);

		totalSize += obj7[5]*12;
		totalSize += obj7[12]*16;

		printVertex70(&obj7base[3],obj7[1],obj7[0]);
		printVertex71(&obj7base[3],obj7[5],obj7[6]);
		printVertex72(&obj7base[3],obj7[12],obj7[13]);
		printVertex73(&obj7base[3],obj7[5],obj7[4], obj7base[1]&1);
		printVertex74(&obj7base[3],obj7[12],obj7[11], obj7base[1]&1);
		obj7 += (0x1c/4)*2;
	}
	printf("  totalSize: 0x%08x (%d)\n", totalSize, totalSize);
}

void printObject(int objNum, void *objPointer)
{
	switch(objNum) {
		case 0:
			printObject0(objPointer);
			break;
/*		case 1:
		case 3:
		case 5:
			printObject1(objPointer);
			break;
		case 2:
		case 4:
		dcase 6:
			printObject2(objPointer);
			break;*/
		case 7:
			printObject7(objPointer);
	}
}

typedef struct {
	unsigned long headerOffset;
	unsigned long headerLength;
} emd_t;

void parseEnhancedModelDescription(emd_t *emd, int length)
{
	int i;
	unsigned long *emdHeaderOffset;

	printf("file length: 0x%08x\n", length);

	printf("header: offset 0x%08x, length 0x%08x\n",
		emd->headerOffset,
		emd->headerLength
	);

	emdHeaderOffset = (unsigned long *) getObjectPtr(emd, emd->headerOffset);
	for (i=0; i<emd->headerLength; i++) {
		printf(" offset %d: 0x%08x\n", i, emdHeaderOffset[i]);
		printObject(i, getObjectPtr(emd, emdHeaderOffset[i]));
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
	fileInMem = loadFile("/windows_d/ResidentEvil2-Demo2/pl0/emd0/em010.emd", &length);
	/*fileInMem = loadFile(argv[1], &length);*/
	if (fileInMem==NULL) {
		return 1;
	}

	parseEnhancedModelDescription((emd_t *)fileInMem, length);

	free(fileInMem);

	return 0;
}
