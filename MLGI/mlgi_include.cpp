#include "mlgi_include.h"

void copyFile(char *src, char *dst){
    FILE *SRC = fopen(src, "rb");
    FILE *DST = fopen(dst, "wb");

    char buf[DEFAULT_FILE_BUFLEN];
    size_t size;

    while (size = fread(buf, 1, DEFAULT_FILE_BUFLEN, SRC)) {
        fwrite(buf, 1, size, DST);
    }

    fclose(SRC);
    fclose(DST);
}