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

void saveFile(char *filename, void *buffer, int length)
{
	int handle;

	/* Load file */
	handle = creat(filename, 0664);
	if (handle<0) {
		fprintf(stderr, "Unable to open %s\n", filename);	
		return;
	}

	write(handle, buffer, length);
	close(handle);
}
