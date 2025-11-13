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
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "log.h"
#include "request.h"
#include "mime.h"
#include <signal.h>
#include <sys/wait.h>

#if defined(__linux__)
#include <sys/sendfile.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/uio.h>      // Provides sendfile() on macOS/BSD
#include <sys/socket.h>
#endif

volatile sig_atomic_t server_running = 1;

int send_all(int socket_fd, const char* buffer, size_t len);
void close_socket(int socket_fd);

ssize_t portable_sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    off_t len = count;
    int result = sendfile(in_fd, out_fd, *offset, &len, NULL, 0);
    if (result == -1) return -1;
    *offset += len;
    return len;
#elif defined(__linux__)
    return sendfile(out_fd, in_fd, offset, count);
#else
#error "sendfile not supported on this platform"
#endif
}


void int_handler(int dummy) {
    (void)dummy;
    printf("Handler");
    server_running = 0;
}

void sigchld_handler(int s) {
    (void)s;
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int server_init(Server * server) {
    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) ==  0) {
        perror("socket failed");
        return -1;
    }

    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(server->config->port);
    int val = 1;
    struct timeval tv = { 1, 0 };
    setsockopt(server->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Set socket options to allow immediate reuse of the address/port (this allows for faster shutdown)
    #ifdef SO_REUSEPORT
        setsockopt(server->fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
    #else
        setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    #endif

    if ((bind(server->fd, (struct sockaddr*)&server->address, sizeof(server->address))) < 0) {
        perror("bind failed");
        return -1;
    }

    if ((listen(server->fd, SOMAXCONN)) < 0) {
        perror("listen failed");
        return -1;
    }

    load_mime_database(server->config->mime_types_path);

    server->access_log_file = fopen(server->config->access_log_path, "a");
    server->error_log_file = fopen(server->config->error_log_path, "a");

    printf("Server listening on port %d\n", server->config->port);
    return 0;
}

void server_run(Server* server) {

    struct sigaction sa_int, sa_chld;
    sa_int.sa_handler = int_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction for SIGINIT failed");
        return;
    }

    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction for SIGCHLD failed");
        return;
    }

    int new_socket;
    socklen_t addrlen = sizeof(server->address);

    while (server_running) {
        if ((new_socket = accept(server->fd, (struct sockaddr*)&server->address, &addrlen)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (!server_running) {
                break;
            }
            perror("accept failed");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            close(new_socket);
            continue;
        } 

        if (pid == 0) {
            close(server->fd);
            char* client_ip = inet_ntoa(server->address.sin_addr);
            handle_connection(server, new_socket, client_ip);
            exit(0);
        } else {
            close(new_socket);
        }
    }
}

void server_destroy(Server* server) {
    free(server->config->content_path);
    free(server->config->mime_types_path);
    mime_destroy();
    close(server->fd);
    fclose(server->access_log_file);
    fclose(server->error_log_file);
}

void handle_connection(Server* server, int client_socket, const char* client_ip) {
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

    Request request = parse_request(server->config, buffer);

    if (request.real_ip) {
        char* comma = strchr(request.real_ip, ',');
        if (comma) {
            size_t len = comma - request.real_ip;
            request.client_ip = malloc(len + 1);
            strncpy(request.client_ip, request.real_ip, len);
            request.client_ip[len] = '\0';
        } else {
            request.client_ip = strdup(request.real_ip);
        }
    } else {
        request.client_ip = client_ip ? strdup(client_ip) : NULL;
    }

    if (request.method == NULL) {
        close_socket(client_socket);
        free_request(&request);
        return;
    }

    handle_request(server, &request, client_socket);

    log_access_request(server, &request);
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
    char* message = "403 Forbidden";
    sprintf(response, "%s 403 Forbidden\nContent-Type: text/plain\nContent-Length: %zu\nConnection: close\n\n%s", HTTP_VERSION, strlen(message), message);
    request->status = 403;
    request->bytes = strlen(message);

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 403 response.\n");
    }
}

void send_404_response(Request* request, int client_socket) {
    char response[BUFFER_SIZE];
    char* message = "404 Not Found";
    sprintf(response, "%s 404 Not Found\nContent-Type: text/plain\nContent-Length: %zu\nConnection: close\n\n%s", HTTP_VERSION, strlen(message), message);
    request->status = 404;
    request->bytes = strlen(message);

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 404 response.\n");
    }
}


void send_405_response(Request *request, int client_socket) {
    char response[BUFFER_SIZE];
    char* message = "405 Method Not Allowed";
    sprintf(response, "%s 405 Method Not Allowed\nALLOW:%s,%s\nContent-Type: text/plain\nContent-Length: %zu\nConnection: close\n\n%s", HTTP_VERSION, HTTP_GET, HTTP_HEAD, strlen(message), message);
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
        snprintf(full_path, sizeof(full_path), "%s/%s", server->config->content_path, INDEX);
    } else {
        snprintf(full_path, sizeof(full_path), "%s%s", server->config->content_path, url_path);
    }

    struct stat path_stats;
    if (stat(full_path, &path_stats) != 0) {
        send_404_response(request, client_socket);
        return;
    }

    char etag[256];
    sprintf(etag, "\"%llx-%llx\"", (unsigned long long)path_stats.st_mtime, (unsigned long long)path_stats.st_size);

    if (request->if_none_match != NULL && strcmp(request->if_none_match, etag) == 0) {
        send_304_not_modified_response(request, client_socket);
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
        if (strncmp(real_path, server->config->content_path, strlen(server->config->content_path)) != 0) {
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
    sprintf(response_header, "%s 200 OK\nContent-Type: %s\nContent-Length: %ld\nETag: %s\nConnection: close\n\n", HTTP_VERSION, file_type, file_size, etag);
    request->status = 200;
    request->bytes = file_size;


    if (send_all(client_socket, response_header, strlen(response_header)) == -1) {
        fprintf(stderr, "Error sending file response.\n");
    }

    if (strcmp(request->method, HTTP_GET) == 0) {
        int file_fd = fileno(file);
        off_t offset = 0;
        ssize_t total_sent = 0;

        while (total_sent < file_size) {
            ssize_t sent = portable_sendfile(client_socket, file_fd, &offset, file_size - total_sent);

            if (sent == -1) {
                perror("sendfile failed");
                break;
            }

            total_sent += sent;

            if (sent == 0) {
                // connection closed or done
                break;
            }
        }
    }
    fclose(file);
}

void send_301_redirect(Request *request, int client_socket, const char *new_location) {
    char response[BUFFER_SIZE];
    sprintf(response, "%s 301 Moved Permanently\nLocation:%s\nContent-Length: 0\nConnection: close\n\n", HTTP_VERSION, new_location);

    request->status = 301;
    request->bytes = 0;

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 301 response.\n");
    }
}

void send_304_not_modified_response(Request *request, int client_socket) {
    char response[BUFFER_SIZE];
    sprintf(response, "%s 304 Not Modified\nConnection: close\n\n", HTTP_VERSION);
    request->status = 304;
    request->bytes = 0;

    if (send_all(client_socket, response, strlen(response)) == -1) {
        fprintf(stderr, "Error sending 304 response.\n");
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
        close(socket_fd);
    }
}
