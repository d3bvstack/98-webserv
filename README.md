*This project has been created as part of the 42 curriculum by*

# Webserv

## Table of Contents
- [Description](#description)
- [Instructions](#instructions)
- [Resources](#resources)

## Description

Webserv is an HTTP/1.1 server written in C++98 that handles concurrent client connections through a single-threaded, event-driven architecture. Rather than forking or threading per connection, it relies on Linux `epoll` to multiplex I/O across many sockets efficiently.

The server loads configuration from `.conf` files, which may be specified as command-line arguments or discovered automatically in the `.conf/` directory. Each file defines one or more virtual hosts, modelled by the [`Vhost`](include/Vhost.hpp#L22) class, with per-host settings for server name, host, port, maximum body size, custom error pages, and CGI extension mappings. Within each virtual host, [`Location`](include/Location.hpp#L22) blocks define URL routing rules — the path to match, a document root or redirect target, allowed HTTP methods, directory indexing, default index files, and upload directories. Invalid vhosts are rejected individually, so a syntax error in one configuration never invalidates another. The full configuration syntax is documented in the [configuration specification](.docs/configuration.md).

Once parsed, the server creates one listening socket for every unique host:port pair across all vhosts and registers all of them with a single [`Epoll`](include/Epoll.hpp#L22) instance. The event loop — driven by [`Epoll::waitWrapper`](src/Epoll.cpp#L72) — dispatches every I/O event in turn. When a listening socket becomes readable, the server accepts the connection and wraps it in a [`ClientConnection`](include/sockets/ClientConnection.hpp#L25) object, which is then registered for `EPOLLIN` and `EPOLLOUT` notifications. On client sockets, incoming data accumulates in a per-connection buffer. The server validates that the data starts with a well-formed HTTP request and, when the request is complete, constructs a [`Request`](include/Request.hpp#L18) object and queues it for processing via [`Server::processPendingRequests`](src/Server.cpp#L618). If the reques is either incorrectly formatted or body size is larger than `max_body_size` a [`Response`](include/Response.hpp#L22) is constructed and queued for sending to client.

[Processing requests logic]

After processing, the [`Response`](include/Response.hpp#L22) is serialized and written back to the client whenever `epoll` signals that the socket is writable. If the response is too large to send in a single call, the unwritten portion is retained in a buffer and transmission resumes on the next `EPOLLOUT` event. This cycle repeats until the entire response has been delivered and no further responses are queued for that connection.

[CGI logic]

## Instructions

### Prerequisites

A C++98-capable compiler (`c++`), `make`, and a Linux environment with `epoll` support.

### 1. Build

```sh
make
```

Or as an explicit target:

```sh
make webserv
```

The binary is placed at `bin/webserv`.

### 2. Run

Start the server with default configs (loads all `.conf` files from the `.conf/` directory):

```sh
make run
```

Or specify one or more configuration files explicitly:

```sh
make run path/to/config1.conf path/to/config2.conf
```

The path to the binary also works:

```sh
./bin/webserv
```

### 3. Connect

Once the server logs that it is listening, open a browser and navigate to:

```
http://<host>:<port>
```

For example, if the server listens on `127.0.0.1:8080`:

```
http://localhost:8080
```

Alternatively, connect via telnet and send a raw HTTP request:

```sh
telnet localhost 8080
```

Then paste the following (end with a blank line):

```
GET / HTTP/1.1
Host: localhost

```

### 4. Clean

```sh
make clean      # Remove object files
make fclean     # Remove object files and the binary
```

## Resources

### Network Programming
* [Beej's Guide to Network Programming: Using Internet Sockets](https://beej.us/guide/bgnet/)
* [LinuxHowtos: Sockets Tutorial](https://www.linuxhowtos.org/C_C++/socket.htm)

### Core HTTP & Protocol Specifications
* [MDN Web Docs: HTTP (Hypertext Transfer Protocol)](https://developer.mozilla.org/en/docs/Web/HTTP)
* [RFC 9112: Hypertext Transfer Protocol (HTTP/1.1) Specification](https://www.rfc-editor.org/info/rfc9112/)
* [MDN Web Docs: An Overview of HTTP](https://developer.mozilla.org/en/docs/Web/HTTP/Guides/Overview)
* [Wikipedia: Transmission Control Protocol (TCP)](https://en.wikipedia.org/wiki/Transmission_Control_Protocol)

### Guides & Architecture
* [MDN Web Docs: Evolution of HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Evolution_of_HTTP)
* [MDN Web Docs: HTTP Sessions](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Session)
* [MDN Web Docs: HTTP Messages](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Messages)
* [MDN Web Docs: HTTP MIME Types](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/MIME_types)
* [MDN Web Docs: Redirections in HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Redirections)

### HTTP Headers, Methods, & Status Reference
* [MDN Web Docs: HTTP Headers Reference](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers)
* [MDN Web Docs Glossary: Request Header](https://developer.mozilla.org/en-US/docs/Glossary/Request_header)
* [MDN Web Docs Glossary: Response Header](https://developer.mozilla.org/en-US/docs/Glossary/Response_header)
* [MDN Web Docs: HTTP Request Methods Reference](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods)
* [MDN Web Docs: GET HTTP Method](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/GET)
* [MDN Web Docs: POST HTTP Method](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/POST)
* [MDN Web Docs: DELETE HTTP Method](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/DELETE)
* [MDN Web Docs: HTTP Response Status Codes Reference](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status)

### Web Server Guides (NGINX)
* [NGINX: Beginner's Guide](https://nginx.org/en/docs/beginners_guide.html)
* [NGINX: How NGINX Processes a Request](https://nginx.org/en/docs/http/request_processing.html)

### Caching, Authentication, & Cookie Management
* [MDN Web Docs: HTTP Caching Guide](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Caching)
* [MDN Web Docs: HTTP Authentication Guide](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Authentication)
* [MDN Web Docs: Using HTTP Cookies](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Cookies)