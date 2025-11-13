#include "config.h"
#include "constants.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
            svalue[strcspn(svalue, "\n")] = 0;
            char* endptr;
            errno = 0;
            long ivalue = strtol(svalue, &endptr, 10);
            
            if ((errno != ERANGE) && (endptr != svalue) && (*endptr == '\0')) {
                if (strcmp(key, "port") == 0) {
                    config->port = (int)ivalue;
                }
            } else {
                if (strcmp(key, "content_path") == 0) {
                    config->content_path = malloc(strlen(svalue) + 1);
                    if (config->content_path) {
                        strcpy(config->content_path, svalue);
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
