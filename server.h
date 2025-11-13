#ifndef SERVER
#define SERVER

#include "request.h"
#include "config.h"
#include <netinet/in.h>
#include <stdio.h>


typedef struct {
    int fd;
    struct sockaddr_in address;
    Config* config;
    FILE* access_log_file;
    FILE* error_log_file;
} Server;

int server_init(Server* server);
void server_run(Server* server);
void server_destroy(Server* server);
void handle_connection(Server* server, int client_socket, const char* client_ip);
void handle_request(Server* server, Request* request, int client_socket);
void log_request(Request *request);

char* process_file(FILE *file);
void send_file_response(Server* server, Request* request, int client_socket, const char* file_path);
void send_301_redirect(Request* request, int client_socket, const char* new_location);
void send_304_not_modified_response(Request* request, int client_socket);
void send_403_response(Request* request, int client_socket);
void send_404_response(Request* request, int client_socket);
void send_405_response(Request* request, int client_socket);

#endif // SERVER
