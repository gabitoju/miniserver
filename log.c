#include "log.h"
#include "server.h"
#include "request.h"
#include <time.h>
#include <stdio.h>

void log_access_request(Server *server, Request *request) {

    time_t now = time(0);
    char time_buff[100];
    strftime(time_buff, sizeof(time_buff), "%d/%b/%Y:%H:%M:%S %z", localtime(&now));

    printf("%s - - [%s] \"%s %s %s\" %d %zu \"%s\" \"%s\"\n",
        request->host ? request->host : "unknown",
        time_buff,
        request->method,
        request->path,
        request->version,
        request->status,
        request->bytes,
        request->referer ? request->referer : "-",
        request->user_agent ? request->user_agent : "unknown"
    );


    if (server->access_log_file != NULL) {
        fprintf(server->access_log_file, "%s - - [%s] \"%s %s %s\" %d %zu \"%s\" \"%s\"\n",
            request->host ? request->host : "unknown",
            time_buff,
            request->method,
            request->path,
            request->version,
            request->status,
            request->bytes,
            request->referer ? request->referer : "-",
            request->user_agent ? request->user_agent : "unknown"
        );
        fflush(server->access_log_file);
    }
}

void log_error(Server *server, const char *error) {
    
    time_t now = time(0);
    char time_buff[100];
    strftime(time_buff, sizeof(time_buff), "%d/%b/%Y:%H:%M:%S %z", localtime(&now));

    fprintf(stderr, "[%s] %s", time_buff, error);

    if (server->error_log_file) {
        fprintf(server->error_log_file, "[%s] %s", time_buff, error);
        fflush(server->error_log_file);
    }
}
