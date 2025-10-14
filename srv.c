#include "server.h"
#include "constants.h"
#include <stdio.h>
#include <string.h>

void read_config(Server* server) {
    FILE* file = fopen(CONFIG_FILE, "r");

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

int main(void) {

    Server server = {
        .port = DEFAULT_PORT,
        .content_path = "."
    };

    read_config(&server);

    if (server_init(&server) != 0) {
        return 1;
    }

    server_run(&server);
    server_destroy(&server);
    return 0;
}
