#include "config.h"
#include "server.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

    Config config = {
        .port = DEFAULT_PORT,
        .content_path = ".",
        .config_file = CONFIG_FILE,
        .mime_types_path = MIME_TYPES_DATABASE,
        .access_log_path = ACCESS_LOG_PATH,
        .error_log_path = ERROR_LOG_PATH,
        .real_ip_header = REAL_IP_HEADER
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

    read_config(&config);

    Server server = {
        .config = &config
    };
    if (server_init(&server) != 0) {
        return 1;
    }

    server_run(&server);
    server_destroy(&server);
    return 0;
}
