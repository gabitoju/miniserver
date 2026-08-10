#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "constants.h"
#include "server.h"
#include "log.h"
#include "request.h"
#include "mime.h"
#include "cache.h"
#include "cgi.h"
#include "response.h"

#if defined(__linux__)
#include <sys/sendfile.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/uio.h>      // Provides sendfile() on macOS/BSD
#include <sys/socket.h>
#endif

static const char* NO_EXTRA[1] = { NULL };

volatile sig_atomic_t server_running = 1;

static int send_response(Request* request, int client_socket, int status_code, const char* status_line, const char* content_type, const char* body, size_t body_len, const char* extra_headers[], size_t extra_count);
static int send_all(int socket_fd, const char* buffer, size_t len);
static void end_connection(Request* request, int client_socket);
static void close_socket(int socket_fd);
static const char* find_header_end(const char* buffer, size_t length);
static void handle_post_request(Request* request, int client_socket);
static int handle_cgi(Server* server, Request* request, int client_socket, const char* executable);

static void send_cached_file(Request* request, int client_socket, FileCache* cached_item);

static const char* find_header_end(const char* buffer, size_t length) {
    if (length < 4) {
        return NULL;
    }

    for (size_t i = 0; i <= length - 4; i++) {
        if (memcmp(buffer + i, "\r\n\r\n", 4) == 0) {
            return buffer + i + 4;
        }
    }

    return NULL;
}

static ssize_t portable_sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
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


static void int_handler(int dummy) {
    (void)dummy;
    printf("Handler");
    server_running = 0;
}

static void sigchld_handler(int s) {
    (void)s;
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int server_init(Server * server) {
    char* content_path = realpath(server->config->content_path, NULL);
    if (content_path == NULL) {
        perror("content path failed");
        return -1;
    }
    if (server->config->content_path_owned) {
        free(server->config->content_path);
    }
    server->config->content_path = content_path;
    server->config->content_path_owned = 1;

    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }

    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(server->config->port);
    int val = 1;
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

    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        if ((new_socket = accept(server->fd, (struct sockaddr*)&client_addr, &addrlen)) < 0) {
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
            char* client_ip = inet_ntoa(client_addr.sin_addr);
            handle_connection(server, new_socket, client_ip);
            exit(0);
        } else {
            close(new_socket);
        }
    }
}

void server_destroy(Server* server) {
    if (server->config->content_path_owned) {
        free(server->config->content_path);
    }
    free(server->config->mime_types_path);
    config_destroy_cgi_handlers(server->config);
    mime_destroy();
    cache_destroy(server->cache);
    close(server->fd);
    fclose(server->access_log_file);
    fclose(server->error_log_file);
}

void handle_connection(Server* server, int client_socket, const char* client_ip) {

    struct timeval tv = { 5, 0 };
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
   
    int requests_served = 0;
    char buffer[BUFFER_SIZE];
    size_t total_received = 0;

    while ((++requests_served) < MAX_KEEPALIVE_REQUESTS) {
        const char* header_end = find_header_end(buffer, total_received);

        while (header_end == NULL && total_received < sizeof(buffer) - 1) {
            size_t remaining = sizeof(buffer) - 1 - total_received;
            ssize_t bytes_received = recv(client_socket, buffer + total_received, remaining, 0);

            if (bytes_received <= 0) {
                close_socket(client_socket);
                return;
            }

            total_received += (size_t)bytes_received;

            header_end = find_header_end(buffer, total_received);
            if (header_end != NULL) {
                break;
            }
        }

        if (header_end == NULL) {
            Request request = { .close_connection = 1 };
            send_400_response(&request, client_socket);
            close_socket(client_socket);
            return;
        }

        size_t header_length = (size_t)(header_end - buffer);
        size_t body_received = total_received - header_length;
        buffer[header_length - 4] = '\0';

        Request request = parse_request(server->config, buffer);
        if (request.bad_request) {
            request.close_connection = 1;
            send_400_response(&request, client_socket);
            free_request(&request);
            return;
        }

        if (request_is_path_traversal(&request)) {
            send_403_response(&request, client_socket);
            free_request(&request);
            return;
        }

        if (request.content_length > server->config->max_body_size) {
            request.close_connection = 1;
            send_413_response(&request, client_socket);
            free_request(&request);
            close_socket(client_socket);
            return;
        }

        if (request.content_length > 0) {
            request.body = malloc(request.content_length + 1);
            if (request.body == NULL) {
                request.close_connection = 1;
                log_error(server, "Unable to allocate request body.\n");
                send_500_response(&request, client_socket);
                free_request(&request);
                close_socket(client_socket);
                return;
            }

            size_t body_to_copy = body_received < request.content_length ? body_received : request.content_length;
            memcpy(request.body, header_end, body_to_copy);

            size_t body_remaining = request.content_length - body_to_copy;
            size_t body_offset = body_to_copy;
            while (body_remaining > 0) {
                ssize_t bytes_received = recv(client_socket, request.body + body_offset, body_remaining, 0);
                if (bytes_received <= 0) {
                    close_socket(client_socket);
                    free_request(&request);
                    return;
                }

                body_offset += (size_t)bytes_received;
                body_remaining -= (size_t)bytes_received;
            }
            request.body[request.content_length] = '\0';
        }

        if (body_received > request.content_length) {
            total_received = body_received - request.content_length;
            memmove(buffer, header_end + request.content_length, total_received);
        } else {
            total_received = 0;
        }

        request_resolve_ip(&request, client_ip);

        if (request.method == NULL) {
            end_connection(&request, client_socket);
            return;
        }

        handle_request(server, &request, client_socket);
        log_access_request(server, &request);
        free_request(&request);

        if (request.close_connection) {
            break;
        }
    }
    close_socket(client_socket);
}

