#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

typedef struct {
    int status;
    size_t bytes;
} Response;

size_t response_build_headers(char* buffer, size_t buffer_size, const char* content_type, const char* status_line, size_t content_length, int close_connection, const char** extra_headers, size_t extra_count);

#endif // !RESPONSE_H
