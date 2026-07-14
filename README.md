
*This project was created as part of the 42 curriculum by **dbarba-v** ([**d3bvstack** on github.com](https://github.com/d3bvstack/)), **gamorcil** ([**byte206** on github.com](https://github.com/byte206)), and **atabarea** ([**artabarean** on github.com](https://github.com/artabarean))*

<table>
  <tr>
    <td align="center">
      <a href="https://github.com/d3bvstack">
        <img src="https://github.com/d3bvstack.png" width="80" style="border-radius:10%"><br/>
        <sub><b>dbarba-v</b></sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/Byte206">
        <img src="https://github.com/Byte206.png" width="80" style="border-radius:10%"><br/>
        <sub><b>gamorcil</b></sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/artabarean">
        <img src="https://github.com/artabarean.png" width="80" style="border-radius:10%"><br/>
        <sub><b>atabarea</b></sub>
      </a>
    </td>
  </tr>
</table>

# 98Webserv 
[![C++](https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white)](#)

**98Webserv** is a handcrafted HTTP server built in C++98. The project covers low-level systems and network programming, focusing on areas such as server configuration parsing, sockets, asynchronous I/O, event polling, TCP/IP and HTTP protocols, request-to-response lifecycles, and CGI execution.

<img alt="98webserv cover image" src=".docs/98webserv.png" />

## Table of Contents
- [Description](#description)
  - [Configuration](#configuration)
  - [Connection Management and Event Loop](#connection-management-and-event-loop)
  - [Processing Request Logic](#processing-request-logic)
  - [CGI Logic](#cgi-logic)
  - [Response Transmission](#response-transmission)
- [Instructions](#instructions)
  - [Prerequisites](#prerequisites)
  - [1. Build](#1-build)
  - [2. Run](#2-run)
  - [3. Connect](#3-connect)
  - [4. Clean](#4-clean)
- [Resources](#resources)
  - [Network Programming](#network-programming)
  - [Core HTTP \& Protocol Specifications](#core-http--protocol-specifications)
  - [Guides \& Architecture](#guides--architecture)
  - [HTTP Headers, Methods, \& Status Reference](#http-headers-methods--status-reference)
  - [Web Server Guides (NGINX)](#web-server-guides-nginx)
  - [Caching, Authentication, \& Cookie Management](#caching-authentication--cookie-management)
    - [CGI](#cgi)
- [AI Usage](#ai-usage)

## Description

**98Webserv** is an HTTP server written in C++98 that manages concurrent client connections using a single-threaded, event-driven architecture. To handle multiple connections without the resource overhead of multi-threading or process forking, the server utilizes Linux `epoll` to multiplex I/O across sockets.

### Configuration

The server loads configuration files (`.conf`) written in an ini-like syntax. These files can be passed as arguments or automatically detected within the `.conf/` directory.

*   **Virtual Hosts:** Modeled by the [`Vhost`](include/Vhost.hpp#L22) class, virtual hosts define settings such as server name, host, port, maximum body size, custom error pages, and CGI extension mappings. Syntax errors within a virtual host block are isolated, meaning an invalid host configuration is rejected individually without preventing other valid hosts from loading.
*   **Routing Rules:** Within each virtual host, [`Location`](include/Location.hpp#L22) blocks define URL routing rules. These specify the matching path, a document root or redirection target, permitted HTTP methods, directory indexing, default index files, and upload directories.

The full configuration syntax is documented in the [configuration specification](.docs/configuration.md).

### Connection Management and Event Loop

During initialization, the server creates a listening socket for each unique host-port pair defined in the configuration and registers them with a single [`Epoll`](include/Epoll.hpp#L22) instance. 

The core event loop, driven by [`Epoll::waitWrapper`](src/Epoll.cpp#L72), dispatches I/O events as they occur:

1.  **New Connections:** When a listening socket detects an incoming connection, the server accepts it, wraps it in a [`ClientConnection`](include/sockets/ClientConnection.hpp#L25) object, and registers it with `epoll` for `EPOLLIN` and `EPOLLOUT` events.
2.  **Request Parsing and Validation:** Incoming data is read into a connection-specific buffer. The server verifies that the data begins with a valid HTTP header.
    *   **Header Limits:** Headers are limited to 8 KB. If exceeded, the server returns a `431 Request Header Fields Too Large` response.
    *   **Body Limits:** The server rejects request bodies exceeding the configured `max_body_size` with a `413 Payload Too Large` status.
    *   These checks are performed progressively during the read phase to reject oversized requests before buffering the entire payload.
3.  **Queueing:** Once a request is successfully parsed, a [`Request`](include/Request.hpp#L18) object is created and queued for handling. If initial validation fails, an error [`Response`](include/Response.hpp#L22) is generated and queued directly for transmission.

### Processing Request Logic

During each event loop iteration, [`Server::processPendingRequests`](src/Server.cpp#L751) processes queued requests. Each [`ClientConnection`](include/sockets/ClientConnection.hpp#L25) maintains its own queue of parsed [`Request`](include/Request.hpp#L18) objects, which the server processes in order:

1.  **Location Resolution:** The request path is matched against the virtual host's `Location` blocks via [`server_utils::findBestLocation`](src/server_utils.cpp#L196), selecting the longest matching prefix.
2.  **Request Dispatching:** The request is handled based on the matching [`Location`](include/Location.hpp#L22) rules:
    *   **No Match:** Yields a `404 Not Found` response.
    *   **Method Not Allowed:** If the location does not support the HTTP method, the server returns `405 Method Not Allowed` with an `Allow` header listing permitted methods.
    *   **CGI Request:** Detected by [`server_utils::isCgiRequest`](src/server_utils.cpp#L312). The request is handed to a [`CGIContext`](include/CGIContext.hpp#L15) and processed asynchronously.
    *   **GET:** Resolves the file under the location root, processes redirections, or serves directory listings (if `autoindex` is enabled).
    *   **POST:** Saves the payload to the configured `upload_store` (`201 Created`) or echoes the body back (`200 OK`).
    *   **DELETE:** Removes the target file (`200 OK`) or returns appropriate error codes on failure.
3.  **Connection Policy:** The server applies the connection policy via [`server_utils::applyConnectionPolicy`](src/server_utils.cpp#L284), setting `Connection: keep-alive` or `Connection: close` based on client headers and configuration.
4.  **Error Handling:** Exceptions thrown during request handling degrade to a `500 Internal Server Error`. Finished responses are queued for client transmission, and the handled request is dequeued.

### CGI Logic

CGI execution is managed by the [`CGIContext`](include/CGIContext.hpp#L15) class, which routes requests to external scripts using `fork` and `execve` while integrated with the non-blocking `epoll` loop.

*   **Routing:** The request path is matched against the host's CGI extension map. [`splitCgiPath`](src/CGIContext.cpp#L55) extracts the script path and `PATH_INFO`, mapping them to the filesystem via [`resolveFilesystemPathFromUrlPath`](src/CGIContext.cpp#L84).
*   **Process Execution:** The server creates two `socketpair` pipes for the script's input and output, then calls `fork()`. The child process sets up the CGI environment variables, redirects standard streams, and calls `execve`. The parent process registers the pipe descriptors with `epoll` and tracks the context.
*   **Asynchronous I/O:** The execution transitions through non-blocking states (`WRITING_BODY` → `READING_OUTPUT` → `COMPLETE`/`ERROR_STATE`). Request data is written to the script's input, and the script's output is read progressively.
*   **Completion and Cleanup:** During each tick, [`Server::checkCgiChildren`](src/Server.cpp#L852) calls `waitpid` with `WNOHANG` to monitor the child process. A 5-second timeout is enforced, resulting in a `504 Gateway Timeout` on expiration. Once finished, the output is parsed, headers are processed, the response is queued, and the resources are cleaned up.

### Response Transmission

Once a response is ready, it is serialized and written to the client socket when `epoll` signals that the socket is writable (`EPOLLOUT`).

*   **Non-blocking Writes:** If a response cannot be sent in a single write operation, the remaining data is stored in a buffer and transmitted during subsequent `EPOLLOUT` events.
*   **Completion:** The process continues until all queued responses for the connection have been fully sent.

---

## Instructions

### Prerequisites

* A C++98 compliant compiler (`c++` or `g++`)
* `make`
* A Linux-based environment (for `epoll` support)

### 1. Build

To compile the project:

```sh
make
```

Or target the executable directly:

```sh
make webserv
```

The binary will be created at `bin/webserv`.

### 2. Run

To start the server with the default configuration (which loads all `.conf` files in the `.conf/` directory):

```sh
make run
```

To run with specific configuration files:

```sh
make run path/to/config1.conf path/to/config2.conf
```

Or run the binary directly:

```sh
./bin/webserv
```

### 3. Connect

Once the server is running, navigate to the configured host and port in your browser:

```
http://localhost:8080
```

Alternatively, you can test the connection using `telnet` or `curl`:

```sh
telnet localhost 8080
```

Then send a basic HTTP request:

```http
GET / HTTP/1.1
Host: localhost

```

### 4. Clean

To clean up build artifacts:

```sh
make clean      # Removes object files
make fclean     # Removes object files and the compiled binary
```

---

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

### CGI

* [O'Reilly: CGI Programming on the World Wide Web](https://www.oreilly.com/openbook/cgi/)
* [Philip Bohun: The Magic of cgi-bin](https://www.youtube.com/watch?v=NwRVJX0Ieno)
* [RFC 3875: The Common Gateway Interface](https://datatracker.ietf.org/doc/html/rfc3875)
* [Universidad de Oviedo: The Common Gateway Interface](https://www6.uniovi.es/~antonio/ncsa_httpd/cgi/overview.html)


### AI usage

- Restructuring, styling and extending both `.md` documentation and code comments.
- Fetching, summarising and clarifying webserv related topics.