void handle_request(Server* server, Request *request, int client_socket) {
    const char* cgi_executable = config_cgi_handler(server->config, request->path);
    if (cgi_executable != NULL && (strcmp(request->method, HTTP_GET) == 0 || strcmp(request->method, HTTP_HEAD) == 0 || strcmp(request->method, HTTP_POST) == 0)) {
        if (handle_cgi(server, request, client_socket, cgi_executable) == -1) {
            log_error(server, "Unable to handle CGI request.\n");
            request->close_connection = 1;
            send_500_response(request, client_socket);
        }
        return;
    }

    if (strcmp(request->method, HTTP_GET) == 0 || strcmp(request->method, HTTP_HEAD) == 0) {
        send_file_response(server, request, client_socket, request->path);
        return;
    }

    if (strcmp(request->method, HTTP_POST) == 0) {
        handle_post_request(request, client_socket);
        return;
    }

    send_405_response(request, client_socket);
}

static int handle_cgi(Server* server, Request* request, int client_socket, const char* executable) {
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s%s", server->config->content_path, request->path);

    char* script_path = realpath(full_path, NULL);
    if (script_path == NULL) {
        send_404_response(request, client_socket);
        return 0;
    }

    if (strncmp(script_path, server->config->content_path, strlen(server->config->content_path)) != 0) {
        free(script_path);
        send_403_response(request, client_socket);
        return 0;
    }

    struct stat path_stats;
    if (stat(script_path, &path_stats) != 0 || !S_ISREG(path_stats.st_mode)) {
        free(script_path);
        send_404_response(request, client_socket);
        return 0;
    }

    int result = cgi_handle_request(request, client_socket, script_path, executable, server->config->port);
    free(script_path);
    return result;
}

static void handle_post_request(Request* request, int client_socket) {
    if (send_response(request, client_socket, HTTP_200_CODE, HTTP_200_STATUS_LINE, TEXT_CONTENT_TYPE, NULL, 0, NO_EXTRA, 0) == -1) {
        fprintf(stderr, "Error sending POST response.\n");
    }
}

void send_400_response(Request* request, int client_socket) {
    if (send_response(request, client_socket, HTTP_400_CODE, HTTP_400_STATUS_LINE, TEXT_CONTENT_TYPE, HTTP_400_MESSAGE, HTTP_400_MESSAGE_LEN, NO_EXTRA, 0) == -1) {
        fprintf(stderr, "Error sending 400 response.\n");
    }
}

void send_413_response(Request* request, int client_socket) {
    if (send_response(request, client_socket, HTTP_413_CODE, HTTP_413_STATUS_LINE, TEXT_CONTENT_TYPE, HTTP_413_MESSAGE, HTTP_413_MESSAGE_LEN, NO_EXTRA, 0) == -1) {
        fprintf(stderr, "Error sending 413 response.\n");
    }
}

void send_500_response(Request* request, int client_socket) {
    if (send_response(request, client_socket, HTTP_500_CODE, HTTP_500_STATUS_LINE, TEXT_CONTENT_TYPE, HTTP_500_MESSAGE, HTTP_500_MESSAGE_LEN, NO_EXTRA, 0) == -1) {
        fprintf(stderr, "Error sending 500 response.\n");
    }
}

void send_403_response(Request* request, int client_socket) {
    if (send_response(request, client_socket, HTTP_403_CODE, HTTP_403_STATUS_LINE, TEXT_CONTENT_TYPE, HTTP_403_MESSAGE, HTTP_403_MESSAGE_LEN, NO_EXTRA, 0) == -1) {
        fprintf(stderr, "Error sending 403 response.\n");
    }
}

void send_404_response(Request* request, int client_socket) {
    if (send_response(request, client_socket, HTTP_404_CODE, HTTP_404_STATUS_LINE, TEXT_CONTENT_TYPE, HTTP_404_MESSAGE, HTTP_404_MESSAGE_LEN, NO_EXTRA, 0) == -1) {
        fprintf(stderr, "Error sending 404 response.\n");
        return;
    }
}

void send_405_response(Request *request, int client_socket) {
    const char* extra_header[] = { HTTP_405_EXTRA_HEADER };
    if (send_response(request, client_socket, HTTP_405_CODE, HTTP_405_STATUS_LINE, TEXT_CONTENT_TYPE, HTTP_405_MESSAGE, HTTP_405_MESSAGE_LEN, extra_header, 1) == -1) {
        fprintf(stderr, "Error sending 405 response.\n");
    }
}

