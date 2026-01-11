#ifndef SYSTEM_PULSE_CONTROLLER_H
#define SYSTEM_PULSE_CONTROLLER_H

#include <stddef.h>

typedef struct FileBuffer {
    char *data;
    size_t len;
    size_t cap;
} FileBuffer;

int read_file_to_buffer(const char *path, FileBuffer *out);
void free_file_buffer(FileBuffer *buf);

#endif
