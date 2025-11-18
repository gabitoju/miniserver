#ifndef LOG_H
#define LOG_H

#include "server.h"
#include "request.h"

void log_access_request(Server* server, Request* request);
void log_error(Server* server, const char* error);

#endif // !LOG_H
