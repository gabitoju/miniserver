#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/wait.h>

#include "cgi.h"
#include "constants.h"
#include "response.h"

#define CGI_MAX_OUTPUT_SIZE (1024 * 1024)
#define CGI_MAX_HEADERS 16

static int cgi_send_all(int socket_fd, const char* buffer, size_t len);
static int cgi_write_all(int fd, const char* buffer, size_t len);
static int cgi_read_output(int fd, char** output, size_t* output_size);
static int cgi_send_response(Request* request, int client_socket, char* output, size_t output_size);
static char* cgi_header_value(char* line, const char* name);

int cgi_handle_request(Request* request, int client_socket, const char* script_path, const char* executable, int server_port) {
    int input_pipe[2] = { -1, -1 };
    int output_pipe[2] = { -1, -1 };

    if (pipe(input_pipe) == -1 || pipe(output_pipe) == -1) {
        if (input_pipe[0] >= 0) {
            close(input_pipe[0]);
            close(input_pipe[1]);
        }
        return -1;
    }

    sigset_t block_sigchld;
    sigset_t old_mask;
    sigemptyset(&block_sigchld);
    sigaddset(&block_sigchld, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &block_sigchld, &old_mask) == -1) {
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        return -1;
    }

    if (pid == 0) {
        close(input_pipe[1]);
        close(output_pipe[0]);

        if (dup2(input_pipe[0], STDIN_FILENO) == -1 || dup2(output_pipe[1], STDOUT_FILENO) == -1) {
            _exit(127);
        }

        close(input_pipe[0]);
        close(output_pipe[1]);

        sigprocmask(SIG_SETMASK, &old_mask, NULL);

        char port[16];
        snprintf(port, sizeof(port), "%d", server_port);
        setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
        setenv("REQUEST_METHOD", request->method ? request->method : "", 1);
        setenv("SCRIPT_NAME", request->path ? request->path : "", 1);
        setenv("SCRIPT_FILENAME", script_path, 1);
        setenv("QUERY_STRING", request->query_params ? request->query_params : "", 1);
        setenv("SERVER_PROTOCOL", request->version ? request->version : HTTP_VERSION, 1);
        setenv("SERVER_NAME", request->host ? request->host : "", 1);
        setenv("SERVER_PORT", port, 1);
        setenv("REMOTE_ADDR", request->client_ip ? request->client_ip : "", 1);
        setenv("HTTP_X_REQUEST_ID", request->request_id, 1);

        if (request->has_content_length) {
            char content_length[32];
            snprintf(content_length, sizeof(content_length), "%zu", request->content_length);
            setenv("CONTENT_LENGTH", content_length, 1);
        }
        if (request->content_type) {
            setenv("CONTENT_TYPE", request->content_type, 1);
        }

        execl(executable, executable, (char*)NULL);
        _exit(127);
    }

    close(input_pipe[0]);
    close(output_pipe[1]);

    struct sigaction ignore_pipe = { .sa_handler = SIG_IGN };
    struct sigaction old_pipe;
    sigemptyset(&ignore_pipe.sa_mask);
    sigaction(SIGPIPE, &ignore_pipe, &old_pipe);
    int write_result = cgi_write_all(input_pipe[1], request->body, request->content_length);
    sigaction(SIGPIPE, &old_pipe, NULL);
    close(input_pipe[1]);

    char* output = NULL;
    size_t output_size = 0;
    int read_result = cgi_read_output(output_pipe[0], &output, &output_size);
    close(output_pipe[0]);

    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            sigprocmask(SIG_SETMASK, &old_mask, NULL);
            free(output);
            return -1;
        }
    }
    sigprocmask(SIG_SETMASK, &old_mask, NULL);

    if (write_result == -1 || read_result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        free(output);
        return -1;
    }

    int result = cgi_send_response(request, client_socket, output, output_size);
    free(output);
    return result;
}

