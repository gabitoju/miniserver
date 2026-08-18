#include "log.h"
#include "server.h"
#include "request.h"
#include <time.h>
#include <stdio.h>

static void log_safe_value(const char* value, char* output, size_t output_size) {
    size_t written = 0;
    if (value == NULL) {
        snprintf(output, output_size, "-");
        return;
    }
    for (size_t i = 0; value[i] != '\0' && written + 1 < output_size; i++) {
        unsigned char c = (unsigned char)value[i];
        output[written++] = (c >= 32 && c <= 126 && c != '"' && c != '\\') ? (char)c : '_';
    }
    output[written] = '\0';
}

void log_access_request(Server *server, Request *request, double duration_ms) {

    time_t now = time(0);
    char time_buff[100];
    strftime(time_buff, sizeof(time_buff), "%d/%b/%Y:%H:%M:%S %z", localtime(&now));

    char client_ip[256], method[64], path[512], version[64], referer[1024], user_agent[1024];
    log_safe_value(request->client_ip ? request->client_ip : "unknown", client_ip, sizeof(client_ip));
    log_safe_value(request->method, method, sizeof(method));
    log_safe_value(request->path, path, sizeof(path));
    log_safe_value(request->version, version, sizeof(version));
    log_safe_value(request->referer, referer, sizeof(referer));
    log_safe_value(request->user_agent, user_agent, sizeof(user_agent));

    printf("%s - - [%s] \"%s %s %s\" %d %zu \"%s\" \"%s\" request_id=%s duration_ms=%.2f\n",
        client_ip,
        time_buff,
        method,
        path,
        version,
        request->status,
        request->bytes,
        referer,
        user_agent,
        request->request_id[0] ? request->request_id : "-",
        duration_ms
    );


    if (server->access_log_file != NULL) {
        fprintf(server->access_log_file, "%s - - [%s] \"%s %s %s\" %d %zu \"%s\" \"%s\" request_id=%s duration_ms=%.2f\n",
            client_ip,
            time_buff,
            method,
            path,
            version,
            request->status,
            request->bytes,
            referer,
            user_agent,
            request->request_id[0] ? request->request_id : "-",
            duration_ms
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
