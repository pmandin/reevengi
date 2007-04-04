#ifndef FILE_H
#define FILE_H

/* Load file in mem from filename, return buffer, update length */

char *loadFile(char *filename, int *length);

void saveFile(char *filename, void *buffer, int length);

#endif /* FILE_H */
