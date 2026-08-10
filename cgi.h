#ifndef CGI_H
#define CGI_H

#include "request.h"

int cgi_handle_request(Request* request, int client_socket, const char* script_path, const char* executable, int server_port);

#endif // CGI_H
