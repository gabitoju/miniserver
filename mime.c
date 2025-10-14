#include "mime.h"
#include "hashmap.h"
#include "constants.h"
#include <stdio.h>
#include <string.h>

HashMap* mime_types;

void load_mime_database() {
    FILE* mime_db = fopen("mime.types", "r");
    char line_buffer[BUFFER_SIZE];

    if (mime_db == NULL) {
        perror("error loading mime database");
        return;
    }

    mime_types = hashmap_create(1024);

    while (fgets(line_buffer, BUFFER_SIZE, mime_db)) {
        if (line_buffer[0] == '#' || line_buffer[0] == '\n') {
            continue;
        }

        char mime[256];
        char extensions[BUFFER_SIZE];

        sscanf(line_buffer, "%s  %s", mime, extensions);
        char* extension = strtok(extensions, ",");

        while (extension != NULL) {
            hashmap_set(mime_types, extension, strdup(mime));
            extension = strtok(NULL, ",");
        }
    }

    fclose(mime_db);
}

void mime_destroy() {
    hashmap_destroy(mime_types);
}

static const char* _file_extension(const char* file_path) {
    const char *dot = strrchr(file_path, '.');
    if(!dot || dot == file_path) return "";
    return dot + 1;
}

const char* file_mime_type(char* file_path) {
    const char* mime_type;

const char* extension = _file_extension(file_path);

    mime_type = hashmap_get(mime_types, extension);    
    return mime_type;
}
