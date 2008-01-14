#include <stdio.h>
#include <stdlib.h>
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

	buffer = (char *) malloc(*length);
	if (buffer==NULL) {
		fprintf(stderr, "Unable to allocate %d bytes\n", length);
		return NULL;
	}

	read(handle, buffer, *length);
	close(handle);

	return buffer;
}

typedef struct {
	unsigned long unk0;
	unsigned long count;
	unsigned short unk1;
	unsigned short const1;	/* always 0xffff ? */
	unsigned short const2;  /* always 0xc5c5c5c5 ? */
} object6_t;

typedef struct {
	unsigned long unk0;
	unsigned long unk1;
	unsigned long unk2;
	unsigned long unk3;
} object6_0_t;

typedef struct {
	unsigned short unk0;
	unsigned short const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	long coord0[3];
	long coord1[3];
	unsigned long offset;
} object7_t;

typedef struct {
	unsigned short const0; /* 0xff01 */
	unsigned short cams;
	unsigned short unknown[8];
} object8elt_t;

typedef struct {
	unsigned short unknown[20];
} object9elt_t;

typedef struct {
	unsigned long offset0;
	unsigned long offset1;
} object10_t;

typedef struct {
	unsigned char unknown[12];
} object11elt_t;

typedef struct {
	unsigned long unk0;
	unsigned long unk1;
	unsigned long unk2;
} object12elt_t;

typedef struct {
	unsigned char block[8];
} object18hdr_t;

typedef struct {
	unsigned short count0;
	unsigned short count1;
	unsigned long unknown;
} object18blk_t;

typedef struct {
	unsigned char x;
	unsigned char y;
	char offset_x;
	char offset_y;
} object18spr_t;

typedef struct {
	char dummy0;
	char numObject7;
	char numObject10;
	char dummy1[5];
	unsigned long offsets[22];
} rdt_t;

object7_t	*obj7Ptr;
object10_t	*obj10Ptr;

const char *western_chars=
	" .___()_____0123"
	"456789:-,\"!?_ABC"
	"DEFGHIJKLMNOPQRS"
	"TUVWXYZ[/]'-_abc"
	"defghijklmnopqrs"
	"tuvwxyz_________";

void *getObjectPtr(void *source, unsigned long offset)
{
	if (offset==0) {
		return NULL;
	}
	return &((unsigned char *) source)[offset];
}

void printObject70(rdt_t *rdt, int obj_offset);

void printObject6(rdt_t *rdt, int obj_offset)
{
	int i, count;

	object6_t *obj6 = getObjectPtr((unsigned char *)rdt, obj_offset);
	count = obj6->count-1;
	printf(" object6 count: %d\n", count);
	if ((count<0) || (count>65536)) {
		return;
	}

	object6_0_t *obj6_0 = getObjectPtr((unsigned char *)obj6, sizeof(object6_t));
	for (i=0; i<count; i++) {
		printf(" object6[%02d]: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			i, obj6_0[i].unk0, obj6_0[i].unk1,
			obj6_0[i].unk2, obj6_0[i].unk3
		);
	}
}

void printObject7(rdt_t *rdt, int obj_offset)
{
	int i;

	object7_t *obj7 = getObjectPtr((unsigned char *)rdt, obj_offset);
	for (i=0; i<rdt->numObject7; i++) {
		printf(" object7[%02d].c0: %.3f,%.3f,%.3f\n",
			i, obj7[i].coord0[0]/256.0,
			obj7[i].coord0[1]/256.0, obj7[i].coord0[2]/256.0
		);
		printf(" object7[%02d].c1: %.3f,%.3f,%.3f\n",
			i, obj7[i].coord1[0]/256.0,
			obj7[i].coord1[1]/256.0, obj7[i].coord1[2]/256.0
		);
		printf(" object7[%02d].offset: 0x%08x\n",
			i, obj7[i].offset);

		printObject70(rdt, obj7[i].offset);
	}
}

