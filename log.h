#ifndef LOG_H
#define LOG_H

#include "server.h"
#include "request.h"

void log_access_request(Server* server, Request* request, double duration_ms);
void log_error(Server* server, const char* error);

#endif // !LOG_H
