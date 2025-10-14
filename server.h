#ifndef SERVER
#define SERVER

#include "request.h"
#include <netinet/in.h>
#include <stdio.h>


typedef struct {
    int fd;
    int port;
    struct sockaddr_in address;
    char* content_path;
} Server;

int server_init(Server* server);
void server_run(Server* server);
void server_destroy(Server* server);
void handle_connection(Server* server, int client_socket);
void handle_request(Server* server, Request* request, int client_socket);
void log_request(Request *request);

char* process_file(FILE *file);
void send_file_response(Server* server, Request* request, int client_socket, const char* file_path);
void send_404_response(Request* request, int client_socket);
void send_403_response(Request* request, int client_socket);
void send_301_redirect(Request* request, int client_socket, const char* new_location);

#endif // SERVER
