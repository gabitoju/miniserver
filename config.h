#ifndef CONFIG
#define CONFIG

#include <stdio.h>

typedef struct  {
    char* config_file;
    char* content_path;
    char* real_ip_header;
    char* mime_types_path;
    char* access_log_path;
    char* error_log_path;
} Config;

#endif // CONFIG
