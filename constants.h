#ifndef CONSTANTS
#define CONSTANTS

#define CONFIG_FILE "/etc/gabitojusrv/server.conf"
#define DEFAULT_PORT 8080

#define BUFFER_SIZE 4096
#define CONFIG_KEY_SIZE 64
#define HTTP_VERSION "HTTP/1.1"
#define HTML_CONTENT_TYPE "text/html"
#define WELCOME_MESSAGE "<b>Welcome to this little HTTP sever</b>"

#define INDEX "index.html"
#define INDEX_SLASH "/index.html"

#define HTTP_GET "GET"
#define HTTP_HEAD "HEAD"

#define MIME_TYPES_DATABASE "/etc/gabitojusrv/mime.types"

#define ACCESS_LOG_PATH "/var/log/gabitojusrv/access.log"
#define ERROR_LOG_PATH "/var/log/gabitojusrv/error.log"

#define REAL_IP_HEADER "X-Forward-For"

#define CACHE_SIZE 265
#define CACHE_TTL 60

#endif //CONSTANTS
