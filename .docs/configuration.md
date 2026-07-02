## Configuration File Specification

A valid configuration file must strictly adhere to the rules, syntax, and structures outlined below.

## File Requirements

* Extension: File must use the `.conf` extension.
* Comments: Indicated by a semicolon `;`. Everything after `;` on a line is ignored.
* Indentation: Will be ignored, but recommended for clarity.

------------------------------

## Virtual Host (vhost) Definition

Every configuration file must contain at least one vhost block.

## Syntax Boundary

A virtual host is defined between the following tags:

```
[vhost.start]
# Settings go here
[vhost.end]
```

## Global vhost Settings

### `server_names` (Required)
* Defines the domains the website responds to.
   * Accepts multiple space-separated values.
   * Example: `server_name = domain.com www.domain.com`
### `host` (Required)
* Defines the IPv4 address to bind to.
   * Accepts only one value.
   * Example: `host = 0.0.0.0`
### `listen` (Required)
* Defines the port to listen for incomming connections.
   * Accepts only one value.
   * Example: `listen = 9898`
### `max_body_size` (Optional)
* Defines the maximum client request size in bytes.
   * Defaults to 1MB (1048576) if omitted.
   * Example: `max_body_size = 1048576`
### `keep_alive_timeout` (Optional)
* Defines the maximum idle time in seconds before a keep-alive connection is closed by the server.
   * Defaults to `60` seconds if omitted.
   * Example: `keep_alive_timeout = 30`
### `error_page` (Optional)
* Maps an HTTP error code to a custom HTML file path.
   * Example: `error_page = 404 /path/to/error/page`
### `cgi` (Optional)
* Maps a file extension to an executable binary path.
   * Example: `cgi = .py /usr/bin/python3`

------------------------------

## Location Definition

Every vhost must contain at least one location block, and a location for the root path (/) is strictly required.

## Syntax Boundary

A location block is enclosed within the following tags:

[location.start]
# Location settings go here
[location.end]

## Location Settings

### path (Required)
* Defines the URI route to match.
   * Example: `path = /` or `path = /route/to/match`
### root (Required if return is absent)
* Defines the local directory for website files.
   * Example: `root = /var/www/html`
### return (Required if root is absent)
* Defines an HTTP redirect code and target URL.
   * Example: `return = 308 /var/www/new/page`
### methods (Optional)
* Defines allowed HTTP methods for the route.
   * Defaults to none.
   * Example: `methods = GET POST DELETE`
### autoindex (Optional)
* Enables or disables directory listing for the location.
   * Accepts `true` or `false`.
   * Defaults to `false` if omitted.
   * Example: `autoindex = true`
### default (Optional)
* Specifies default files to serve when accessing a directory.
   * Multiple files can be defined
   * Example: `default = index.html index.php index`
### upload_store (Optional)
* Defines the directory where uploaded files are stored.
   * Example: `upload_store = /var/www/uploads`
### max_body_size (Optional)
* Defines the maximum client request size in bytes for this location.
   * Overrides the vhost-level `max_body_size` if set.
   * Example: `max_body_size = 5242880`

------------------------------

## Configuration Example

Example of a valid .conf file:

```
; Main web server configuration
[vhost.start]
server_name = example.com www.example.com
host = 127.0.0.1
listen = 8080
max_body_size = 2097152
error_page = 404 /html/404.html
cgi = .py /usr/bin/python3

    ; Required root location block
    [location.start]
    path = /
    root = /var/www/mysite
    [location.end]

    ; Optional redirect location block
    [location.start]
    path = /old-page
    return = 301 /new-page
    [location.end]
[vhost.end]
```
