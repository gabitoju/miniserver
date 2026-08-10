#include "config.h"
#include "constants.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

void read_config(Config* config) {

    FILE* file = fopen(config->config_file, "r");

    if (file == NULL) {
        config->port = DEFAULT_PORT;
        config->content_path = ".";
    } else {
        char buffer[BUFFER_SIZE];
        char *saveptr;
        while ((fgets(buffer, BUFFER_SIZE, file)) != NULL) {

            if (buffer[0] == '#' || buffer[0] == '\n') {
                continue;
            }

            char* key = strtok_r(buffer, " ", &saveptr);
            char* svalue = strtok_r(NULL, " ", &saveptr);
            char* third_value = strtok_r(NULL, " ", &saveptr);
            if (key == NULL || svalue == NULL) {
                continue;
            }
            svalue[strcspn(svalue, "\n")] = 0;

            if (strcmp(key, "cgi_handler") == 0) {
                if (third_value != NULL && config->cgi_handler_count < MAX_CGI_HANDLERS) {
                    third_value[strcspn(third_value, "\n")] = 0;
                    CgiHandler* handler = &config->cgi_handlers[config->cgi_handler_count];
                    handler->extension = strdup(svalue);
                    handler->executable = strdup(third_value);
                    if (handler->extension != NULL && handler->executable != NULL) {
                        config->cgi_handler_count++;
                    } else {
                        free(handler->extension);
                        free(handler->executable);
                        handler->extension = NULL;
                        handler->executable = NULL;
                    }
                }
                continue;
            }

            char* endptr;
            errno = 0;
            long ivalue = strtol(svalue, &endptr, 10);
            
            if ((errno != ERANGE) && (endptr != svalue) && (*endptr == '\0')) {
                if (strcmp(key, "port") == 0) {
                    config->port = (int)ivalue;
                }
                if (strcmp(key, "max_body_size") == 0 && ivalue >= 0) {
                    config->max_body_size = (size_t)ivalue;
                }
            } else {
                if (strcmp(key, "content_path") == 0) {
                    char* content_path = malloc(strlen(svalue) + 1);
                    if (content_path) {
                        strcpy(content_path, svalue);
                        if (config->content_path_owned) {
                            free(config->content_path);
                        }
                        config->content_path = content_path;
                        config->content_path_owned = 1;
                    }
                }
                if (strcmp(key, "mime_types_path") == 0) {
                    config->mime_types_path = malloc(strlen(svalue) + 1);
                    if (config->mime_types_path) {
                        strcpy(config->mime_types_path, svalue);
                    }
                }
                if (strcmp(key, "access_log_path") == 0) {
                    config->access_log_path = malloc(strlen(svalue) + 1);
                    if (config->access_log_path) {
                        strcpy(config->access_log_path, svalue);
                    }
                }
                if (strcmp(key, "error_log_path") == 0) {
                    config->error_log_path = malloc(strlen(svalue) + 1);
                    if (config->error_log_path) {
                        strcpy(config->error_log_path, svalue);
                    }
                }
                if (strcmp(key, "real_ip_header") == 0) {
                    config->real_ip_header = malloc(strlen(svalue) + 1);
                    if (config->real_ip_header) {
                        strcpy(config->real_ip_header, svalue);
                    }
                }
            }
        }
        fclose(file);
    }
}

const char* config_cgi_handler(const Config* config, const char* path) {
    if (path == NULL) {
        return NULL;
    }

    const char* extension = strrchr(path, '.');
    if (extension == NULL) {
        return NULL;
    }

    for (int i = 0; i < config->cgi_handler_count; i++) {
        if (strcmp(extension, config->cgi_handlers[i].extension) == 0) {
            return config->cgi_handlers[i].executable;
        }
    }

    return NULL;
}

void config_destroy_cgi_handlers(Config* config) {
    for (int i = 0; i < config->cgi_handler_count; i++) {
        free(config->cgi_handlers[i].extension);
        free(config->cgi_handlers[i].executable);
    }
}
