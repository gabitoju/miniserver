#include "config.h"
#include "server.h"
#include "constants.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void read_config(Server* server) {
    FILE* file = fopen(server->config->config_file, "r");

    if (file == NULL) {
        server->port = DEFAULT_PORT;
        server->config->content_path = ".";
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
                    server->port = (int)ivalue;
                }
            } else {
                if (strcmp(key, "content_path") == 0) {
                    server->config->content_path = malloc(strlen(svalue) + 1);
                    if (server->config->content_path) {
                        strcpy(server->config->content_path, svalue);
                    }
                }
                if (strcmp(key, "mime_types_path") == 0) {
                    server->config->mime_types_path = malloc(strlen(svalue) + 1);
                    if (server->config->mime_types_path) {
                        strcpy(server->config->mime_types_path, svalue);
                    }
                }
                if (strcmp(key, "access_log_path") == 0) {
                    server->config->access_log_path = malloc(strlen(svalue) + 1);
                    if (server->config->access_log_path) {
                        strcpy(server->config->access_log_path, svalue);
                    }
                }
                if (strcmp(key, "error_log_path") == 0) {
                    server->config->error_log_path = malloc(strlen(svalue) + 1);
                    if (server->config->error_log_path) {
                        strcpy(server->config->error_log_path, svalue);
                    }
                }
                if (strcmp(key, "real_ip_header") == 0) {
                    server->config->real_ip_header = malloc(strlen(svalue) + 1);
                    if (server->config->real_ip_header) {
                        strcpy(server->config->real_ip_header, svalue);
                    }
                }
            }
        }
        fclose(file);
    }
}

int main(int argc, char* argv[]) {

    Config config = {
        .content_path = ".",
        .config_file = CONFIG_FILE,
        .mime_types_path = MIME_TYPES_DATABASE,
        .access_log_path = ACCESS_LOG_PATH,
        .error_log_path = ERROR_LOG_PATH,
        .real_ip_header = REAL_IP_HEADER
    };

    Server server = {
        .port = DEFAULT_PORT,
        .config = &config
    };
    char c;

    while ((c = getopt(argc, argv, "c:")) != -1) {
        switch (c) {
            case 'c':
                config.config_file = optarg;
                break;
        }
    }

    printf("%s\n", config.config_file);

    read_config(&server);

    if (server_init(&server) != 0) {
        return 1;
    }

    server_run(&server);
    server_destroy(&server);
    return 0;
}