static int cgi_send_all(int socket_fd, const char* buffer, size_t len) {
    size_t total_sent = 0;

    while (total_sent < len) {
        ssize_t sent = write(socket_fd, buffer + total_sent, len - total_sent);
        if (sent == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (sent == 0) {
            return -1;
        }
        total_sent += (size_t)sent;
    }

    return 0;
}

static int cgi_write_all(int fd, const char* buffer, size_t len) {
    if (len == 0) {
        return 0;
    }

    size_t total_written = 0;
    while (total_written < len) {
        ssize_t written = write(fd, buffer + total_written, len - total_written);
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        total_written += (size_t)written;
    }

    return 0;
}

static int cgi_read_output(int fd, char** output, size_t* output_size) {
    char* buffer = malloc(CGI_MAX_OUTPUT_SIZE + 1);
    if (buffer == NULL) {
        return -1;
    }

    size_t total_read = 0;
    while (total_read < CGI_MAX_OUTPUT_SIZE) {
        ssize_t read_count = read(fd, buffer + total_read, CGI_MAX_OUTPUT_SIZE - total_read);
        if (read_count == -1) {
            if (errno == EINTR) {
                continue;
            }
            free(buffer);
            return -1;
        }
        if (read_count == 0) {
            buffer[total_read] = '\0';
            *output = buffer;
            *output_size = total_read;
            return 0;
        }
        total_read += (size_t)read_count;
    }

    free(buffer);
    return -1;
}

static int cgi_send_response(Request* request, int client_socket, char* output, size_t output_size) {
    char* header_end = NULL;
    for (size_t i = 0; i + 3 < output_size; i++) {
        if (memcmp(output + i, "\r\n\r\n", 4) == 0) {
            header_end = output + i + 4;
            output[i] = '\0';
            break;
        }
    }

    if (header_end == NULL) {
        return -1;
    }

    const char* content_type = TEXT_CONTENT_TYPE;
    const char* extra_headers[CGI_MAX_HEADERS];
    size_t extra_count = 0;
    char status_line[64];
    snprintf(status_line, sizeof(status_line), "%s 200 Ok", HTTP_VERSION);

    char* saveptr;
    char* line = strtok_r(output, "\r\n", &saveptr);
    while (line != NULL) {
        char* value = cgi_header_value(line, "Status");
        if (value != NULL) {
            char* endptr;
            long status = strtol(value, &endptr, 10);
            if (endptr == value || status < 100 || status > 599) {
                return -1;
            }
            snprintf(status_line, sizeof(status_line), "%s %s", HTTP_VERSION, value);
            request->status = (int)status;
        } else if ((value = cgi_header_value(line, "Content-Type")) != NULL) {
            content_type = value;
        } else if ((value = cgi_header_value(line, "Content-Length")) != NULL) {
            char* endptr;
            unsigned long long length = strtoull(value, &endptr, 10);
            size_t body_size = output_size - (size_t)(header_end - output);
            if (endptr == value || *endptr != '\0' || length != body_size) {
                return -1;
            }
        } else if (cgi_header_value(line, "X-Request-ID") != NULL) {
            continue;
        } else if (cgi_header_value(line, "Connection") != NULL || cgi_header_value(line, "Transfer-Encoding") != NULL) {
            return -1;
        } else {
            if (strchr(line, ':') == NULL || extra_count == CGI_MAX_HEADERS) {
                return -1;
            }
            extra_headers[extra_count++] = line;
        }

        line = strtok_r(NULL, "\r\n", &saveptr);
    }

    size_t body_size = output_size - (size_t)(header_end - output);
    char headers[BUFFER_SIZE * 2];
    size_t headers_size = response_build_headers(headers, sizeof(headers), content_type, status_line, body_size, request->close_connection, extra_headers, extra_count, request->request_id);
    if (cgi_send_all(client_socket, headers, headers_size) == -1) {
        return -1;
    }

    request->status = request->status ? request->status : HTTP_200_CODE;
    request->bytes = strcmp(request->method, HTTP_HEAD) == 0 ? 0 : body_size;
    if (strcmp(request->method, HTTP_HEAD) == 0 || body_size == 0) {
        return 0;
    }

    return cgi_send_all(client_socket, header_end, body_size);
}

static char* cgi_header_value(char* line, const char* name) {
    size_t name_length = strlen(name);
    if (strncasecmp(line, name, name_length) != 0 || line[name_length] != ':') {
        return NULL;
    }

    char* value = line + name_length + 1;
    while (*value == ' ' || *value == '\t') {
        value++;
    }
    return value;
}
