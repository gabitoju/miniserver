#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdio.h>

typedef struct  {
    int port;
    size_t max_body_size;
    char* config_file;
    char* content_path;
    char* real_ip_header;
    char* mime_types_path;
    char* access_log_path;
    char* error_log_path;
} Config;

void read_config(Config* config);

#endif // CONFIG_H
