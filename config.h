#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdio.h>

#define MAX_CGI_HANDLERS 16

typedef struct {
    char* extension;
    char* executable;
} CgiHandler;

typedef struct  {
    int port;
    size_t max_body_size;
    char* config_file;
    char* content_path;
    int content_path_owned;
    char* real_ip_header;
    char* mime_types_path;
    char* access_log_path;
    char* error_log_path;
    CgiHandler cgi_handlers[MAX_CGI_HANDLERS];
    int cgi_handler_count;
} Config;

void read_config(Config* config);
const char* config_cgi_handler(const Config* config, const char* path);
void config_destroy_cgi_handlers(Config* config);

#endif // CONFIG_H
