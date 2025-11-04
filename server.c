#include "constants.h"
#include "server.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include "request.h"
#include "mime.h"

#include <signal.h>

volatile sig_atomic_t server_running = 1;

int send_all(int socket_fd, const char* buffer, size_t len);
void close_socket(int socket_fd);

void int_handler(int dummy) {
    (void)dummy;
    printf("Handler");
    server_running = 0;
}


int server_init(Server * server) {
    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) ==  0) {
        perror("socket failed");
        return -1;
    }

    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(server->port);
    int val = 1;
    struct linger linger_opt = { 1, 0 };  // Enable linger with timeout 0

    // Set socket options to allow immediate reuse of the address/port (this allows for faster shutdown)
    #ifdef SO_REUSEPORT
        setsockopt(server->fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
    #else
        setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    #endif
    // Set linger to force close the connection immediately (this allows for faster shutdown)
    setsockopt(server->fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));

    if ((bind(server->fd, (struct sockaddr*)&server->address, sizeof(server->address))) < 0) {
        perror("bind failed");
        return -1;
    }

    if ((listen(server->fd, 10)) < 0) {
        perror("listen failed");
        return -1;
    }

    load_mime_database(server->mime_types_path);

    printf("Server listening on port %d\n", server->port);
    return 0;
}

void server_run(Server* server) {

    struct sigaction sa;
    sa.sa_handler = int_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        return;
    }

    int new_socket;
    socklen_t addrlen = sizeof(server->address);

    while (server_running) {
        if ((new_socket = accept(server->fd, (struct sockaddr*)&server->address, &addrlen)) < 0) {
            if (!server_running) {
                break;
            }
            perror("accept failed");
            continue;
        }

        handle_connection(server, new_socket);
    }
}

void server_destroy(Server* server) {
    free(server->content_path);
    free(server->mime_types_path);
    mime_destroy();
    close(server->fd);
}

void handle_connection(Server* server, int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    size_t total_received = 0;
    size_t remaining = 0;
    while (total_received < BUFFER_SIZE) {
        remaining = BUFFER_SIZE - total_received;
        ssize_t bytes_received = recv(client_socket,
                                     buffer + total_received,
                                     remaining,
                                     0);

        if (bytes_received <= 0) {
            close_socket(client_socket);
            return;
        }

        total_received += (size_t)bytes_received;

        if ((size_t)bytes_received < remaining) {
            break;
        }
    }

    Request request = parse_request(buffer);

    if (request.method == NULL) {
        close_socket(client_socket);
        free_request(&request);
        return;
    }

    handle_request(server, &request, client_socket);

    log_request(&request);
    free_request(&request);
    close_socket(client_socket);
}

void handle_request(Server* server, Request *request, int client_socket) {
    if (strcmp(request->method, HTTP_GET) == 0 || strcmp(request->method, HTTP_HEAD) == 0) {
        send_file_response(server, request, client_socket, request->path);
        return;
    }
    send_405_response(request, client_socket);
}

void send_403_response(Request* request, int client_socket) {
    char response[BUFFER_SIZE];
    char* message = "403 FORBIDDEN";
    sprintf(response, "%s 403 FORBIDDEN\nContent-Type: text/plain\nContent-Length: %zu\n\n%s", HTTP_VERSION, strlen(message), message);
    request->status = 403;
    request->bytes = strlen(message);

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 403 response.\n");
    }
}

void send_404_response(Request* request, int client_socket) {
    char response[BUFFER_SIZE];
    char* message = "404 NOT FOUND";
    sprintf(response, "%s 404 NOT FOUND\nContent-Type: text/plain\nContent-Length: %zu\n\n%s", HTTP_VERSION, strlen(message), message);
    request->status = 404;
    request->bytes = strlen(message);

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 404 response.\n");
    }
}


void send_405_response(Request *request, int client_socket) {
    char response[BUFFER_SIZE];
    char* message = "405 METHOD NOT ALLOWED";
    sprintf(response, "%s 405 METHOD NOT ALLOWED\nALLOW:%s,%s\nContent-Type: text/plain\nContent-Length: %zu\n\n%s", HTTP_VERSION, HTTP_GET, HTTP_HEAD, strlen(message), message);
    request->status = 405;
    request->bytes = strlen(message);

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 405 response.\n");
    }
}


void send_file_response(Server* server, Request* request, int client_socket, const char* url_path) {

    if (strstr(url_path, "/../") != NULL) {
        send_403_response(request, client_socket);
        return;
    }
    char full_path[BUFFER_SIZE];

    if (strcmp(url_path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "%s/%s", server->content_path, INDEX);
    } else {
        snprintf(full_path, sizeof(full_path), "%s%s", server->content_path, url_path);
    }

    struct stat path_stats;
    if (stat(full_path, &path_stats) != 0) {
        send_404_response(request, client_socket);
        return;
    }

    if (S_ISDIR(path_stats.st_mode)) {

        size_t path_len = strlen(url_path);

        if (path_len > 0 && url_path[path_len - 1] != '/') {
            char new_location[BUFFER_SIZE];
            snprintf(new_location, sizeof(new_location), "%s/", url_path);
            send_301_redirect(request, client_socket, new_location);
            return;
        } else {
            strncat(full_path, INDEX_SLASH, sizeof(full_path)- strlen(full_path) - 1);
        }
    }

    FILE *file = fopen(full_path, "rb");
    if (file == NULL) {
        send_404_response(request, client_socket);
        return;
    }

    char* real_path = realpath(full_path, NULL);

    if (real_path != NULL) {
        if (strncmp(real_path, server->content_path, strlen(server->content_path)) != 0) {
            free(real_path);
            fclose(file);
            send_403_response(request, client_socket);
            return;
        }
    }
    else {
        send_404_response(request, client_socket);
        fclose(file);
        return;
    }
    free(real_path);

    const char* file_type = file_mime_type(full_path);
    if (file_type == NULL) {
        file_type = "application/octet-stream";
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char response_header[BUFFER_SIZE];
    sprintf(response_header, "%s 200 OK\nContent-Type: %s\nContent-Length: %ld\n\n", HTTP_VERSION, file_type, file_size);
    request->status = 200;
    request->bytes = file_size;


    if (send_all(client_socket, response_header, strlen(response_header)) == -1) {
        fprintf(stderr, "Error sending file response.\n");
    }

    if (strcmp(request->method, HTTP_GET) == 0) {
        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            if (send_all(client_socket, buffer, bytes_read) == -1) {
                fprintf(stderr, "Error sending file response.\n");
                break;
            }
        }
    }
    fclose(file);
}

void send_301_redirect(Request *request, int client_socket, const char *new_location) {
    char response[BUFFER_SIZE];
    sprintf(response, "%s 301 MOVED PERMANENTLY\nLocation:%s\nContent-Length: 0\n\n", HTTP_VERSION, new_location);

    request->status = 301;
    request->bytes = 0;

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 301 response.\n");
    }
}

int send_all(int socket_fd, const char* buffer, size_t len) {
    size_t total_sent = 0;
    ssize_t bytes_sent;

    while (total_sent < len) {
        bytes_sent = write(socket_fd, buffer + total_sent, len - total_sent);

        if (bytes_sent == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("write failed in send_all");
                return -1;
            }
        }

        if (bytes_sent == 0 && len - total_sent > 0) {
            fprintf(stderr, "send_all: Peer closed connection unexpectedly.\n");
            return -1;
        }

        total_sent += (size_t)bytes_sent;
    }
    return 0;
}

void log_request(Request* request) {
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
}

void close_socket(int socket_fd) {
    if (socket_fd >= 0) {
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    }
}
