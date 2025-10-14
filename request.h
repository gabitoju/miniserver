#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>
#include <stdlib.h>


typedef struct {
    char* method;
    char* path;
    char* version;
    char* host;
    char* user_agent;
    char* referer;
    int status;
    size_t bytes;
} Request;

Request parse_request(char* raw_request);

void free_request(Request* request);

#endif // REQUEST_H
