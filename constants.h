#ifndef CONSTANTS_H
#define CONSTANTS_H

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define CONFIG_FILE "/etc/gabitojusrv/server.conf"
#define DEFAULT_PORT 8080

#define BUFFER_SIZE 4096
#define CONFIG_KEY_SIZE 64
#define HTTP_VERSION "HTTP/1.1"
#define HTML_CONTENT_TYPE "text/html"
#define TEXT_CONTENT_TYPE "text/plain"
#define WELCOME_MESSAGE "<b>Welcome to this little HTTP server</b>"

#define INDEX "index.html"
#define INDEX_SLASH "/index.html"

#define HTTP_GET "GET"
#define HTTP_HEAD "HEAD"

#define MIME_TYPES_DATABASE "/etc/gabitojusrv/mime.types"

#define ACCESS_LOG_PATH "/var/log/gabitojusrv/access.log"
#define ERROR_LOG_PATH "/var/log/gabitojusrv/error.log"

#define REAL_IP_HEADER "X-Forwarded-For"

#define CACHE_SIZE 256
#define CACHE_TTL 60

#define MAX_KEEPALIVE_REQUESTS 1000

#define HTTP_200_CODE 200

#define HTTP_403_CODE 403
#define HTTP_403_MESSAGE "403 Forbidden"
#define HTTP_403_MESSAGE_LEN 13 
#define HTTP_403_STATUS_LINE HTTP_VERSION " " HTTP_403_MESSAGE "\r\n"

#define HTTP_404_CODE 404
#define HTTP_404_MESSAGE "404 Not Found"
#define HTTP_404_MESSAGE_LEN 13 
#define HTTP_404_STATUS_LINE HTTP_VERSION " " HTTP_404_MESSAGE "\r\n"

#define HTTP_405_CODE 405
#define HTTP_405_MESSAGE "405 Method Not Allowed"
#define HTTP_405_MESSAGE_LEN 22
#define HTTP_405_STATUS_LINE HTTP_VERSION " " HTTP_405_MESSAGE "\r\n"
#define HTTP_405_EXTRA_HEADER "Allow: GET, HEAD"

#define HTTP_304_CODE 304
#define HTTP_304_MESSAGE "Not Modified"
#define HTTP_304_MESSAGE_LEN 12 
#define HTTP_304_STATUS_LINE HTTP_VERSION " " HTTP_304_MESSAGE "\r\n"

#define HTTP_301_CODE 301
#define HTTP_301_MESSAGE "301 Moved Permanently"
#define HTTP_301_MESSAGE_LEN 21
#define HTTP_301_STATUS_LINE HTTP_VERSION " " HTTP_301_MESSAGE "\r\n"

#endif //CONSTANTS_H
