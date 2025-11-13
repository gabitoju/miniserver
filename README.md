# Gabitoju's miniserver

A simple, single-threaded HTTP/1.1 server written in C. It is designed to be small, fast, and portable between Linux and macOS.

## Features

*   **Efficient File Transfer:** Uses the `sendfile()` system call for high-performance, zero-copy file delivery. A portable wrapper ensures it works on both Linux and macOS/BSD.
*   **Dynamic IP Detection:** Correctly identifies the original client IP address when running behind a reverse proxy (like Cloudflare or Nginx) by reading a configurable HTTP header (e.g., `CF-Connecting-IP` or `X-Forwarded-For`).
*   **External Configuration:** All settings are managed via a simple `server.conf` file, including port, content path, and log file locations.
*   **MIME Type Support:** Dynamically loads MIME types from a `mime.types` file to serve content with the correct `Content-Type` header.
*   **Logging:** Provides separate, configurable log files for access and error reporting.
*   **ETag Support:** Generates and validates ETags for cache control, supporting `304 Not Modified` responses.

## Internals

The server is built with a modular structure.

*   **`srv.c`**: Main entry point. Initializes the configuration and the server.
*   **`config.c` / `config.h`**: Handles loading and parsing the `server.conf` file.
*   **`server.c`**: Core server logic. It initializes the socket and enters the main `accept()` loop. It contains the `send_file_response` function which uses a portable `sendfile()` wrapper for efficient file delivery.
*   **`request.c`**: Parses raw HTTP requests and dynamically checks for the client IP header specified in the configuration.
*   **`log.c`**: Provides functions for access and error logging.
*   **`mime.c` / `hashmap.c`**: Implements a hash map to load and query MIME types from the `mime.types` file.

## Limitations

*   **Single-Threaded**: It processes requests sequentially. A blocking operation (like a slow network connection) will block the entire server.
*   **HTTP Only**: Does not support HTTPS/TLS. It should be run behind a reverse proxy for SSL termination.
*   **Limited HTTP Support**: Implements only `GET` and `HEAD` methods.
*   **IPv4 Only**: The server socket listens only on IPv4 (`AF_INET`).

## Installation and Usage

### Dependencies

*   A C compiler (e.g., `gcc` or `clang`)
*   `make`

### Compilation

To compile the server, run `make`:

```sh
make
```

This will produce the `srvd` executable.

### Installation

To install the server binary and default configuration files:

```sh
sudo make install
```

This command will:
*   Install the `srvd` binary to `/usr/local/bin/`.
*   Install `server.conf` and `mime.types` to `/etc/srv/`.
*   Create a web directory at `/var/www/srv` and a log directory at `/var/log/gabitojusrv`.

### Configuration

The server is configured via `/etc/srv/server.conf`. The file uses a `key value` format.

**Example `server.conf`:**
```
# Server port
port 8080

# Path to the website content
content_path /var/www/srv

# Path to log files
access_log_path /var/log/gabitojusrv/access.log
error_log_path /var/log/gabitojusrv/error.log

# Header for identifying the real client IP
real_ip_header CF-Connecting-IP
```

### Running the Server

Run the server with the following command:

```sh
srvd
```

By default, it will look for the configuration file at `/etc/srv/server.conf`.

To specify a different configuration file, use the `-c` flag:

```sh
srvd -c /path/to/your/server.conf
```