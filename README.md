# Gabitoju's miniserver

A simple, multi-process HTTP/1.1 server written in C. It is designed to be small, fast, and portable between Linux and macOS.

## Features

*   **Efficient File Transfer:** Uses the `sendfile()` system call for high-performance, zero-copy file delivery. A portable wrapper ensures it works on both Linux and macOS/BSD.
*   **Metadata Caching**: Implements an in-memory LRU cache to store file metadata (size, modification time, MIME type). This significantly reduces disk I/O by avoiding `stat()` system calls on subsequent requests.
*   **Dynamic IP Detection:** Correctly identifies the original client IP address when running behind a reverse proxy (like Cloudflare or Nginx) by reading a configurable HTTP header (e.g., `CF-Connecting-IP` or `X-Forwarded-For`).
*   **External Configuration:** All settings are managed via a simple `server.conf` file, including port, content path, and log file locations.
*   **MIME Type Support:** Dynamically loads MIME types from a `mime.types` file to serve content with the correct `Content-Type` header.
*   **Logging:** Provides separate, configurable log files for access and error reporting.
*   **ETag Support:** Generates and validates ETags for cache control, supporting `304 Not Modified` responses.

## Internals

The server is built with a modular structure.

*   **`srv.c`**: Main entry point. Initializes the configuration, creates the cache, and starts the server.
*   **`config.c` / `config.h`**: Handles loading and parsing the `server.conf` file.
*   **`server.c`**: Core server logic. It initializes the listening socket, runs the main `accept()` loop, and forks child processes to handle connections. It also contains the `send_file_response` function which integrates the caching logic.
*   **`request.c`**: Parses raw HTTP requests and dynamically checks for the client IP header specified in the configuration.
*   **`log.c`**: Provides functions for access and error logging.
*   **`cache.c`**: Implements a fixed-size LRU cache for file metadata. It uses a hash map for fast lookups and a doubly linked list to manage item recency.
*   **`mime.c` / `hashmap.c`**: Implements a hash map to load and query MIME types from the `mime.types` file.

## Limitations

*   **Concurrency Model Scalability**: The server uses a process-per-connection model (`fork`), which is simple and robust but can be resource-intensive (especially memory) and may not scale to thousands of concurrent connections as efficiently as thread-based or event-driven models.
*   **No Shared Cache**: The cache is created in each child process due to the `fork()` model. It is not shared between concurrent connections, which limits its effectiveness under heavy load. A shared memory cache would be required to solve this.
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