void send_301_response(Request *request, int client_socket, const char *new_location) {
    char location[BUFFER_SIZE];
    snprintf(location, BUFFER_SIZE, "Location: %s", new_location);
    const char* extra_header[] = { location };

    if (send_response(request, client_socket, HTTP_301_CODE, HTTP_301_STATUS_LINE, TEXT_CONTENT_TYPE, NULL, 0, extra_header, 1) == -1) {
        fprintf(stderr, "Error sending 301 response.\n");
    }
}

void send_304_response(Request *request, int client_socket) {
    if (send_response(request, client_socket, HTTP_304_CODE, HTTP_304_STATUS_LINE, TEXT_CONTENT_TYPE, NULL, 0, NO_EXTRA, 0) == -1) {
        fprintf(stderr, "Error sending 304 response.\n");
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

    FileCache* cached_item = cache_get(server->cache, url_path);
    if (cached_item != NULL) {
        send_cached_file(request, client_socket, cached_item);
        return;
    }

    struct stat path_stats;
    if (stat(full_path, &path_stats) != 0) {
        send_404_response(request, client_socket);
        return;
    }

    char etag[ETAG_SIZE];
    char etag_header[BUFFER_SIZE];
    build_etag(etag, etag_header, ETAG_SIZE, BUFFER_SIZE, path_stats.st_mtime, path_stats.st_size);
    if (request->if_none_match != NULL && strcmp(request->if_none_match, etag) == 0) {
        send_304_response(request, client_socket);
        return;
    }

    if (S_ISDIR(path_stats.st_mode)) {

        size_t path_len = strlen(url_path);

        if (path_len > 0 && url_path[path_len - 1] != '/') {
            char new_location[BUFFER_SIZE];
            snprintf(new_location, BUFFER_SIZE, "%s/", url_path);
            send_301_response(request, client_socket, new_location);
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
    } else {
        send_404_response(request, client_socket);
        fclose(file);
        return;
    }
    free(real_path);

    const char* file_type = file_mime_type(full_path);
    if (file_type == NULL) {
        file_type = DEFAULT_MIME;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    fclose(file);

    char response_header[BUFFER_SIZE];
    const char* extra_headers[] = { etag_header };

    response_build_headers(response_header, BUFFER_SIZE, file_type, HTTP_200_STATUS_LINE, file_size, request->close_connection, extra_headers, 1);

    request->status = HTTP_200_CODE;

    if (send_all(client_socket, response_header, strlen(response_header)) == -1) {
        fprintf(stderr, "Error sending file response.\n");
    }

    if (strcmp(request->method, HTTP_GET) == 0) {
        request->bytes = file_size;
        int file_fd = open(full_path, O_RDONLY);
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
                break;
            }
        }
        cache_set(server->cache, url_path, full_path, file_type, response_header, file_size, path_stats.st_mtime);
        if (file_fd >= 0) {
            close(file_fd);
        }
    }

    if (strcmp(request->method, HTTP_HEAD) == 0) {
        request->bytes = 0;
    }        
}

static int send_all(int socket_fd, const char* buffer, size_t len) {
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

static void close_socket(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}


static int send_response(
    Request* request, 
    int client_socket, 
    int status_code,
    const char* status_line, 
    const char* content_type, 
    const char* body, 
    size_t body_len, 
    const char* extra_headers[], 
    size_t extra_count
) {
    char headers[BUFFER_SIZE];
    request->status = status_code;
    request->bytes = body_len;

    size_t headers_size = response_build_headers(headers, BUFFER_SIZE, content_type, status_line, body_len, request->close_connection, extra_headers, extra_count);

    if (send_all(client_socket, headers, headers_size) == -1) {
        return -1;
    }

    if (body && body_len > 0) {
        if (send_all(client_socket, body, body_len) == -1) {
            return -1;
        }
    }

    return 0;
}

static void send_cached_file(Request* request, int client_socket, FileCache* cached_item) {
    char etag[ETAG_SIZE];
    build_etag(etag, NULL, ETAG_SIZE, 0, cached_item->mtime, cached_item->size);

    if (request->if_none_match != NULL && strcmp(request->if_none_match, etag) == 0) {
        send_304_response(request, client_socket);
        return;
    }

    request->status = HTTP_200_CODE;

    if (send_all(client_socket, cached_item->headers, strlen(cached_item->headers)) == -1) {
        fprintf(stderr, "Error sending file response.\n");
    }

    if (strcmp(request->method, HTTP_GET) == 0) {
        request->bytes = cached_item->size;
        off_t offset = 0;
        int fd = open(cached_item->path, O_RDONLY);
        ssize_t sent = portable_sendfile(client_socket, fd, &offset, cached_item->size);
        if (sent == -1) {
            perror("send file from cache failed");
        }
        if (fd > 0) {
            close(fd);
        }
    } else if (strcmp(request->method, HTTP_HEAD) == 0) {
        request->bytes = 0;
    }        
}

static void end_connection(Request* request, int client_socket) {
    close_socket(client_socket);
    free_request(request);
}
