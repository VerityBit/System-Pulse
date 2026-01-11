#include "controller.h"

#include <stdio.h>
#include <stdlib.h>

int read_file_to_buffer(const char *path, FileBuffer *out) {
    FILE *fp = NULL;
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    if (!out || !path) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }

    cap = 4096;
    buf = (char *)malloc(cap);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    for (;;) {
        size_t avail = cap - len;
        size_t nread = 0;

        if (avail == 0) {
            size_t new_cap = cap * 2;
            char *tmp = (char *)realloc(buf, new_cap);
            if (!tmp) {
                free(buf);
                fclose(fp);
                return -1;
            }
            buf = tmp;
            cap = new_cap;
            avail = cap - len;
        }

        nread = fread(buf + len, 1, avail, fp);
        len += nread;

        if (nread == 0) {
            if (feof(fp)) {
                break;
            }
            if (ferror(fp)) {
                free(buf);
                fclose(fp);
                return -1;
            }
        }
    }

    if (cap == len) {
        char *tmp = (char *)realloc(buf, cap + 1);
        if (!tmp) {
            free(buf);
            fclose(fp);
            return -1;
        }
        buf = tmp;
        cap += 1;
    }

    buf[len] = '\0';

    fclose(fp);

    out->data = buf;
    out->len = len;
    return 0;
}

void free_file_buffer(FileBuffer *buf) {
    if (!buf) {
        return;
    }
    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
}
