#include <stdio.h>
#include <stdlib.h>

char *file_read(const char* name, size_t* size) {
    FILE *fp = fopen(name, "rb");
    if (!fp) return NULL;

    char *buffer = NULL;
    size_t capacity = 0, length = 0;

    while (1) {
        if (length + 4096 + 1 > capacity) {
            size_t cap = capacity + 4097;
            char *buf = realloc(buffer, cap);
            if (!buf) {
                free(buffer);
                fclose(fp);
                return NULL;
            }
            buffer = buf;
            capacity = cap;
        }

        size_t read = fread(buffer + length, 1, 4096, fp);
        length += read;
        if (read < 4096) break;
    }

    fclose(fp);

    if (buffer) buffer[length] = '\0';
    if (size) *size = length;

    return buffer;
}
