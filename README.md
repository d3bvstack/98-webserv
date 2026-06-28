*This project has been created as part of the 42 curriculum by*


# 98Webserv 
[![C++](https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white)](#)

**98Webserv** is a handcrafted HTTP server built in C++98. The project covers low-level systems and network programming, focusing on areas such as server configuration parsing, sockets, asynchronous I/O, event polling, TCP/IP and HTTP protocols, request-to-response lifecycles, and CGI execution.

<img alt="98webserv cover image" src="https://github.com/user-attachments/assets/d021c065-767b-42c5-ae13-b8b9407b1e5a" />



## Table of Contents
- [Description](#description)
- [Instructions](#instructions)
- [Resources](#resources)

## Description

98Webserv is an HTTP server written in C++98 that manages concurrent client connections using a single-threaded, event-driven architecture. To handle multiple connections without the resource overhead of multi-threading or process forking, the server utilizes Linux `epoll` to multiplex I/O across sockets.

### Configuration

The server loads ini-like configuration files (`.conf`), which can be passed as command-line arguments or automatically detected within the `.conf/` directory. A single configuration file can declare one or multiple vhosts.

*   **Virtual Hosts:** Modeled by the [`Vhost`](include/Vhost.hpp#L22) class, virtual hosts define settings such as server name, host, port, maximum body size, custom error pages, and CGI extension mappings. Syntax errors within a virtual host block are isolated, meaning an invalid host configuration is rejected individually without preventing other valid hosts from loading.
*   **Routing Rules:** Within each virtual host, [`Location`](include/Location.hpp#L22) blocks define URL routing rules. These specify the matching path, a document root or redirection target, permitted HTTP methods, directory indexing, default index files, and upload directories.

The full configuration syntax is documented in the [configuration specification](.docs/configuration.md).

### Connection Management and Event Loop

During initialization, the server creates a listening socket for each unique host-port pair defined in the configuration and registers them with a single [`Epoll`](include/Epoll.hpp#L22) instance. 

The core event loop, driven by [`Epoll::waitWrapper`](src/Epoll.cpp#L72), dispatches I/O events as they occur:

1.  **New Connections:** When a listening socket detects an incoming connection, the server accepts it and wraps it in a [`ClientConnection`](include/sockets/ClientConnection.hpp#L25) object. This connection is then registered with `epoll` for both `EPOLLIN` and `EPOLLOUT` notifications.
2.  **Request Parsing and Validation:** Incoming data accumulates in a buffer dedicated to that connection. The server verifies that the data begins with a valid HTTP request header.
    *   **Header Limits:** The server enforces an 8 KB limit on the total header size. If exceeded, it responds with `431 Request Header Fields Too Large`.
    *   **Body Limits:** The server rejects request bodies that exceed the virtual host's configured `max_body_size` with a `413 Payload Too Large` status. 
    *   These checks are performed progressively while data is being read from the socket, allowing the server to reject oversized requests before buffering the entire payload.
3.  **Queueing:** Once a request is successfully received and parsed, the server instantiates a [`Request`](include/Request.hpp#L18) object and queues it for handling via [`Server::processPendingRequests`](src/Server.cpp#L618). If validation fails early, an error [`Response`](include/Response.hpp#L22) is generated and queued directly for transmission.

[Processing requests logic]
[CGI logic]

### Response Transmission

Once processed, the [`Response`](include/Response.hpp#L22) is serialized and written back to the client socket when `epoll` signals that the socket is writable (`EPOLLOUT`). 

*   **Non-blocking Transmission:** If a response is too large to write in a single system call, the remaining unsent data is retained in a buffer. Transmission resumes upon the next `EPOLLOUT` event.
*   **Completion:** This cycle repeats until all queued responses for the connection have been fully transmitted.

## Instructions

### Prerequisites

A C++98 compiler (`c++`), `make`, and a Linux environment with `epoll` support.

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
