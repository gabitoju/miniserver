# Gabitoju's miniserver

A simple, single-threaded HTTP/1.1 server written in C. It is designed to serve static files and log requests.

## Internals

The server is built with a modular structure.

*   **`srv.c`**: Main entry point. It parses command-line arguments and manages the main server and configuration lifecycle.
*   **`config.h` / `srv.c`**: Handles configuration. A `Config` struct holds settings loaded from `server.conf`. This includes port, content path, and the header to use for the client's real IP.
*   **`server.c`**: Core server logic. It initializes the socket, binds to a port, and enters the main `accept()` loop to handle incoming connections. Each connection is passed to `handle_connection`.
*   **`request.c`**: Responsible for parsing raw HTTP requests. It reads the request line and headers, populating a `Request` struct. It dynamically identifies the client IP by checking for the header specified in the configuration file (e.g., `CF-Connecting-IP`), falling back to the direct connection IP if the header is not present.
*   **`log.c`**: Provides functions for access and error logging.
*   **`mime.c`**: Maps file extensions to MIME types using the `mime.types` file.

## Limitations

This server is a minimalist implementation and has several limitations.

*   **Single-Threaded**: It processes requests sequentially and can only handle one connection at a time. A request for a large file will block all subsequent requests.
*   **HTTP Only**: Does not support HTTPS/TLS. It should be run behind a reverse proxy (like Nginx or Cloudflare) for SSL termination.
*   **Limited HTTP Support**: Implements only `GET` and `HEAD` methods.
*   **IPv4 Only**: The server socket listens only on IPv4 (`AF_INET`). It can log IPv6 addresses passed via a header but cannot accept IPv6 connections directly.
*   **Basic Security**: Includes basic path traversal checks, but has not been hardened against other vulnerabilities like Slowloris attacks.

## Installation and Usage

### Dependencies

*   A C compiler (e.g., `gcc`)
*   `make`

### Compilation

To compile the server, run `make`:

```sh
make
```

This will produce the `srv` executable.

### Installation

To install the server binary and default configuration files:

```sh
sudo make install
```

This command will:
*   Install the `srv` binary to `/usr/local/bin/`.
*   Install `server.conf` and `mime.types` to `/etc/gabitojusrv/`.

### Configuration

The server is configured via `/etc/gabitojusrv/server.conf`. The file uses a `key value` format.

**Example `server.conf`:**
```
# Server port
port 8080

# Path to the website content
content_path /var/www/html

# Path to log files
access_log_path /var/log/gabitojusrv/access.log
error_log_path /var/log/gabitojusrv/error.log

# Header for identifying the real client IP when behind a proxy
# (e.g., "CF-Connecting-IP" for Cloudflare)
real_ip_header X-Forwarded-For
```

### Running the Server

Run the server with the following command:

```sh
srv
```

By default, it will look for the configuration file at `/etc/gabitojusrv/server.conf`.

To specify a different configuration file, use the `-c` flag:

```sh
srv -c /path/to/your/server.conf
```
