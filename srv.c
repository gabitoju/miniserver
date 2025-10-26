#include "server.h"
#include "constants.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void read_config(Server* server) {
    FILE* file = fopen(server->config_file, "r");

    if (file == NULL) {
        server->port = DEFAULT_PORT;
        server->content_path = ".";
    } else {
        char buffer[BUFFER_SIZE];
        while ((fgets(buffer, BUFFER_SIZE, file)) != NULL) {

            if (buffer[0] == '#' || buffer[0] == '\n') {
                continue;
            }

            char key[64];
            int ivalue;
            char svalue[BUFFER_SIZE];

            if (sscanf(buffer, "%s %d", key, &ivalue) == 2) {
                if (strcmp(key, "port") == 0) {
                    server->port = ivalue;
                }
            }
            if (sscanf(buffer, "%s %s", key, svalue) == 2) {
                if (strcmp(key, "content_path") == 0) {
                    server->content_path = malloc(strlen(svalue) + 1);
                    if (server->content_path) {
                        strcpy(server->content_path, svalue);
                    }
                }
            }
        }
        fclose(file);
    }

}

int main(int argc, char* argv[]) {

    Server server = {
        .port = DEFAULT_PORT,
        .content_path = ".",
        .config_file = CONFIG_FILE 
    };
    char c;

    while ((c = getopt(argc, argv, "c:")) != -1) {
        switch (c) {
            case 'c':
                server.config_file = optarg;
                break;
        }
    }

    printf("%s\n", server.config_file);

    read_config(&server);

    if (server_init(&server) != 0) {
        return 1;
    }

    server_run(&server);
    server_destroy(&server);
    return 0;
}