void printObject70(rdt_t *rdt, int obj_offset)
{
	int i, j, offset = obj_offset+4;

	short *counts = getObjectPtr((unsigned char *)rdt, obj_offset);
	if (counts[0]>0) {
		printf("  object70.0: %d\n", counts[0]);
		for (i=0; i<counts[0]; i++) {
			unsigned short *obj = getObjectPtr((unsigned char *)rdt, offset);
			printf("   0x%04x 0x%04x 0x%04x 0x%04x\n",
				obj[0], obj[1], obj[2], obj[3]);
			
			offset += 8;
		}
	}
	if (counts[1]>0) {
		printf("  object70.1: %d\n", counts[1]);
		for (i=0; i<counts[1]; i++) {
			unsigned short *obj = getObjectPtr((unsigned char *)rdt, offset);
			int size = obj[3] ? 8 : 12;
			printf("  ");
			for (j=0; j<size>>1; j++) {
				printf(" %04x", obj[j]);
			}
			printf("\n");

			offset += size;
		}
	}
}

void printObject8(rdt_t *rdt, int obj_offset)
{
	int i, count;

	object8elt_t *obj8 = getObjectPtr((unsigned char *)rdt, obj_offset);
	count = (rdt->offsets[9] - obj_offset) / sizeof(object8elt_t);

	for (i=0; i<count; i++) {
		printf(" object8[%02d]\n", i);
		printf("  0x%04x,%d,%d\n",
			obj8[i].const0, obj8[i].cams & 0xff, obj8[i].cams >> 8
		);
		printf("  0x%04x,0x%04x,0x%04x,0x%04x\n",
			obj8[i].unknown[0], obj8[i].unknown[1],
			obj8[i].unknown[2], obj8[i].unknown[3]
		);
		printf("  0x%04x,0x%04x,0x%04x,0x%04x\n",
			obj8[i].unknown[4], obj8[i].unknown[5],
			obj8[i].unknown[6], obj8[i].unknown[7]
		);
	}
}

void printObject9(rdt_t *rdt, int obj_offset)
{
	int i, count = 9;

	object9elt_t *obj9 = getObjectPtr((unsigned char *)rdt, obj_offset);
	for (i=0; i<count; i++) {
		printf(" object9[%02d]\n", i);
		printf("  0x%04x,0x%04x\n",
			obj9[i].unknown[0], obj9[i].unknown[1]
		);
	}
}

void printObject10(rdt_t *rdt, int obj_offset)
{
	int i;

	object10_t *obj10 = getObjectPtr((unsigned char *)rdt, obj_offset);
	for (i=0; i<rdt->numObject10; i++) {
		printf(" object10[%02d]: 0x%08x, 0x%08x\n",
			i, obj10[i].offset0, obj10[i].offset1
		);
	}
}

void printObject11(rdt_t *rdt, int obj_offset)
{
	int i, j, count;

	unsigned short *obj11 = getObjectPtr((unsigned char *)rdt, obj_offset);
	count = *obj11;
	printf(" object11 count: %d\n", count);

	unsigned long *o = getObjectPtr((unsigned char *)rdt, obj_offset+2);
	for (i=0; i<count; i++) {
		printf("  object11[%d]: 0x%08x, 0x%08x, 0x%08x\n", i,
			o[0], o[1], o[2]
		);
		o += 3;
	}
}

void printObject12(rdt_t *rdt, int obj_offset)
{
	int i;

	unsigned long *obj12 = getObjectPtr((unsigned char *)rdt, obj_offset);
	printf(" object12 count: %d\n", *obj12);
	if (*obj12 > 65536) {
		return;
	}

	object12elt_t *obj12elt = getObjectPtr((unsigned char *)rdt, obj_offset+4);
	for (i=0; i<*obj12; i++) {
		printf(" object12[%02d]: 0x%08x, 0x%08x, 0x%08x\n",
			i,
			obj12elt[i].unk0,
			obj12elt[i].unk1,
			obj12elt[i].unk2
		);
	}
}

void printObject13(rdt_t *rdt, int obj_offset)
{
	int i, count;

	unsigned char *obj13 = getObjectPtr((unsigned char *)rdt, obj_offset);
	unsigned short *offsets = (unsigned short *) obj13;

	count = offsets[0]/2;
	printf(" object13 count: %d\n", count);
}

