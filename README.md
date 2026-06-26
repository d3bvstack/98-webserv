*This project has been created as part of the 42 curriculum by*

# Webserv

## Table of Contents
- [Description](#description)
- [Instructions](#instructions)
- [Resources](#resources)

## Description
This project is an implementation of an HTTP server in the C++98 standard. The webserver is implemented to either accept files specified as arguments when running the executable, or if no arguments provided, it searches the default configurations directory ".conf/" and treats, every .conf file as a configurations file. A single configuration file can define a single or multiple vhosts [`include/Vhost.hpp`](include/Vhost.hpp).  The application uses the configuration file to define virtual hosts, per-vhost limits, URL routing rules, etc. The webserver parses the configuration files for correct syntax and required fields, when a configuration is not valid it doesn't invalidate other vhost configurations. For a configuration file to be considered valid, it must follow the rules and syntax outlined in [`.docs/configuration`](.docs/configuration.md).
Once the vhosts and locations are parsed, the server creates listenning sockets bound to the interfaces defined in the respective configurations of each vhost. These listenning sockets are added to an epoll instance that polls for events on the associated file descriptors, when a connection request happens, the server accepts it, creates a ClientConncection object for comunication with client and adds the new file descriptor to the epoll instance to poll for EPOLLIN/EPOLLOUT events as well as errors/closing conection.
Epoll indicated event both on listenning sockets and client connection, when an event is on clientconnection, if is a read event, the fd is received and saved to a buffer, a check is performed to verify if it is the start of a valid http request, if it is, then checks if the request is complete, if not it saves the buffer for next read event on socket until complete request or max body size reached. If complete request received, create object from raw bytes request and added to the pending requests.
[Processing requests logic].
Then the response is converted to raw bits and sent through client connection socket whenever the epoll notifies fd is writable, if message is longer than the sent number of bytes, the remaining buffer is stored for next epoll write event and so on until the complete response is sent and no more responses are queued for sending through the client socket.
[CGI logic]

## Instructions
To build the executable a makefile is provided with targets `make webserv` and `make`, these will create the executable on bin/ direcctory.
To run, the makefile provides target `make run` (also recompiles if any file changed from last make), although can be executed by running `./bin/webserv` from root directory.
Program can be executed with or without arguments, it will either use arguments as paths to config files to use or if no arguments it will use the ones it finds on default directory.
Once the server logs output that it is listenning on the ports, you can connect to server.
[Instructions for connecting with both browser or telnet]

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