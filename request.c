#define _GNU_SOURCE

#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Request parse_request(char* raw_request) {

    Request req;

    req.method = req.version = req.path = req.host = req.user_agent = req.referer = NULL;

    req.method = malloc(sizeof(char) * 16);
    req.path = malloc(sizeof(char) * 256);
    req.version = malloc(sizeof(char) * 16);

    if (req.method == NULL || req.path == NULL || req.version == NULL) {
        perror("malloc failed");
        free_request(&req);
        req.method = req.path = req.version = NULL;
        return req;
    }

    sscanf(raw_request, "%s %s %s", req.method, req.path, req.version);

    char* line = strstr(raw_request, "\r\n");

    if (!line) return req;

    char* header_start = line + 2;
    char* saveptr;
    line = strtok_r(header_start, "\r\n", &saveptr);


    while (line != NULL) {
        if (strncasecmp(line, "Host: ", 6) == 0) {
            req.host = malloc(1024);
            if (req.host) strcpy(req.host, line + 6);
        } else if (strncasecmp(line, "User-Agent: ", 12) == 0) {
            req.user_agent = malloc(1024);
            if (req.user_agent) strcpy(req.user_agent, line + 12);
        } else if (strncasecmp(line, "Referer: ", 9) == 0) {
            req.referer = malloc(1024);
            if (req.referer) strcpy(req.referer, line + 9);
        }
        line = strtok_r(NULL, "\r\n", &saveptr);
    }

    return req;    
}

void free_request(Request *request) {
    free(request->method);
    free(request->path);
    free(request->version);
    free(request->host);
    free(request->user_agent);
    free(request->referer);
}
