#define _GNU_SOURCE

#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_METHOD_SIZE 16
#define MAX_PATH_SIZE 256
#define MAX_VERSION_SIZE 16
#define MAX_HEADER_SIZE 1024

Request parse_request(char* raw_request) {
    Request req = {0}; // Initialize all fields to NULL/0

    char* request_line = strtok_r(raw_request, "\r\n", &raw_request);
    if (request_line == NULL) {
        return req;
    }

    // Parse Method
    char* method_str = strtok_r(request_line, " ", &request_line);
    if (method_str) {
        req.method = malloc(MAX_METHOD_SIZE);
        if (req.method) strncpy(req.method, method_str, MAX_METHOD_SIZE - 1);
        req.method[MAX_METHOD_SIZE - 1] = '\0';
    }

    // Parse Path
    char* path_str = strtok_r(NULL, " ", &request_line);
    if (path_str) {
        req.path = malloc(MAX_PATH_SIZE);
        if (req.path) strncpy(req.path, path_str, MAX_PATH_SIZE - 1);
        req.path[MAX_PATH_SIZE - 1] = '\0';
    }

    // Parse Version
    char* version_str = strtok_r(NULL, " ", &request_line);
    if (version_str) {
        req.version = malloc(MAX_VERSION_SIZE);
        if (req.version) strncpy(req.version, version_str, MAX_VERSION_SIZE - 1);
        req.version[MAX_VERSION_SIZE - 1] = '\0';
    }

    // Parse Headers
    char* line;
    while ((line = strtok_r(NULL, "\r\n", &raw_request)) != NULL) {
        if (strncasecmp(line, "Host: ", 6) == 0) {
            req.host = malloc(MAX_HEADER_SIZE);
            if (req.host) strncpy(req.host, line + 6, MAX_HEADER_SIZE - 1);
            req.host[MAX_HEADER_SIZE - 1] = '\0';
        } else if (strncasecmp(line, "User-Agent: ", 12) == 0) {
            req.user_agent = malloc(MAX_HEADER_SIZE);
            if (req.user_agent) strncpy(req.user_agent, line + 12, MAX_HEADER_SIZE - 1);
            req.user_agent[MAX_HEADER_SIZE - 1] = '\n';
        } else if (strncasecmp(line, "Referer: ", 9) == 0) {
            req.referer = malloc(MAX_HEADER_SIZE);
            if (req.referer) strncpy(req.referer, line + 9, MAX_HEADER_SIZE - 1);
            req.referer[MAX_HEADER_SIZE - 1] = '\0';
        }
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