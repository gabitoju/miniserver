#include <stdio.h>

#include "response.h"

size_t response_build_headers(
    char *buffer, 
    size_t buffer_size,
    const char* content_type,
    const char* status_line, 
    size_t content_length, 
    int connection_close, 
    const char** extra_headers, 
    size_t extra_count
) {
    size_t total = 0;

    int n = snprintf(buffer + total, buffer_size - total, "%s", status_line);

    total += (n > 0) ? n : 0;
    
    for (size_t i = 0; i < extra_count; i++) {
        n = snprintf(buffer + total, buffer_size - total, "%s\r\n", extra_headers[i]);
        total += (n > 0) ? n : 0;
    }

    n = snprintf(buffer + total, buffer_size - total, 
                "Content-Type: %s; charset=utf-8\r\n"
                "Content-Length: %zu\r\n"
                "Connection: %s\r\n"
                "\r\n",
                 content_type, content_length, connection_close ? "close ": "keep-alive"
                );

    total += (n > 0) ? n : 0;

    return total;
}
