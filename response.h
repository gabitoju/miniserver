#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int status;
    size_t bytes;
} Response;

size_t response_build_headers(char* buffer, size_t buffer_size, const char* content_type, const char* status_line, size_t content_length, int close_connection, const char** extra_headers, size_t extra_count, const char* request_id);
void build_etag(char* etag, char* header, size_t etag_size, size_t header_size, uint64_t mtime, uint64_t size);

#endif // !RESPONSE_H
