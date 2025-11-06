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

    char* method_str = strtok_r(request_line, " ", &request_line);
    if (method_str) {
        req.method = malloc(MAX_METHOD_SIZE);
        if (req.method) {
            strncpy(req.method, method_str, MAX_METHOD_SIZE - 1);
        }
        req.method[MAX_METHOD_SIZE - 1] = '\0';
    }

    char* path_str = strtok_r(NULL, " ", &request_line);
    if (path_str) {
        char* query_start = strchr(path_str, '?');

        if (query_start != NULL) {
            size_t path_len = query_start - path_str;
            req.path = malloc(path_len + 1);
            if (req.path) {
                strncpy(req.path, path_str, path_len);
                req.path[path_len] = '\0';
            }

            char* query_str = query_start + 1;
            req.query_params = malloc(strlen(query_str) + 1);
            if (req.query_params) {
                strcpy(req.query_params, query_str);
            }
        } else {
            req.path = malloc(strlen(path_str) + 1);
            if (req.path) {
                strcpy(req.path, path_str);
            }
        }
    }

    char* version_str = strtok_r(NULL, " ", &request_line);
    if (version_str) {
        req.version = malloc(MAX_VERSION_SIZE);
        if (req.version) {
            strncpy(req.version, version_str, MAX_VERSION_SIZE - 1);
        }
        req.version[MAX_VERSION_SIZE - 1] = '\0';
    }

    char* line;
    while ((line = strtok_r(NULL, "\r\n", &raw_request)) != NULL) {
        if (strncasecmp(line, "Host: ", 6) == 0) {
            req.host = malloc(MAX_HEADER_SIZE);
            if (req.host) {
                strncpy(req.host, line + 6, MAX_HEADER_SIZE - 1);
            }
            req.host[MAX_HEADER_SIZE - 1] = '\0';
        } else if (strncasecmp(line, "User-Agent: ", 12) == 0) {
            req.user_agent = malloc(MAX_HEADER_SIZE);
            if (req.user_agent) {
                strncpy(req.user_agent, line + 12, MAX_HEADER_SIZE - 1);
            }
            req.user_agent[MAX_HEADER_SIZE - 1] = '\0';
        } else if (strncasecmp(line, "Referer: ", 9) == 0) {
            req.referer = malloc(MAX_HEADER_SIZE);
            if (req.referer) {
                strncpy(req.referer, line + 9, MAX_HEADER_SIZE - 1);
            }
            req.referer[MAX_HEADER_SIZE - 1] = '\0';
        } else if (strncasecmp(line, "If-None-Match: ", 15) == 0) {
            req.if_none_match = malloc(MAX_HEADER_SIZE);
            if (req.if_none_match) {
                strncpy(req.if_none_match, line + 15, MAX_HEADER_SIZE - 1);
            }
            req.if_none_match[MAX_HEADER_SIZE - 1] = '\0';
        } else if (strncmp(line, "X-Forwarded-For: ", 17) == 0) {
            req.x_forwarded_for = malloc(MAX_HEADER_SIZE);
            if (req.x_forwarded_for) {
                strncpy(req.x_forwarded_for, line + 17, MAX_HEADER_SIZE - 1);
            }
            req.x_forwarded_for[MAX_HEADER_SIZE - 1] = '\0';
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
    free(request->if_none_match);
    free(request->query_params);
    free(request->client_ip);
    free(request->x_forwarded_for);
}
