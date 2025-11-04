#ifndef LOG
#define LOG

#include "server.h"
#include "request.h"

void log_access_request(Server* server, Request* request);
void log_error_request(Server* server, const char* error);

#endif // !LOG