void printObject14(rdt_t *rdt, int obj_offset)
{
	int i, count;

	unsigned char *obj14 = getObjectPtr((unsigned char *)rdt, obj_offset);
	unsigned short *offsets = (unsigned short *) obj14;

	count = offsets[0]/2;
	printf(" object14 count: %d\n", count);
	for (i=0; i<count; i++) {
		unsigned char *pos = getObjectPtr((unsigned char *)rdt, obj_offset+offsets[i]);
		unsigned char c = *pos++;
		printf("  text[%d]: ", i);
		while (c!= 0xfe) {
			if ((c==0xfa) || (c==0xfd)) {
				/* Skip next */
				c = *pos++;
			} else if (c==0xfc) {
				/* CR */
				printf("<br>");
			} else if (c<0x60) {
				/*printf("%c[0x%02x]", western_chars[c], c);*/
				printf("%c", western_chars[c]);
			} else {
				printf("<0x%02x>", c);
			}
			c = *pos++;
		}
		printf("\n");
	}
}

void printObject18(rdt_t *rdt, int obj_offset)
{
	int i,j, offset = obj_offset+sizeof(object18hdr_t);

	object18hdr_t *p = getObjectPtr((unsigned char *)rdt, obj_offset);

	for (i=0; i<8; i++) {
		if (p->block[i]==0xff) {
			break;
		}

		printf(" object18[%02d]: 0x%02x\n", i, p->block[i]);

		object18blk_t *b = getObjectPtr((unsigned char *)rdt, offset);
		offset += sizeof(object18blk_t);

		printf("  objects: %d\n", b->count0);
		offset += b->count0*8;

		printf("  sprites: %d\n", b->count1);
		object18spr_t *s = getObjectPtr((unsigned char *)rdt, offset);
		for (j=0; j<b->count1; j++) {
			printf("   sprite[%02d]: %d,%d,%d,%d\n", j,
				s[j].x, s[j].y,
				s[j].offset_x, s[j].offset_y
			);
		}
		offset += b->count1*sizeof(object18spr_t);

		printf(" offset 0x%08x\n", offset);
		unsigned short *b0 = getObjectPtr((unsigned char *)rdt, offset);
		offset += 8*sizeof(unsigned short);
		for (j=0; j<8; j++) {
			if (b0[j]!=0) {
				offset += 0x38;
			}
		}

		offset += 4;

		printf(" offset 0x%08x\n", offset);
	}
}

void parseRoomDescription(rdt_t *rdt, int length)
{
	int i;

	printf("length: %d\n", length);

	for (i=0; i<23; i++) {
		printf("object %02d: 0x%08x\n", i, rdt->offsets[i]);
		if (rdt->offsets[i] == 0) {
			continue;
		}

		switch(i) {
/*			case 6:
				printObject6(rdt, rdt->offsets[i]);
				break;
*/
			case 7:
				printObject7(rdt, rdt->offsets[i]);
				break;
/*			case 8:
				printObject8(rdt, rdt->offsets[i]);
				break;
			case 9:
				printObject9(rdt, rdt->offsets[i]);
				break;
			case 10:
				printObject10(rdt, rdt->offsets[i]);
				break;
			case 11:
				printObject11(rdt, rdt->offsets[i]);
				break;
			case 12:
				printObject12(rdt, rdt->offsets[i]);
				break;
			case 13:
				printObject14(rdt, rdt->offsets[i]);
				break;
			case 14:
				printObject14(rdt, rdt->offsets[i]);
				break;
*/
/*
			case 18:
				printObject18(rdt, rdt->offsets[i]);
				break;
*/		}
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
	/*fileInMem = loadFile("/windows_d/Re2Demo/pl0/rdu/room1000.rdt", &length);*/
	fileInMem = loadFile(argv[1], &length);
	if (fileInMem==NULL) {
		return 1;
	}

	printf("Reading %s\n", argv[1]);
	parseRoomDescription((rdt_t *)fileInMem, length);

	free(fileInMem);

	return 0;
}
