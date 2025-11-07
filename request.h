#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>
#include <stdlib.h>
#include "config.h"

typedef struct {
    char* method;
    char* path;
    char* version;
    char* host;
    char* client_ip;
    char* real_ip;
    char* user_agent;
    char* referer;
    char* if_none_match;
    char* query_params;
    int status;
    size_t bytes;
} Request;

Request parse_request(Config* config, char* raw_request);

void free_request(Request* request);

#endif // REQUEST_H
